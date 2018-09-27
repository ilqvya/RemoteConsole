#include "catch.hpp"
#include "RpcRequest.hpp"
#include "RpcResponse.hpp"

TEST_CASE( "RpcRequest Basic", "[RpcRequest]" ) {
    rmcmd::RpcRequest rpc{ "Method", 10, -1 };
    std::size_t argNum{ 0u };

    REQUIRE( "Method" == rpc.methodName( ) );
    REQUIRE( 2 == rpc.paramsSize( ) );

    REQUIRE( 10 == rpc.param<int>( argNum++ ) );
    REQUIRE( -1 == rpc.param<int>( argNum++ ) );
}

TEST_CASE( "String and numbers", "[RpcRequest]" ) {
    rmcmd::RpcRequest rpc{ "Method2", 10, "TEST", 1.231 };
    std::size_t argNum{ 0u };

    REQUIRE( "Method2" == rpc.methodName( ) );
    REQUIRE( 3 == rpc.paramsSize( ) );

    REQUIRE( 10 == rpc.param<int>( argNum++ ) );
    REQUIRE( "TEST" == rpc.param<std::string>( argNum++ ) );
    REQUIRE( 1.231 == rpc.param<double>( argNum++ ) );
}

TEST_CASE( "RpcRequest Vector", "[RpcRequest]" ) {
    rmcmd::RpcRequest rpc{ "Method", "TEST", std::vector<int>{ { 1, 2, 3 } } };
    std::size_t argNum{ 0u };

    REQUIRE( "Method" == rpc.methodName( ) );
    REQUIRE( 2 == rpc.paramsSize( ) );

    REQUIRE( "TEST" == rpc.param<std::string>( argNum++ ) );

    std::vector<int> required{ { 1, 2, 3 } };
    const auto result = rpc.param<std::vector<int>>( argNum++ );
    REQUIRE( required == result );
}

TEST_CASE( "RpcRequest Map", "[RpcRequest]" ) {
    std::map<std::string, int> c_map{ { "one", 1 },{ "two", 2 },{ "three", 3 } };
    rmcmd::RpcRequest rpc{ "Method", "TEST", c_map };
    std::size_t argNum{ 0u };

    REQUIRE( "Method" == rpc.methodName( ) );
    REQUIRE( 2 == rpc.paramsSize( ) );

    REQUIRE( "TEST" == rpc.param<std::string>( argNum++ ) );

    const auto result = rpc.param<std::map<std::string, int>>( argNum++ );
    REQUIRE( c_map == result );
}

TEST_CASE( "RpcRequest Zero params", "[RpcRequest]" ) {
    rmcmd::RpcRequest rpc{ std::string{"Method"} };

    REQUIRE( "Method" == rpc.methodName( ) );
    REQUIRE( 0 == rpc.paramsSize( ) );
}

TEST_CASE( "makeResponse test", "[RpcRequest]" ) {
    rmcmd::RpcRequest req{ "Method" };

    REQUIRE( "Method" == req.methodName( ) );
    REQUIRE( 0 == req.paramsSize( ) );

    auto res = req.makeResponse( true, "hi" );

    REQUIRE( 2 == res.resultSize( ) );
    REQUIRE( req.getId( ) == res.getId( ) );
    REQUIRE( true == res.result<bool>( 0 ) );
    REQUIRE( "hi" == res.result<std::string>( 1 ) );
}

TEST_CASE( "makeResponseError test", "[RpcRequest]" ) {
    using namespace rmcmd;
    rmcmd::RpcRequest req{ "Method" };
    REQUIRE( "Method" == req.methodName( ) );
    REQUIRE( 0 == req.paramsSize( ) );

    auto res = req.makeResponseError( RpcResponse::ErrorCode::methodNotFound, "ErrMsg" );

    REQUIRE( res.isError( ) );
    REQUIRE( req.getId( ) == res.getId( ) );
    REQUIRE( res.errorCode( ) == RpcResponse::ErrorCode::methodNotFound );
    REQUIRE( "ErrMsg" == res.errorMessage( ) );
}