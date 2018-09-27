#include "ClientSession.hpp"
#include "Console.hpp"
#include "Utils.hpp" // Binary serialization of global structures
#include "argon2.h"

using namespace std::literals;
namespace errc = boost::system::errc;

rmcmd::ClientSession::~ClientSession( ) {
    if( console_ )
        console_->stop( );
}

void rmcmd::ClientSession::setLogin( std::string login ) {
    login_ = std::move( login );
}

void rmcmd::ClientSession::setPasswordHash( std::vector<char> passwordHash ) {
    passwordHash_ = std::move( passwordHash );
}

void rmcmd::ClientSession::disableConsole( ) {
    startConsole_ = false;
}

void rmcmd::ClientSession::start( ) {
    console_ = std::make_shared<Console>( strand_, *this, Console::Type::server );
    console_->setUpdateInterval( updateInterval_ );
    initRpcFunctions( );

    console_->setOnStopCallback( [ this ] {
        disconnect( );
    } );

    Session::start( );
}

void rmcmd::ClientSession::initRpcFunctions( ) {
    bindRpcFunc( "login", [ this ] ( const RpcRequest& req ) {
        auto password = req.param<std::string>( 1 );
        if( req.param<std::string>( 0 ) == login_ &&
            ARGON2_OK == argon2i_verify( &passwordHash_[ 0 ], &password[ 0 ], password.size( ) ) )
        {
            isAuthorized_ = true;

            if( startConsole_ ) 
                console_->start( );

            onSessionLogin_( errc::make_error_code( errc::success ), remoteAddress_, remotePort_,
                             console_->getConsoleProcessId( ) );

            return req.makeResponse( true );;
        }

        onSessionLogin_( errc::make_error_code(errc::invalid_argument ),
                         remoteAddress_, remotePort_, 0 );
        return req.makeResponse( false );
    } );

    bindRpcFunc( "permission_denied", [ this ] ( const RpcRequest& req ) {
        return req.makeResponseError( RpcResponse::ErrorCode::serverError,
                                      "permission denied" );
    } );

    bindRpcFunc( "echo", [ this ] ( const RpcRequest& req ) {
        return req.makeResponse( req.param<std::string>( 0 ) );
    } );
}

void rmcmd::ClientSession::onRequest( RpcRequest&& rpcRequest ) {
    if( !isAuthorized_ && rpcRequest.methodName( ) != "login" ) {
        rpcRequest.setMethodName( "permission_denied" );
    }

    Session::onRequest( std::move( rpcRequest ) );
}
