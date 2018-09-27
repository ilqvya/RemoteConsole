#include "RpcRequest.hpp"

decltype( rmcmd::RpcRequest::idGenerator_ ) rmcmd::RpcRequest::idGenerator_;

std::string rmcmd::RpcRequest::methodName( ) const {
    return json_[ "method" ].get<std::string>( );
}

void rmcmd::RpcRequest::setMethodName( const std::string& methodName ) {
    json_[ "method" ] = methodName;
}

std::size_t rmcmd::RpcRequest::paramsSize( ) const {
    return json_[ "params" ].size( );
}

rmcmd::RpcResponse 
rmcmd::RpcRequest::makeResponseError( RpcResponse::ErrorCode errorCode,
                                      std::string errorMessage ) const {
    return RpcResponse{ getId( ), std::make_pair( errorCode, std::move( errorMessage ) ) };
}