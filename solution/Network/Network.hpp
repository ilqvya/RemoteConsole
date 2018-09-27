#pragma once

#include <memory>
#include <functional>
#include <atomic>

#include "boost/asio.hpp"
#include "boost/asio/ssl.hpp"
#include <thread>

namespace rmcmd {
    class Network {
    public:
        using OnFinish = std::function<void( const boost::system::error_code& )>;
        using OnDisconnect = OnFinish;

        Network( );

        // disconnect if isConnected_
        virtual ~Network( );

        bool isConnected( ) const;

        void setOnDisconnectCallback( OnDisconnect onDisconnect );

        virtual void disconnect( );

        void join( );
    protected:
        void startNetworkThread( );
    protected:
        std::atomic_bool isConnected_{ false };
        OnDisconnect onDisconnect_{ [ ] ( auto& ) { } };

        std::unique_ptr<boost::asio::io_service> io_service_;

        std::vector<std::thread> threads_;
    };
}