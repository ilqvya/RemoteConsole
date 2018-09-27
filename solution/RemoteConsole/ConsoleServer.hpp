#pragma once

#include "Server.hpp"
#include "ClientSession.hpp"
#include <array>

namespace rmcmd {
    class ConsoleServer final: public Server<ClientSession> {
    public:
        ConsoleServer( std::string login, std::string password );

        void onSessionStart( std::shared_ptr<ClientSession> session ) override;
    private:
        std::string login_;
        std::vector<char> passwordHash_;
    };
}