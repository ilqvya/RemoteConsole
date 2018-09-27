#pragma once

#include <memory>
#include <string>
#include <functional>
#include "boost/system/error_code.hpp"
#include <chrono>

namespace rmcmd {
    // Do not use headers which is fails with C++\CLI
    class ConsoleServerCLI final {
    public:
        using OnFinish = std::function<void( const boost::system::error_code& )>;
        using OnDisconnect = OnFinish;
        using OnSessionStart = std::function<void( const std::string& address,
                                                   std::uint16_t port )>;

        using OnSessionLogin = std::function<void( const boost::system::error_code& isOk,
                                                   const std::string& address,
                                                   std::uint16_t port,
                                                   std::uint32_t consoleProcessId )>;

        ConsoleServerCLI( const std::string login,
                          const std::string password );

        ~ConsoleServerCLI( ); // required for impl_

        void createServer( unsigned short port, OnFinish onFinish );

        bool isConnected( ) const;

        void setOnDisconnectCallback( OnDisconnect onDisconnect );

        void setOnSessionStartCallback( OnSessionStart onDisconnect );

        void setOnSessionEndCallback( OnSessionStart onDisconnect );

        void setOnSessionLoginCallback( OnSessionLogin callback );

        void disconnect( );

        void setUpdateInterval( const std::chrono::milliseconds& updateInterval );
    private:
        struct Impl;
        std::unique_ptr<Impl> impl_;
    };
}