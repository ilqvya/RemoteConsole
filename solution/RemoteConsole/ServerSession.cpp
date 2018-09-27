#include "ServerSession.hpp"
#include "Console.hpp"
#include "Utils.hpp" // Binary serialization of global structures

namespace errc = boost::system::errc;

rmcmd::ServerSession::~ServerSession( ) {
    if( console_ )
        console_->stop( );
}

void rmcmd::ServerSession::setLogin( std::string login ) {
    login_ = std::move( login );
}

void rmcmd::ServerSession::setPassword( std::string password ) {
    password_ = std::move( password );
}

void rmcmd::ServerSession::disableConsole( ) {
    startConsole_ = false;
}

void rmcmd::ServerSession::start( ) {
    remoteAddress_ = socket_->lowest_layer( ).local_endpoint( ).address( ).to_string( );
    remotePort_ = socket_->lowest_layer( ).local_endpoint( ).port( );

    console_ = std::make_shared<Console>( strand_, *this, Console::Type::client );
    console_->setUpdateInterval( updateInterval_ );

    console_->setOnStopCallback( [ this ] {
        disconnect( );
    } );

    Session::start( );

    sendLoginRequest( );
}

void rmcmd::ServerSession::setOnLoginCallback( OnLogin onLogin ) {
    assert( onLogin );
    onLogin_ = std::move( onLogin );
}

void rmcmd::ServerSession::sendLoginRequest( ) {
    RpcRequest rpcRequest{ "login", login_, password_ };

    sendRequest( std::move( rpcRequest ), [ this ] ( const RpcResponse& res ) {
        auto err = errc::make_error_code( boost::system::errc::invalid_argument );
        if( !res.isError( ) && res.result<bool>( 0 ) ) {
            err = errc::make_error_code( errc::success );
        }

        onLogin_( err );

        if( err ) {
            onSessionLogin_( err, remoteAddress_, remotePort_, 0 );
            disconnect( );
        } else {
            isAuthorized_ = true;
            if( startConsole_ )
                console_->start( );
            onSessionLogin_( err, remoteAddress_, remotePort_, console_->getConsoleProcessId( ) );
        }
    } );
}
