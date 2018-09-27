#include "ConsoleClientCLI.hpp"
#include "ConsoleClient.hpp"

struct rmcmd::ConsoleClientCLI::Impl final {
    Impl( const std::string login,
          const std::string password )
        :consoleServer_{ login, password } { }

    ConsoleClient consoleServer_;
};

rmcmd::ConsoleClientCLI::ConsoleClientCLI( const std::string login,
                                           const std::string password )
    : impl_{ std::make_unique<Impl>( std::move( login ), std::move( password ) ) } { }

rmcmd::ConsoleClientCLI::~ConsoleClientCLI( ) = default;

void rmcmd::ConsoleClientCLI::connectToServer( const std::string& address,
                                               std::uint16_t port,
                                               OnFinish onFinish ) {
    impl_->consoleServer_.connectToServer( address, port, std::move( onFinish ) );
}

bool rmcmd::ConsoleClientCLI::isConnected( ) const {
    return impl_->consoleServer_.isConnected( );
}

void rmcmd::ConsoleClientCLI::setOnDisconnectCallback( OnDisconnect onDisconnect ) {
    impl_->consoleServer_.setOnDisconnectCallback( std::move( onDisconnect ) );
}

void rmcmd::ConsoleClientCLI::setOnSessionStartCallback( OnSessionStart onSessionStart ) {
    impl_->consoleServer_.setOnSessionStartInfoCallback( std::move( onSessionStart ) );
}

void rmcmd::ConsoleClientCLI::setOnSessionEndCallback( OnSessionStart onSessionStart ) {
    impl_->consoleServer_.setOnSessionEndInfoCallback( std::move( onSessionStart ) );
}

void rmcmd::ConsoleClientCLI::setOnSessionLoginCallback( OnSessionLogin callback ) {
    impl_->consoleServer_.setOnSessionLoginCallback( std::move( callback ) );
}

void rmcmd::ConsoleClientCLI::setOnLoginCallback( OnLogin onLogin ) {
    impl_->consoleServer_.setOnLoginCallback( std::move( onLogin ) );
}

void rmcmd::ConsoleClientCLI::disconnect( ) {
    impl_->consoleServer_.disconnect( );
}

void rmcmd::ConsoleClientCLI::setUpdateInterval( const std::chrono::milliseconds& updateInterval ) {
    impl_->consoleServer_.setUpdateInterval( updateInterval );
}