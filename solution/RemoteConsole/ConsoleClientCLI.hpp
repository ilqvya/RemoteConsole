#pragma once

#include <memory>
#include <string>
#include <functional>
#include <chrono>
#include "boost/system/error_code.hpp"

namespace rmcmd {
    class ConsoleClientCLI {
    public:
        using OnFinish = std::function<void( const boost::system::error_code& )>;
        using OnDisconnect = OnFinish;
        using OnLogin = OnFinish;

        using OnSessionStart = std::function<void( const std::string& address,
                                                   std::uint16_t port )>;

        using OnSessionLogin = std::function<void( const boost::system::error_code& isOk,
                                                   const std::string& address,
                                                   std::uint16_t port,
                                                   std::uint32_t consoleProcessId )>;

        ConsoleClientCLI( const std::string login,
                          const std::string password );

        ~ConsoleClientCLI( ); // required for impl_

        void connectToServer(
            const std::string& address,
            std::uint16_t port,
            OnFinish onFinish );

        bool isConnected( ) const;

        void setOnDisconnectCallback( OnDisconnect onDisconnect );

        void setOnSessionStartCallback( OnSessionStart onDisconnect );

        void setOnSessionEndCallback( OnSessionStart onDisconnect );

        void setOnSessionLoginCallback( OnSessionLogin callback );

        void setOnLoginCallback( OnLogin onLogin );

        void disconnect( );

        void setUpdateInterval( const std::chrono::milliseconds& updateInterval );
    private:
        struct Impl;
        std::unique_ptr<Impl> impl_;
    };
}
