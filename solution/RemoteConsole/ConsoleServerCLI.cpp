#include "ConsoleServerCLI.hpp"
#include "ConsoleServer.hpp"

struct rmcmd::ConsoleServerCLI::Impl final {
    Impl( const std::string login,
          const std::string password )
        :consoleServer_{ login, password } { }
    
    ConsoleServer consoleServer_;
};

rmcmd::ConsoleServerCLI::ConsoleServerCLI( const std::string login,
                                           const std::string password ) 
    : impl_{ std::make_unique<Impl>( std::move( login ), std::move( password ) ) } { }

rmcmd::ConsoleServerCLI::~ConsoleServerCLI( ) = default;

void rmcmd::ConsoleServerCLI::createServer( unsigned short port, OnFinish onFinish ) {
    impl_->consoleServer_.createServer( port, std::move( onFinish ) );
}

bool rmcmd::ConsoleServerCLI::isConnected( ) const {
    return impl_->consoleServer_.isConnected( );
}

void rmcmd::ConsoleServerCLI::setOnDisconnectCallback( OnDisconnect onDisconnect ) {
    impl_->consoleServer_.setOnDisconnectCallback( std::move( onDisconnect ) );
}

void rmcmd::ConsoleServerCLI::setOnSessionStartCallback( OnSessionStart onSessionStart ) {
    impl_->consoleServer_.setOnSessionStartInfoCallback( std::move( onSessionStart ) );
}

void rmcmd::ConsoleServerCLI::setOnSessionEndCallback( OnSessionStart onSessionStart ) {
    impl_->consoleServer_.setOnSessionEndInfoCallback( std::move( onSessionStart ) );
}

void rmcmd::ConsoleServerCLI::setOnSessionLoginCallback( OnSessionLogin callback ) {
    impl_->consoleServer_.setOnSessionLoginCallback( std::move( callback ) );
}

void rmcmd::ConsoleServerCLI::disconnect( ) {
    impl_->consoleServer_.disconnect( );
}

void rmcmd::ConsoleServerCLI::setUpdateInterval( const std::chrono::milliseconds& updateInterval ) {
    impl_->consoleServer_.setUpdateInterval( updateInterval );
}