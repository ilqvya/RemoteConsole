#pragma once

#include <chrono>
#include <memory>
#include <map>
#include <queue>
#include <functional>
#include <chrono>
#include "RpcRequest.hpp"
#include "RpcHeader.hpp"
#include "boost/asio.hpp"
#include "boost/asio/strand.hpp"
#include "boost/asio/steady_timer.hpp"
#include "boost/lexical_cast.hpp"
#include "boost/variant.hpp"
#include "IdGenerator.hpp"
#include "Windows.h"

namespace rmcmd {
    class Session;

    // Holds all common stuff of Client and Server consoles
    class Console final: public std::enable_shared_from_this<Console> {
    public:
        using OnRequest = std::function<void( const RpcRequest& )>;

        enum class Type: std::uint8_t { server, client };

        Console( std::shared_ptr<boost::asio::strand> strand,
                 Session& session, Type consoleType );

        // Set isRunning_ to false and whait for consoleThread_ exit
        ~Console( );

        // Start new thread update loop
        void start( );

        std::uint32_t getConsoleProcessId( ) const;

        // Invokes at destructor and before spawn exit
        // set isRunning flag to false
        void stop( );

        // true if console process is running
        bool isRunning( ) const;

        void setOnStopCallback( std::function<void( )> callback );

        void sendRequestToConsoleProcess( boost::variant<Rpc, std::vector<std::uint8_t>>&& req );

        void setUpdateInterval( const std::chrono::milliseconds& updateInterval );
    private:
        void startConsoleProcess( );

        void sendRequestToEndpoint( RpcRequest&& req );
    private:
        std::queue<boost::variant<Rpc, std::vector<std::uint8_t>>> writeQueue_;
        std::shared_ptr<boost::asio::strand> strand_;
        boost::asio::ip::tcp::socket socket_;
        boost::asio::ip::tcp::acceptor acceptor_;
    private:
        bool isRunning_{ false };

        Session& session_;

        std::function<void( )> onStop_{ [ ] { } };

        const Type consoleType_;
    private:
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
        HANDLE hJob = CreateJobObject( NULL, NULL );

        static const std::wstring& getDesktopName( );
        std::wstring desktopName_ = getDesktopName( );

        PROCESS_INFORMATION                  pi = { 0 };
        STARTUPINFO                          si = { 0 };
    private:
        std::chrono::milliseconds updateInterval_{ 50 };
    };
}