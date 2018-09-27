#define CATCH_CONFIG_MAIN  // This tells Catch to provide a main() - only do this in one cpp file
#include "catch.hpp"

#include "ConsoleClient.hpp"
#include "ConsoleServer.hpp"

#include <future>
#include <chrono>

using namespace std::literals;
using namespace rmcmd;

// Process terminates before output result.

//TEST_CASE( "RemoteConsole login test", "RemoteConsole" ) {
//    const auto localhost{ "127.0.0.1"s };
//    const auto login{ "login" }, password{ "password" };
//
//    const uint16_t port{ 8081u };
//
//    static std::promise<bool> promise;
//    auto testResult = promise.get_future( );
//
//    ConsoleServer server{ login, password };
//    ConsoleClient client{ login, password };
//
//    server.setOnSessionStartCallback( [ ] ( auto session ) {
//        session->disableConsole( );
//    } );
//
//    client.setOnSessionStartCallback( [ ] ( auto session ) {
//        session->disableConsole( );
//    } );
//
//    client.setOnLoginCallback( [ ] ( const boost::system::error_code& error ) {
//        promise.set_value( !error );
//        REQUIRE( !error );
//    } );
//
//    server.createServer( port, [ & ] ( auto& error ) {
//        REQUIRE( !error );
//    } );
//
//    client.connectToServer( localhost, port, [ & ] ( auto& error ) {
//        REQUIRE( !error );
//    } );
//
//    auto st = testResult.wait_for( 5s );
//    if( std::future_status::ready == st ) {
//        REQUIRE( testResult.get( ) );
//    } else {
//        REQUIRE( false );
//    }
//}

//TEST_CASE( "RemoteConsole login fail test", "RemoteConsole" ) {
//    const auto localhost{ "127.0.0.1"s };
//    const auto login{ "login" }, password{ "password" };
//
//    const uint16_t port{ 8081u };
//
//    static std::promise<bool> promise;
//    auto testResult = promise.get_future( );
//
//    ConsoleServer server{ login, password };
//    ConsoleClient client{ password, login };
//
//    client.setOnLoginCallback( [ ] ( const boost::system::error_code& error ) {
//        promise.set_value( error );
//        REQUIRE( error );        
//    } );
//
//    server.createServer( port, [ & ] ( auto& error ) {
//        REQUIRE( !error );
//    } );
//
//    client.connectToServer( localhost, port, [ & ] ( auto& error ) {
//        REQUIRE( !error );
//    } );
//
//    auto st = testResult.wait_for( 5s );
//    if( std::future_status::ready == st ) {
//        REQUIRE( testResult.get( ) );
//    } else {
//        REQUIRE( false );
//    }
//}