#define CATCH_CONFIG_MAIN  // This tells Catch to provide a main() - only do this in one cpp file
#include "catch.hpp"

#include "Server.hpp"
#include "Client.hpp"
#include "Session.hpp"

#include <future>
#include <chrono>

using namespace rmcmd;
using namespace std::literals;

TEST_CASE( "Connection test", "Network" ) {
    const auto localhost{ "127.0.0.1"s };

    const uint16_t port{ 8081u };

    Server<Session> server;
    Client<Session> client;
    
    std::promise<bool> promise;
    auto testResult = promise.get_future( );

    server.createServer( port, [ & ] ( auto& error ) {
        REQUIRE( !error );
    } );
    

    client.connectToServer( localhost, port, [ & ] ( auto& error ) {
        REQUIRE( !error );

        promise.set_value( true );
    } );

    
    if( std::future_status::ready == testResult.wait_for( 10s ) ) {
        REQUIRE( testResult.get( ) );
    } else {
        REQUIRE( false );
    }
}

TEST_CASE( "Request echo", "Network" ) {
    const auto localhost{ "127.0.0.1"s };
    static const auto echo{ "echo"s };

    const uint16_t port{ 8081u };

    static std::promise<bool> promise;

    class ServerSession final: public Session {
    public:
        using Session::Session;

        void start( ) override {
            Session::start( );

            sendRequest( { echo, echo }, [ this ] ( const RpcResponse& res ) {
                auto resStr = res.result<std::string>( 0 );

                REQUIRE( resStr == echo );

                promise.set_value( true );
                disconnect( );
            } );
        }
    };

    class ClientSession final: public Session {
    public:
        using Session::Session;

        void start( ) override {
            bindRpcFunc( echo, [ this ] ( const RpcRequest& req ) {
                return req.makeResponse( req.param<std::string>( 0 ) );
            } );
            Session::start( );
        }
    };

    Server<ServerSession> server;
    Client<ClientSession> client;

    auto testResult = promise.get_future( );

    server.createServer( port, [ & ] ( auto& error ) {
        REQUIRE( !error );
    } );

    client.connectToServer( localhost, port, [ & ] ( auto& error ) {
        REQUIRE( !error );
    } );

    if( std::future_status::ready == testResult.wait_for( 10s ) ) {
        REQUIRE( testResult.get( ) );
    } else {
        REQUIRE( false );
    }
}

TEST_CASE( "Request echo by onSessionStart callback", "Network" ) {
    const auto localhost{ "127.0.0.1"s };
    static const auto echo{ "echo"s };

    const uint16_t port{ 8081u };

    static std::promise<bool> promise;

    Server<Session> server;
    Client<Session> client;

    server.setOnSessionStartCallback( [ ] ( std::shared_ptr<Session> session ) {
        session->bindRpcFunc( echo, [ ] ( const RpcRequest& req ) {
            return req.makeResponse( req.param<std::string>( 0 ) );
        } );
    } );

    client.setOnSessionStartCallback( [ ] ( std::shared_ptr<Session> session ) {
        session->sendRequest( { echo, echo }, [ ] ( const RpcResponse& res ) {
            auto echoRes = res.result<std::string>( 0 );

            REQUIRE( echoRes == echo );

            promise.set_value( true );
        } );
    } );

    auto testResult = promise.get_future( );

    server.createServer( port, [ & ] ( auto& error ) {
        REQUIRE( !error );
    } );

    client.connectToServer( localhost, port, [ & ] ( auto& error ) {
        REQUIRE( !error );
    } );

    if( std::future_status::ready == testResult.wait_for( 10s ) ) {
        REQUIRE( testResult.get( ) );
    } else {
        REQUIRE( false );
    }
}