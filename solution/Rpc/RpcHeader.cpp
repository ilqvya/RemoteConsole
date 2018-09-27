#include "RpcHeader.hpp"
#include <algorithm>

using nlohmann::json;

rmcmd::RpcHeader::RpcHeader( body_size_t rpcSize, RpcType rpcType ) {
    json_[ "size" ] = util::numToFullString( rpcSize );
    json_[ "type" ] = boost::lexical_cast< std::string >( util::toUType( rpcType ) );
}

rmcmd::RpcHeader::RpcHeader( const std::array<char, RpcHeader::SIZE>& buffer ) {
    json_ = json::parse( std::string{ buffer.data( ), buffer.size( ) } );
    if( bodySize( ) > MAX_BODY_SIZE )
        throw Exception{ "Too big body size" };
}

void rmcmd::RpcHeader::toBuffer( std::array<char, RpcHeader::SIZE>& buffer ) const {
    auto jstr = json_.dump( );
    assert( jstr.size( ) == RpcHeader::SIZE ); // !
    jstr.copy( &buffer[ 0 ], RpcHeader::SIZE );
}

rmcmd::RpcHeader::body_size_t rmcmd::RpcHeader::bodySize( ) const {
    return boost::lexical_cast< body_size_t >( json_[ "size" ].get<std::string>( ) );
}

rmcmd::RpcHeader::RpcType rmcmd::RpcHeader::bodyType( ) const {
    auto type = json_[ "type" ].get<std::string>( );

    if( "0" == type )
        return RpcType::Response;
    else
        return RpcType::Request;
}