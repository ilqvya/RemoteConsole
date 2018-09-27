#pragma once
#include "Session.hpp"

#include "boost/system/error_code.hpp"

namespace rmcmd {
    // Do work with cmdProcess
    class ServerSession final: public Session {
    public:
        using OnLogin = std::function<void( const boost::system::error_code& )>;

        using Session::Session; // use parent constructors

        // Stop console
        ~ServerSession( ) override;

        void setLogin( std::string login );
        void setPassword( std::string password );
        void disableConsole( ); // use it for unit test

        void start( ) override; // Call server 'login' rpc function
    public:
        void setOnLoginCallback( OnLogin onLogin );
    private:
        void sendLoginRequest( );
    private:
        bool startConsole_{ true };
        std::string login_, password_;

        OnLogin onLogin_{ [ ] ( auto& ) { } };
    };
}

