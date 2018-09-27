#include "catch.hpp"
#include "RpcHeader.hpp"
#include "boost/lexical_cast.hpp"
#include <limits>

using namespace rmcmd;
using nlohmann::json;

TEST_CASE( "RpcHeader TeSt", "[RpcHeader]" ) {
    RpcHeader::body_size_t bodySize{ 100 };

    RpcHeader rpcHeader{ bodySize, RpcHeader::RpcType::Request };

    REQUIRE( rpcHeader.bodySize( ) == bodySize );
    REQUIRE( rpcHeader.bodyType( ) == RpcHeader::RpcType::Request );

    RpcHeader::buffer_t buffer;
    rpcHeader.toBuffer( buffer );

    //auto dumpStr = rpcHeader.getJson( ).dump( );

    //std::cout << "\nB " << bufferStr << '\n';
    //std::cout << "\nD " << dumpStr << '\n';

    bool isCorrectBuffer = std::string( buffer.data( ), buffer.size( ) ) == rpcHeader.getJson( ).dump( );

    REQUIRE( isCorrectBuffer );

    RpcHeader fromBufferHeader{ buffer };

    REQUIRE( fromBufferHeader.getJson( ) == rpcHeader.getJson( ) );
}