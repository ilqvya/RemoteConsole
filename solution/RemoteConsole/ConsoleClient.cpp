#include "ConsoleClient.hpp"

rmcmd::ConsoleClient::ConsoleClient( std::string login, std::string password )
    : login_{ std::move( login ) }, password_{ std::move( password ) } {
}

void rmcmd::ConsoleClient::onSessionStart( std::shared_ptr<ServerSession> session ) {
    session->setLogin( login_ );
    session->setPassword( password_ );
    session->setOnLoginCallback( onLogin_ );
}

void rmcmd::ConsoleClient::setOnLoginCallback( OnLogin onLogin ) {
    assert( onLogin );
    onLogin_ = std::move( onLogin );
}