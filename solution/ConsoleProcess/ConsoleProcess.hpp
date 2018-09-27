#pragma once

#include <string>
#include <map>
#include <functional>
#include <queue>
#include <chrono>
#include "Rpc.hpp"
#include "RpcHeader.hpp"
#include "RpcRequest.hpp"
#include "boost/asio.hpp"
#include "Windows.h"

namespace rmcmd {
    class RpcRequest;

    class ConsoleProcess {
    public:
        using OnRequest = std::function<void( const RpcRequest& )>;

        ConsoleProcess( std::uint16_t parentPort );

        virtual ~ConsoleProcess( ) = default;

        // start console update loop
        int run( );

        // Check isRunning_ flag and parent process life
        virtual bool isRunning( ) const;

        void stop( );

        void bindRpcFunc( std::string methodName, OnRequest onRequest );

        void sendRequest( RpcRequest rpcRequest );

        void sendRequestInNetworkThread( RpcRequest rpcRequest );
    protected:
        virtual void update( ) = 0;
    private:
        void onRequest( RpcRequest&& req );
    private:
        std::queue<Rpc> writeQueue_;
        std::map<std::string, OnRequest> rpcMethods_;
        bool isRunning_{ false };
    private:
        std::uint16_t parentPort_{ 0 };
        boost::asio::io_service io_service_;
        boost::asio::ip::tcp::socket socket_;
    private:
        boost::asio::ip::tcp::endpoint endpoint_;
        boost::asio::ip::tcp::resolver resolver_{ io_service_ };

        boost::asio::deadline_timer timer_{ io_service_ };
        void startUpdateLoop( );

        std::vector<std::uint8_t> bytes_;
        RpcHeader::buffer_t headerBuff_;
        RpcHeader header_;
        Rpc body_;

        void readHeader( );
        void readBody( );

        struct Write {
            std::vector<std::uint8_t> bodyBytes_;
            RpcHeader rpcHeader_;
            RpcHeader::buffer_t headerBuff_;
        } write_;

        void writeHeader( );
        void writeBody( );
    private:
        std::chrono::milliseconds updateInterval_{ 50 };
    };
}