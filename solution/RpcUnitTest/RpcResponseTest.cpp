#include "catch.hpp"
#include "RpcResponse.hpp"
#include <iostream>

using namespace rmcmd;

TEST_CASE( "BasicResponse", "[RpcResponse]" ) {
    IdGenerator idGenerator;
    auto id = idGenerator.generateId( );
    rmcmd::RpcResponse rpc{ id, boost::none, 10, "hi" };
    std::size_t resNum{ 0u };

    REQUIRE( !rpc.isError( ) );
    REQUIRE( id == rpc.getId( ) );

    REQUIRE( 2 == rpc.resultSize( ) );
    REQUIRE( 10 == rpc.result<int>( resNum++ ) );
    REQUIRE( "hi" == rpc.result<std::string>( resNum++ ) );
}

TEST_CASE( "BasicResponseError", "[RpcResponse]" ) {
    IdGenerator idGenerator;
    auto id = idGenerator.generateId( );
    rmcmd::RpcResponse rpc{ id,{ {rmcmd::RpcResponse::ErrorCode::serverError, "ERROR" }} };

    REQUIRE( rpc.isError( ) );
    REQUIRE( id == rpc.getId( ) );

    REQUIRE( rmcmd::RpcResponse::ErrorCode::serverError == rpc.errorCode( ) );
    REQUIRE( "ERROR" == rpc.errorMessage( ) );

    REQUIRE( 0 == rpc.resultSize( ) );
}

TEST_CASE( "RpcResponse Zero Results", "[RpcResponse]" ) {
    IdGenerator idGenerator;
    auto id = idGenerator.generateId( );
    rmcmd::RpcResponse rpc{ id, { } };

    REQUIRE( !rpc.isError( ) );
    REQUIRE( id == rpc.getId( ) );

    REQUIRE( 0 == rpc.resultSize( ) );
}
