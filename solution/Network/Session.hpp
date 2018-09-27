#pragma once

#include "Rpc.hpp"
#include "RpcRequest.hpp"
#include "RpcResponse.hpp"
#include "RpcHeader.hpp"
#include "IdGenerator.hpp"

#include "boost/asio.hpp"
#include "boost/asio/ssl.hpp"
#include "boost/functional/hash.hpp"
#include "boost/variant.hpp"

#include <unordered_map>
#include <map>
#include <memory>
#include <queue>
#include <chrono>

namespace rmcmd {
    class Console;
    // Example: http://www.boost.org/doc/libs/1_65_1/doc/html/boost_asio/example/cpp11/chat/chat_server.cpp

    // Encapsulates work with sockets
    class Session: public std::enable_shared_from_this<Session> {
    public:
        using OnResponse = std::function<void( const RpcResponse& )>;
        using OnRequest = std::function<RpcResponse( const RpcRequest& )>;
        using OnSessionEnd = std::function<void( const std::string& address,
                                                       std::uint16_t port )>;

        using OnSessionLogin = std::function<void( const boost::system::error_code& isOk,
                                                   const std::string& address,
                                                   std::uint16_t port,
                                                   std::uint32_t consoleProcessId )>;
    public:
        virtual ~Session( );

        explicit Session( std::unique_ptr<boost::asio::ssl::stream<boost::asio::ip::tcp::socket>> socket );

        // Start read messages loop
        virtual void start( );

        // Response do not required
        void sendRequest( RpcRequest rpcRequest );

        void sendRequest( RpcRequest rpcRequest,
                          OnResponse onResponse );

        void sendRequest( std::vector<std::uint8_t>&& rpcRequestBytes );

        void sendRequestThreadSafe( RpcRequest rpcRequest,
                                    OnResponse onResponse = [ ] ( auto& ) { } );

        void bindRpcFunc( std::string funcName, // By val for move
                          OnRequest onRequest );

        // close socket
        void disconnect( );

        void setOnSessionEndCallback( OnSessionEnd callback );
        void setOnSessionLoginCallback( OnSessionLogin callback );

        void setUpdateInterval( const std::chrono::milliseconds& updateInterval );
    protected:
        virtual void onRequest( RpcRequest&& rpcRequest );

        // Make thread-safe function invokation in this session thread
        void invokeInThisThread( std::function<void( )> callback );
    private:
        void onResponse( RpcResponse&& rpcResponse );

        // add rpc to a writeQueue_ and invoke doSend if the queue was empty (i.e. we aren't writting now)
        void send( Rpc&& rpc );

        void send( std::vector<std::uint8_t>&& rpc );

        // start new spawn
        void doSend( );

        // return underlying tcp socket
        boost::asio::ssl::stream<boost::asio::ip::tcp::socket>::lowest_layer_type& socket( );
    protected:
        std::unique_ptr<boost::asio::ssl::stream<boost::asio::ip::tcp::socket>> socket_;
    protected:
        std::shared_ptr<boost::asio::io_service::strand> strand_;

        std::shared_ptr<Console> console_;
    private:
        std::queue<boost::variant<Rpc, std::vector<std::uint8_t>>> writeQueue_;
    
        std::unordered_map<boost::uuids::uuid, OnResponse,
                           boost::hash<boost::uuids::uuid>
        > sentRequests_;

        std::map<std::string, OnRequest> rpcMethods_;
    private:
        std::vector<std::uint8_t> bytes_;
        RpcHeader::buffer_t headerBuff_;
        RpcHeader header_;
        Rpc body_;

        void readHeader( );
        void readBody( );
    private:
        struct Write {
            std::vector<std::uint8_t> bodyBytes_;
            RpcHeader rpcHeader_;
            RpcHeader::buffer_t headerBuff_;
        } write_;

        void writeHeader( );
        void writeBody( );
    protected:
        bool isAuthorized_{ false };
    protected:
        OnSessionEnd onSessionEnd_{ [ ] ( auto... ) { } };
        OnSessionLogin onSessionLogin_{ [ ] ( auto... ) { } };
        std::string remoteAddress_;
        std::uint16_t remotePort_{ 0u };
    protected:
        std::chrono::milliseconds updateInterval_{ 50 };
    };
}