#include "Console.hpp"

#include <cassert>

#include "boost/asio.hpp"
#include "boost/asio/steady_timer.hpp"
#include "boost/process/extend.hpp"
#include "boost/lexical_cast.hpp"

#include "Session.hpp"

namespace asio = boost::asio;
namespace bp = boost::process;
using namespace std::literals;

rmcmd::Console::Console( std::shared_ptr<boost::asio::strand> strand,
                         Session& session, Type consoleType )
    : strand_{ std::move( strand ) },
    socket_{ strand_->get_io_service( ) },
    acceptor_{ strand_->get_io_service( ),{ boost::asio::ip::tcp::v4( ), 0 } },
    session_{ session },
    consoleType_{ consoleType } {

    JOBOBJECT_EXTENDED_LIMIT_INFORMATION jeli = { 0 };
    jeli.BasicLimitInformation.LimitFlags = JOB_OBJECT_LIMIT_KILL_ON_JOB_CLOSE;

    if( !SetInformationJobObject( hJob, JobObjectExtendedLimitInformation, &jeli, sizeof( jeli ) ) )
        throw std::runtime_error{ "SetInformationJobObject" };

}

rmcmd::Console::~Console( ) {
    stop( );
}

std::uint32_t rmcmd::Console::getConsoleProcessId( ) const {
    return pi.dwProcessId;
}

void rmcmd::Console::start( ) {
    assert( !isRunning_ );
    isRunning_ = true;
    
    startConsoleProcess( );
    //TODO: test local endpoint port
    boost::system::error_code ec;
    acceptor_.accept( socket_, ec );

    // start read cicle
    if( !ec )
        readHeader( );
    else
        stop( );

    sendRequestToConsoleProcess( RpcRequest{ "SetUpdateInterval",
                                 static_cast< std::uint32_t >( updateInterval_.count( ) )
    } );
}

void rmcmd::Console::sendRequestToEndpoint( RpcRequest&& req ) {
    session_.sendRequest( std::move( req ) );
}

void rmcmd::Console::stop( ) {
    if( isRunning_ ) {
        CloseHandle( pi.hThread );
        CloseHandle( pi.hProcess );

        sendRequestToConsoleProcess( RpcRequest{ "Exit" } );


        /* https://stackoverflow.com/q/24012773/5734836
        * At this point, if we are closed, windows will automatically clean up
        * by closing any handles we have open. When the handle to the job object
        * is closed, any processes belonging to the job will be terminated.
        * Note: Grandchild processes automatically become part of the job and
        * will also be terminated. This behaviour can be avoided by using the
        * JOB_OBJECT_LIMIT_SILENT_BREAKAWAY_OK limit flag.
        */
        CloseHandle( hJob );

        isRunning_ = false;
        onStop_( );
    }
}

void rmcmd::Console::sendRequestToConsoleProcess( boost::variant<Rpc, std::vector<std::uint8_t>>&& req ) {
    bool writeInProgress{ !writeQueue_.empty( ) };
    writeQueue_.emplace( std::move( req ) );

    if( !writeInProgress )
        writeHeader( );
}

bool rmcmd::Console::isRunning( ) const {
    return isRunning_;
}

void rmcmd::Console::setOnStopCallback( std::function<void( )> callback ) {
    assert( callback );
    onStop_ = std::move( callback );
}

const std::wstring& rmcmd::Console::getDesktopName( ) {
    static std::wstring desktopName{ [ ] { 
        auto name = ( L"RemoteConsoleDesktop#"
        + boost::lexical_cast< std::wstring >( IdGenerator{ }.generateId( ) ) );

        CreateDesktop( name.c_str( ), 0, 0, 0, GENERIC_ALL, 0 );

        return name;
    } ( ) };

    return desktopName;
}

void rmcmd::Console::startConsoleProcess( ) {
    si.cb = sizeof( si );

    auto exeName{ consoleType_ == Type::client ? L"ConsoleClient.exe" : L"ConsoleServer.exe" };
    
    if( Type::server == consoleType_ )
        si.lpDesktop = &desktopName_[ 0 ];

    std::wostringstream cmdArgslistSetChannel;
    cmdArgslistSetChannel << exeName;
    cmdArgslistSetChannel << L" " << acceptor_.local_endpoint( ).port( );

    std::wstring cmd = cmdArgslistSetChannel.str( );

    if( !CreateProcess( exeName, &cmd[ 0 ], NULL, NULL, FALSE,
                        CREATE_NEW_CONSOLE |
                        CREATE_SUSPENDED | CREATE_BREAKAWAY_FROM_JOB /*Important*/,
                        NULL, NULL, &si, &pi ) )
        throw std::runtime_error{ "CreateProcess" };

    if( !AssignProcessToJobObject( hJob, pi.hProcess ) )
        throw std::runtime_error{ "AssignProcessToJobObject" };

    ResumeThread( pi.hThread );
}

void rmcmd::Console::readHeader( ) {
    asio::async_read( socket_, asio::buffer( headerBuff_, RpcHeader::SIZE ), strand_->wrap(
                      [ this, self = shared_from_this( ) ]
    ( const boost::system::error_code& err, std::size_t readed ) {
        if( err || !isRunning_ ) {
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
    } ) );
}

void rmcmd::Console::readBody( ) {
    bytes_.resize( header_.bodySize( ) );

    asio::async_read( socket_, asio::buffer( bytes_, header_.bodySize( ) ), strand_->wrap(
                      [ this, self = shared_from_this( ) ]
    ( const boost::system::error_code& err, std::size_t readed ) {
        if( err || !isRunning_ ) {
            auto msg{ err.message( ) };
            stop( );
        } else {
            try {
                if( isRunning_ ) {
                    session_.sendRequest( std::move( bytes_ ) );
                    readHeader( );
                }
            } catch( const std::exception& err ) {
                auto what{ ""s + err.what( ) };
                stop( );
            }
        }
    } ) );
}

void rmcmd::Console::writeHeader( ) {
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

            boost::asio::async_write( socket_, asio::buffer( write_.headerBuff_ ),
                                      strand_->wrap( [ this, self = shared_from_this( ) ]
                                      ( const boost::system::error_code& err, std::size_t writed ) {
                if( err ) {
                    auto msg{ err.message( ) };
                    stop( );
                } else {
                    writeBody( );
                }
            } ) );
        } catch( const std::exception& err ) {
            auto what{ ""s + err.what( ) };
            stop( );
        }
    }
}

void rmcmd::Console::writeBody( ) {
    boost::asio::async_write( socket_, asio::buffer( write_.bodyBytes_ ),
                              strand_->wrap( [ this, self = shared_from_this( ) ]
                              ( const boost::system::error_code& err, std::size_t writed ) {
        if( err ) {
            auto msg{ err.message( ) };
            stop( );
        } else {
            writeQueue_.pop( );
            writeHeader( );
        }
    } ) );
}

void rmcmd::Console::setUpdateInterval( const std::chrono::milliseconds& updateInterval ) {
    if( updateInterval < 5ms || updateInterval > 5s )
        return;
    updateInterval_ = updateInterval;
}