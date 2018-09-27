#include "RpcResponse.hpp"

std::size_t rmcmd::RpcResponse::resultSize( ) const {
    if( isError( ) ) return 0;
    return json_[ "result" ].size( );
}

bool rmcmd::RpcResponse::isError( ) const {
    return json_.find( "error" ) != json_.cend( );
}

rmcmd::RpcResponse::ErrorCode rmcmd::RpcResponse::errorCode( ) const {
    assert( isError( ) );

    switch( json_[ "error" ][ "code" ].get<std::underlying_type_t<ErrorCode>>( ) ) {
        case -32700: return ErrorCode::parseError;
        case -32600: return ErrorCode::invalidRequest;
        case -32601: return ErrorCode::methodNotFound;
        case -32602: return ErrorCode::invalidParams;
        case -32603: return ErrorCode::internalError;
        case -32000: return ErrorCode::serverError;
    }

    return ErrorCode::invalidErrorCode;
}

std::string rmcmd::RpcResponse::errorMessage( ) const {
    assert( isError( ) );
    return json_[ "error" ][ "message" ].get<std::string>( );
}