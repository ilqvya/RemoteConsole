#include "Rpc.hpp"
#include "RpcRequest.hpp"
#include "RpcResponse.hpp"
#include "boost/lexical_cast.hpp"
#include <iostream>

using nlohmann::json;

rmcmd::Rpc::Rpc( const std::vector<std::uint8_t>& bytes )
    : json_{ json::from_cbor( bytes ) } {
    json_ = json_.front( ); // Returns an our json into [ { } ]. That's why we need 'front' element
    if( !isValidRpc( ) ) throw Exception{ "Invalid Rpc" };
}

rmcmd::Rpc::Rpc( json&& json ) {
    json_ = std::move( json );
    if( !isValidRpc( ) ) throw Exception{ "Invalid Rpc" };
}

rmcmd::Rpc::Rpc( const json& json ) {
    json_ = json;
    if( !isValidRpc( ) ) throw Exception{ "Invalid Rpc" };
}

std::vector<std::uint8_t> rmcmd::Rpc::toBytes( ) const {
    return json_.to_cbor( json_ );
}

bool rmcmd::Rpc::isRequest( ) const {
    return json_.find( "method" ) != json_.cend( );
}

bool rmcmd::Rpc::isResponse( ) const {
    return ( json_.find( "result" ) != json_.cend( ) )
        || ( json_.find( "error" ) != json_.cend( ) );
}

rmcmd::RpcRequest rmcmd::Rpc::moveToRequest( ) {
    assert( isRequest( ) );
    return RpcRequest{ std::move( json_ ) };
}

rmcmd::RpcResponse rmcmd::Rpc::moveToResponse( ) {
    assert( isResponse( ) );
    return RpcResponse{ std::move( json_ ) };
}

boost::uuids::uuid rmcmd::Rpc::getId( ) const {
    return boost::lexical_cast< boost::uuids::uuid >( json_[ "id" ].get<std::string>( ) );
}

bool rmcmd::Rpc::isValidRpc( ) const {
    // TODO: Refactor + optimize
    /*if( ( json_.find( "id" ) == json_.cend( ) )
        || !json_[ "id" ].is_string( ) ) return false;*/

    auto request = isRequest( );
    auto response = isResponse( );

    if( request && response ) return false;
    if( !request && !response ) return false;

    if( request ) {
        if( ( json_.find( "method" ) == json_.cend( ) )
            || !json_[ "method" ].is_string( ) ) return false;

        if( ( json_.find( "params" ) == json_.cend( ) )
            || !json_[ "params" ].is_array( ) ) return false;

    } else { // response
        auto isError = json_.find( "error" ) != json_.cend( );
        auto isNotError = json_.find( "result" ) != json_.cend( );

        if( isError && isNotError ) return false;
        if( !isError && !isNotError ) return false;

        if( isError ) {
            if( !json_[ "error" ].is_object( ) ) return false;

            if( json_[ "error" ].find( "code" ) == json_[ "error" ].cend( ) ) return false;
            if( !json_[ "error" ][ "code" ].is_number( ) ) return false;

            if( json_[ "error" ].find( "message" ) == json_[ "error" ].cend( ) ) return false;
            if( !json_[ "error" ][ "message" ].is_string( ) ) return false;
        } else if( isNotError ) {
            if( !json_[ "result" ].is_array( ) ) return false;
        }
    }

    return true;
}

nlohmann::json rmcmd::Rpc::getJson( ) const {
    return json_;
}

void rmcmd::Rpc::responseIsNotRequired( ) {
    json_.erase( "id" );
}

bool rmcmd::Rpc::isResponseRequired( ) const {
    return json_.find( "id" ) != json_.cend( );
}