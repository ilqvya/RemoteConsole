#define CATCH_CONFIG_MAIN  // This tells Catch to provide a main() - only do this in one cpp file
#include "catch.hpp"
#include "Rpc.hpp"
#include "RpcRequest.hpp"
#include "RpcResponse.hpp"
#include "Utils.hpp"

using nlohmann::json;
using namespace rmcmd;

TEST_CASE( "Valid rpc request", "[Rpc]" ) {
    IdGenerator idGenerator;
    auto id = idGenerator.generateId( );

    json jsonrpc = {
        { "id", boost::lexical_cast<std::string>(id) },
        { "method", "Method" },
        { "params", json::array( ) },
    };

    bool wasException = false;

    try {
        Rpc rpc{ std::move( jsonrpc ) };

        REQUIRE( id == rpc.getId( ) );
        REQUIRE( true == rpc.isRequest( ) );

        auto rpcReq = rpc.moveToRequest( );
        REQUIRE( 0 == rpc.getJson( ).size( ) );

        REQUIRE( 0 == rpcReq.paramsSize( ) );
        REQUIRE( "Method" == rpcReq.methodName( ) );
    } catch( const Rpc::Exception& /*err*/ ) {
        wasException = true;
    }

    REQUIRE( !wasException );
}

TEST_CASE( "Valid rpc toBytes", "[Rpc]" ) {
    IdGenerator idGenerator;
    auto id = idGenerator.generateId( );

    json jsonrpc = {
        { "id", boost::lexical_cast<std::string>( id ) },
        { "method", "Method" },
        { "params", json::array( ) },
    };

    bool wasException = false;

    try {
        Rpc rpc{ std::move( jsonrpc ) };

        auto bytes = rpc.toBytes( );
        rpc = Rpc{ bytes };

        REQUIRE( id == rpc.getId( ) );
        REQUIRE( true == rpc.isRequest( ) );

        auto rpcReq = rpc.moveToRequest( );
        REQUIRE( 0 == rpc.getJson( ).size( ) );

        REQUIRE( 0 == rpcReq.paramsSize( ) );
        REQUIRE( "Method" == rpcReq.methodName( ) );
    } catch( const Rpc::Exception& /*err*/ ) {
        wasException = true;
    }

    REQUIRE( !wasException );
}

TEST_CASE( "Valid rpc response", "[Rpc]" ) {
    IdGenerator idGenerator;
    auto id = idGenerator.generateId( );

    json jsonrpc = {
        { "id", boost::lexical_cast<std::string>( id ) },
        { "result", json::array( ) },
    };

    bool wasException = false;

    try {
        Rpc rpc{ std::move( jsonrpc ) };

        REQUIRE( id == rpc.getId( ) );
        REQUIRE( true == rpc.isResponse( ) );

        auto rpcRes = rpc.moveToResponse( );
        REQUIRE( 0 == rpc.getJson( ).size( ) );

        REQUIRE( 0 == rpcRes.resultSize( ) );
        REQUIRE( !rpcRes.isError( ) );
    } catch( const Rpc::Exception& /*err*/ ) {
        wasException = true;
    }

    REQUIRE( !wasException );
}

TEST_CASE( "Valid rpc response error", "[Rpc]" ) {
    IdGenerator idGenerator;
    auto id = idGenerator.generateId( );


    json jsonrpc = {
        { "id", boost::lexical_cast<std::string>( id ) },
        { "error", {
            { "code", util::toUType( RpcResponse::ErrorCode::internalError ) },
            { "message","Msg" }
        } },
    };

    bool wasException = false;

    try {
        Rpc rpc{ std::move( jsonrpc ) };

        REQUIRE( id == rpc.getId( ) );
        REQUIRE( true == rpc.isResponse( ) );

        auto rpcRes = rpc.moveToResponse( );

        REQUIRE( 0 == rpcRes.resultSize( ) );
        REQUIRE( rpcRes.isError( ) );

        REQUIRE( RpcResponse::ErrorCode::internalError == rpcRes.errorCode( ) );
        REQUIRE( "Msg" == rpcRes.errorMessage( ) );
    } catch( const Rpc::Exception& /*err*/ ) {
        wasException = true;
    }

    REQUIRE( !wasException );
}

TEST_CASE( "Invalid rpc request id", "[Rpc]" ) {
    json jsonrpc = {
        { "method", "Method" },
        { "params", json::array( ) },
    };

    bool wasException = false;

    try {
        Rpc rpc{ std::move( jsonrpc ) };
    } catch( const Rpc::Exception& /*err*/ ) {
        wasException = true;
    }

    REQUIRE( !wasException );
}

TEST_CASE( "Invalid rpc request method", "[Rpc]" ) {
    json jsonrpc = {
        { "id", "Id" },
        //{ "method", "Method" },
        { "params", json::array( ) },
    };

    bool wasException = false;

    try {
        Rpc rpc{ std::move( jsonrpc ) };
    } catch( const Rpc::Exception& /*err*/ ) {
        wasException = true;
    }

    REQUIRE( wasException );
}

TEST_CASE( "Invalid rpc request params", "[Rpc]" ) {
    json jsonrpc = {
        { "id", "Id" },
        { "method", "Method" },
        //{ "params", json::array( ) },
    };

    bool wasException = false;

    try {
        Rpc rpc{ std::move( jsonrpc ) };
    } catch( const Rpc::Exception& /*err*/ ) {
        wasException = true;
    }

    REQUIRE( wasException );
}

TEST_CASE( "Invalid rpc response id", "[Rpc]" ) {
    json jsonrpc = {
       // { "id", "Id" },
        { "result", json::array( ) },
    };

    bool wasException = false;

    try {
        Rpc rpc{ std::move( jsonrpc ) };
    } catch( const Rpc::Exception& /*err*/ ) {
        wasException = true;
    }

    REQUIRE( !wasException );
}

TEST_CASE( "Invalid rpc response result", "[Rpc]" ) {
    json jsonrpc = {
         { "id", "Id" },
        //{ "result", json::array( ) },
    };

    bool wasException = false;

    try {
        Rpc rpc{ std::move( jsonrpc ) };
    } catch( const Rpc::Exception& /*err*/ ) {
        wasException = true;
    }

    REQUIRE( wasException );
}

TEST_CASE( "Invalid rpc empty", "[Rpc]" ) {
    json jsonrpc;

    bool wasException = false;

    try {
        Rpc rpc{ std::move( jsonrpc ) };
    } catch( const Rpc::Exception& /*err*/ ) {
        wasException = true;
    }

    REQUIRE( wasException );
}

TEST_CASE( "Invalid rpc response error id", "[Rpc]" ) {
    json jsonrpc = {
        //{ "id", "Id" },
        { "error",{
            { "code", util::toUType( RpcResponse::ErrorCode::internalError ) },
            { "message","Msg" }
        } },
    };

    bool wasException = false;

    try {
        Rpc rpc{ std::move( jsonrpc ) };
    } catch( const Rpc::Exception& /*err*/ ) {
        wasException = true;
    }

    REQUIRE( !wasException );
}

TEST_CASE( "Invalid rpc response error code", "[Rpc]" ) {
    json jsonrpc = {
        { "id", "Id" },
        { "error",{
            //{ "code", util::toUType( RpcResponse::ErrorCode::internalError ) },
            { "message","Msg" }
        } },
    };

    bool wasException = false;

    try {
        Rpc rpc{ std::move( jsonrpc ) };
    } catch( const Rpc::Exception& /*err*/ ) {
        wasException = true;
    }

    REQUIRE( wasException );
}

TEST_CASE( "Invalid rpc response error message", "[Rpc]" ) {
    json jsonrpc = {
        { "id", "Id" },
        { "error",{
            { "code", util::toUType( RpcResponse::ErrorCode::internalError ) },
            //{ "message","Msg" }
        } },
    };

    bool wasException = false;

    try {
        Rpc rpc{ std::move( jsonrpc ) };
    } catch( const Rpc::Exception& /*err*/ ) {
        wasException = true;
    }

    REQUIRE( wasException );
}

TEST_CASE( "Rpc bytes has only one '\0' at the end", "[Rpc]" ) {
    return; // fail test
    json jsonrpc = {
        { "id", "Id00" },
        { "error",{
            { "code", 0 },
            { "message","Msg\0\0" }
        } },
        { "test", 1 }
    };

    Rpc rpc{ std::move( jsonrpc ) };

    const auto bytes = rpc.toBytes( );

    for( const auto& byte : bytes ) {
        std::cout << std::hex << static_cast< unsigned short >( byte ) << ' ';
    }
    std::cout << std::endl;

    bool hasZeroAtEnd{ 0 == bytes.back( ) };
    bool hasNotZeroNotAtEnd{ std::none_of( cbegin( bytes ), cend( bytes ), // ADL http://en.cppreference.com/w/cpp/language/adl
                                           [ ] ( const auto& byte ) { return 0 == byte; } ) };

    REQUIRE( hasZeroAtEnd );
    REQUIRE( hasNotZeroNotAtEnd );
}