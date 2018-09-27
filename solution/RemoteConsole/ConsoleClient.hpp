#pragma once

#include "Client.hpp"
#include "ServerSession.hpp"

namespace rmcmd {
    class ConsoleClient final: public Client<ServerSession> {
    public:
        using OnLogin = std::function<void( const boost::system::error_code& )>;

        ConsoleClient( std::string login, std::string password );

        void onSessionStart( std::shared_ptr<ServerSession> session ) override;

        void setOnLoginCallback( OnLogin onLogin );
    private:
        std::string login_, password_;
        OnLogin onLogin_{ [ ] ( auto& ) { } };
    };
}