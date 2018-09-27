#include "Session.hpp"
#include "Rpc.hpp"
#include "RpcRequest.hpp"
#include "RpcResponse.hpp"
#include "RpcHeader.hpp"
#include "Utils.hpp"
#include "Console.hpp"

using boost::asio::ip::tcp;
using nlohmann::json;
namespace ip = boost::asio::ip;
namespace asio = boost::asio;
namespace ssl = boost::asio::ssl;
using namespace std::literals;

rmcmd::Session::Session( std::unique_ptr<ssl::stream<tcp::socket>> socket )
    : socket_{ std::move( socket ) },
    strand_{ std::make_shared<boost::asio::io_service::strand>( socket_->get_io_service( ) ) },
    remoteAddress_{ socket_->lowest_layer( ).remote_endpoint( ).address( ).to_string( ) },
    remotePort_{ socket_->lowest_layer( ).remote_endpoint( ).port( ) } { }

rmcmd::Session::~Session( ) {
    disconnect( );
}

void rmcmd::Session::start( ) {
    readHeader( );
}

void rmcmd::Session::sendRequest( RpcRequest req,
                                  OnResponse onResponse ) {
    sentRequests_[ req.getId( ) ] = std::move( onResponse );

    send( std::move( req ) );
}

void rmcmd::Session::sendRequest( RpcRequest req ) {
    req.responseIsNotRequired( );

    send( std::move( req ) );
}

void rmcmd::Session::sendRequest( std::vector<std::uint8_t>&& rpcRequestBytes ) {
    send( std::move( rpcRequestBytes ) );
}

void rmcmd::Session::onRequest( RpcRequest&& req ) {
    auto findIt{ rpcMethods_.find( req.methodName( ) ) };
    bool isExist{ findIt != rpcMethods_.cend( ) };
    bool isResponseRequired{ req.isResponseRequired( ) };

    RpcResponse res;

    if( isExist ) {
        try {
            res = std::get<OnRequest>( *findIt )( req );
        } catch( const std::exception& err ) {
            auto what{ ""s + err.what( ) };
            res = req.makeResponseError( RpcResponse::ErrorCode::invalidRequest,
                                         ""s + err.what( ) );
        }
    } else {
        res = req.makeResponse( ); // TODO: Remove unused response
        // THIS IS CONSOLE METHOD
        console_->sendRequestToConsoleProcess( std::move( req ) );
    }

    if( isResponseRequired )
        send( std::move( res ) );
}

void rmcmd::Session::onResponse( RpcResponse&& rpcResponse ) {
    auto findIt{ sentRequests_.find( rpcResponse.getId( ) ) };
    bool isExist{ findIt != sentRequests_.cend( ) };

    if( isExist ) {
        std::get<OnResponse>( *findIt )( rpcResponse );
        sentRequests_.erase( findIt );
    } else {
        
    }
}

void rmcmd::Session::bindRpcFunc( std::string funcName,
                                  OnRequest onRequest ) {
    if( rpcMethods_.find( funcName ) != rpcMethods_.cend( ) )
        throw std::invalid_argument{ "RpcMethods already exists" };
    rpcMethods_.emplace( std::move( funcName ), std::move( onRequest ) );
}

void rmcmd::Session::send( Rpc&& rpc ) {
    bool writeInProgress{ !writeQueue_.empty( ) };
    writeQueue_.emplace( std::move( rpc ) );

    if( !writeInProgress )
        doSend( );
}

void rmcmd::Session::send( std::vector<std::uint8_t>&& rpc ) {
    bool writeInProgress{ !writeQueue_.empty( ) };
    writeQueue_.emplace( std::move( rpc ) );

    if( !writeInProgress )
        doSend( );
}

void rmcmd::Session::doSend( ) {
    writeHeader( );
}

void rmcmd::Session::disconnect( ) {
    if( onSessionEnd_ ) {
        onSessionEnd_( remoteAddress_, remotePort_ );
        onSessionEnd_ = nullptr;

        boost::system::error_code ec;
    //    socket_->shutdown( ec ); // BUG endless wait
        socket_->lowest_layer( ).close( ec );
        console_->stop( );
    }
}

void rmcmd::Session::invokeInThisThread( std::function<void( )> callback ) {
    strand_->post( std::move( callback ) );
}

void rmcmd::Session::sendRequestThreadSafe( RpcRequest req, OnResponse onRes ) {
    invokeInThisThread( [ this, mreq = std::move( req ), monRes = std::move( onRes ) ]{
        sendRequest( std::move( mreq ), std::move( monRes ) );
    } );
}

ssl::stream<tcp::socket>::lowest_layer_type& rmcmd::Session::socket( ) {
    return socket_->lowest_layer( );
}

void rmcmd::Session::readHeader( ) {
    asio::async_read( *socket_, asio::buffer( headerBuff_, RpcHeader::SIZE ), strand_->wrap(
        [ this, self = shared_from_this( ) ]
    ( const boost::system::error_code& err, std::size_t readed ) {
        if( err ) {
            auto msg{ err.message( ) };
            disconnect( );
        } else {
            try {
                header_ = headerBuff_;
                readBody( );
            } catch( const std::exception& err ) {
                auto what{ ""s + err.what( ) };
                disconnect( );
            }
        }
    } ) );
}

void rmcmd::Session::readBody( ) {
    bytes_.resize( header_.bodySize( ) );

    asio::async_read( *socket_, asio::buffer( bytes_, header_.bodySize( ) ), strand_->wrap(
        [ this, self = shared_from_this( ) ]
    ( const boost::system::error_code& err, std::size_t readed ) {
        if( err ) {
            auto msg{ err.message( ) };
            disconnect( );
        } else {
            try {
                if( isAuthorized_ ) {
                    console_->sendRequestToConsoleProcess( std::move( bytes_ ) );
                } else {
                    body_ = bytes_;
                    //auto dump{ body_.getJson( ).dump( 0 ) };
                    if( body_.isRequest( ) ) {
                        onRequest( body_.moveToRequest( ) );
                    } else { // isResponse
                        onResponse( body_.moveToResponse( ) );
                    }
                }

                readHeader( );
            } catch( const std::exception& err ) {
                auto what{ ""s + err.what( ) };
                disconnect( );
            }
        }
    } ) );
}

void rmcmd::Session::writeHeader( ) {
    if( !writeQueue_.empty( ) ) {
        try {
            auto& nextRpc{ writeQueue_.front( ) };
            
            if( auto rpc = boost::get<Rpc>( &nextRpc ) ) {
                write_.bodyBytes_ = rpc->toBytes( );
            } else {
                write_.bodyBytes_ = std::move( *boost::get<std::vector<std::uint8_t>>( &nextRpc ) );
            }

            write_.rpcHeader_ = static_cast< RpcHeader::body_size_t >( write_.bodyBytes_.size( ) );
            write_.rpcHeader_.toBuffer( write_.headerBuff_ );

            boost::asio::async_write( *socket_, asio::buffer( write_.headerBuff_, RpcHeader::SIZE ),
                                      strand_->wrap( [ this, self = shared_from_this( ) ]
            ( const boost::system::error_code& err, std::size_t writed ) {
                if( err ) {
                    auto msg{ err.message( ) };
                    disconnect( );
                } else {
                    writeBody( );
                }
            } ) );
        } catch( const std::exception& err ) {
            auto what{ ""s + err.what( ) };
            disconnect( );
        }
    }
}

void rmcmd::Session::writeBody( ) {
    boost::asio::async_write( *socket_, asio::buffer( write_.bodyBytes_ ),
                              strand_->wrap( [ this, self = shared_from_this( ) ]
    ( const boost::system::error_code& err, std::size_t writed ) {
        if( err ) {
            auto msg{ err.message( ) };
            disconnect( );
        } else {
            writeQueue_.pop( );
            writeHeader( );
        }
    } ) );
}

void rmcmd::Session::setOnSessionEndCallback( OnSessionEnd callback ) {
    assert( callback );
    onSessionEnd_ = std::move( callback );
}

void rmcmd::Session::setOnSessionLoginCallback( OnSessionLogin callback ) {
    assert( callback );
    onSessionLogin_ = std::move( callback );
}

void rmcmd::Session::setUpdateInterval( const std::chrono::milliseconds& updateInterval ) {
    updateInterval_ = updateInterval;
}