#pragma once

#include "Session.hpp"

#include <string>

namespace rmcmd {
    // Do work with cmdProcess
    class ClientSession final: public Session {
    public:
        using Session::Session; // use parent constructors

        // Stop console process
        ~ClientSession( ) override;

        void setLogin( std::string login );
        void setPasswordHash( std::vector<char> passwordHash );
        void disableConsole( );  // use it for unit test

        void start( ) override;
    private:
        void initRpcFunctions( );

        // Prevent actions if isAuthorized_ is false
        void onRequest( RpcRequest&& rpcRequest ) override;
    private:
        // Warning: clientConsole_ will call this methods from own thread.
        // Do not forget make it thread safe by invokeInThisThread method

    private:
        bool startConsole_{ true };
        std::string login_;
        std::vector<char> passwordHash_;
    };
}

