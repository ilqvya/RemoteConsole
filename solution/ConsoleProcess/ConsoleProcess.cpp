#include "ConsoleProcess.hpp"
#include "RpcRequest.hpp"
#include "RpcHeader.hpp"
#include <thread>
#include <chrono>

using namespace std::literals;
namespace asio = boost::asio;
using asio::ip::tcp;

rmcmd::ConsoleProcess::ConsoleProcess( std::uint16_t parentPort )
    : parentPort_{ parentPort },
    socket_{ io_service_ }
{
    bindRpcFunc( "Exit", [ this ] ( auto ) {
        this->stop( );
    } );

    bindRpcFunc( "SetUpdateInterval", [ this ] ( const RpcRequest& req ) {
        std::chrono::milliseconds newUpdateInterval{ req.param<std::uint32_t>( 0 ) };

        if( newUpdateInterval < 5ms || newUpdateInterval > 5s )
            return;

        updateInterval_ = newUpdateInterval;
    } );
}

int rmcmd::ConsoleProcess::run( ) {
    isRunning_ = true;

    endpoint_ = { asio::ip::address::from_string( "127.0.0.1" ), parentPort_ };
    tcp::resolver resolver{ io_service_ };
    auto addressIt = resolver.resolve( endpoint_ );
    asio::connect( socket_, addressIt );

    readHeader( );

    startUpdateLoop( );

    io_service_.run( );
    isRunning_ = false;
    return 0;
}

void rmcmd::ConsoleProcess::startUpdateLoop( ) {
    if( isRunning( ) ) {
        timer_.expires_from_now( boost::posix_time::milliseconds{ updateInterval_.count( ) } );
        timer_.async_wait( [ this ] ( auto err ) {
            if( !err ) try {
                update( );
                startUpdateLoop( );
            } catch( const std::exception& err ) {
                ::MessageBoxA( NULL, err.what( ),
                               "RemoteConsole", MB_OK | MB_ICONERROR );
            }
        } );
    } else {
        io_service_.stop( );
    }
}

bool rmcmd::ConsoleProcess::isRunning( ) const {
    return isRunning_;
}

void rmcmd::ConsoleProcess::stop( ) {
    isRunning_ = false;
}

void rmcmd::ConsoleProcess::onRequest( RpcRequest&& req ) {
    auto findIt{ rpcMethods_.find( req.methodName( ) ) };
    if( findIt != rpcMethods_.cend( ) ) {
        std::get<OnRequest>( *findIt )( std::move( req ) );
    }
}

void rmcmd::ConsoleProcess::bindRpcFunc( std::string methodName,
                                         OnRequest onRequest ) {
    auto findIt{ rpcMethods_.find( methodName ) };
    if( findIt != rpcMethods_.cend( ) ) throw std::runtime_error{ "Rpc method already exist!" };
    rpcMethods_[ std::move( methodName ) ] = std::move( onRequest );
}

void rmcmd::ConsoleProcess::sendRequest( RpcRequest rpcRequest ) {
    bool writeInProgress{ !writeQueue_.empty( ) };
    writeQueue_.emplace( std::move( rpcRequest ) );

    if( !writeInProgress )
        writeHeader( );
}

void rmcmd::ConsoleProcess::sendRequestInNetworkThread( RpcRequest rpcRequest ) {
    // io_service_.post is thread-safe
    io_service_.post( [ this, req = std::move( rpcRequest ) ]{
        sendRequest( std::move( req ) );
    } );
}

void rmcmd::ConsoleProcess::readHeader( ) {
    asio::async_read( socket_, asio::buffer( headerBuff_, RpcHeader::SIZE ), 
        [ this ]
    ( const boost::system::error_code& err, std::size_t readed ) {
        if( err ) {
            auto msg{ err.message( ) };
            stop( );
        } else {
            try {
                header_ = headerBuff_;
                readBody( );
            } catch( const std::exception& err ) {
                auto what{ ""s + err.what( ) };
                stop( );
            }
        }
    } );
}

void rmcmd::ConsoleProcess::readBody( ) {
    bytes_.resize( header_.bodySize( ) );

    asio::async_read( socket_, asio::buffer( bytes_, header_.bodySize( ) ),
        [ this ]
    ( const boost::system::error_code& err, std::size_t readed ) {
        if( err ) {
            auto msg{ err.message( ) };
            stop( );
        } else {
            try {
                body_ = bytes_;
                auto dump{ body_.getJson( ).dump( 0 ) };
                if( body_.isRequest( ) ) {
                    onRequest( body_.moveToRequest( ) );
                } else { // isResponse
                   // onResponse( body_.moveToResponse( ) );
                }

                readHeader( );
            } catch( const std::exception& err ) {
                ::MessageBoxA( NULL, err.what( ),
                               "RemoteConsole", MB_OK | MB_ICONERROR );
                auto what{ ""s + err.what( ) };
                stop( );
            }
        }
    } );
}

void rmcmd::ConsoleProcess::writeHeader( ) {
    if( !writeQueue_.empty( ) ) {
        try {
            auto& nextRpc{ writeQueue_.front( ) };
            nextRpc.responseIsNotRequired( );
            write_.bodyBytes_ = nextRpc.toBytes( );
            write_.rpcHeader_ = static_cast< RpcHeader::body_size_t >( write_.bodyBytes_.size( ) );
            write_.rpcHeader_.toBuffer( write_.headerBuff_ );
            boost::asio::async_write( socket_, asio::buffer( write_.headerBuff_, RpcHeader::SIZE ),
                                      [ this ]
                                      ( const boost::system::error_code& err, std::size_t writed ) {
                if( err ) {
                    auto msg{ err.message( ) };
                    stop( );
                } else {
                    writeBody( );
                }
            } );
        } catch( const std::exception& err ) {
            ::MessageBoxA( NULL, err.what( ),
                           "RemoteConsole", MB_OK | MB_ICONERROR );
            auto what{ ""s + err.what( ) };
            stop( );
        }
    }
}

void rmcmd::ConsoleProcess::writeBody( ) {
    boost::asio::async_write( socket_, asio::buffer( write_.bodyBytes_ ),
                               [ this ]
                              ( const boost::system::error_code& err, std::size_t writed ) {
        if( err ) {
            auto msg{ err.message( ) };
            stop( );
        } else {
            writeQueue_.pop( );
            writeHeader( );
        }
    } );
}