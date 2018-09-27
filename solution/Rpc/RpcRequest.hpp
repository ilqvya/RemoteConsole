#pragma once

#include "Rpc.hpp"
#include "Utils.hpp"
#include "IdGenerator.hpp"
#include "RpcResponse.hpp"
#include "boost/utility/string_view.hpp"

namespace rmcmd {
    class RpcRequest final: public Rpc {
    public:
        using Rpc::Rpc; // Use parent constructors

        // Construct RPC request and set id
        template<typename... Args>
        RpcRequest( std::string methodName, Args&&... args ) {
            json_[ "method" ] = std::move( methodName );
            json_[ "params" ] = json_.array( );
            json_[ "id" ] = boost::lexical_cast< std::string >( idGenerator_.generateId( ) );

            util::forEachArgument( [ & ] ( auto&& arg ) {
                json_[ "params" ].emplace_back( std::forward<decltype( arg )>( arg ) );
            }, std::forward<Args>( args )... );
        }

        /* This constructor fix bug when RpcRequest constructing with sinble const char*
           method name, and for this argument invokes constructor Rpc::Rpc( json )*/
        template<typename... Args>
        RpcRequest( const char* methodName, Args&&... args ) {
            json_[ "method" ] = methodName;
            json_[ "params" ] = json_.array( );
            json_[ "id" ] = boost::lexical_cast< std::string >( idGenerator_.generateId( ) );

            util::forEachArgument( [ & ] ( auto&& arg ) {
                json_[ "params" ].emplace_back( std::forward<decltype( arg )>( arg ) );
            }, std::forward<Args>( args )... );
        }

        // @return parameter on 'number' position in this rpc
        template<typename T>
        T param( std::size_t number ) const {
            assert( number < paramsSize( ) );
            return json_[ "params" ][ number ].get<T>( );
        }

        // @return a method name of the RPC request
        std::string methodName( ) const;

        // reset method name
        void setMethodName( const std::string& methodName );

        // @return number of parameters of the RPC request
        std::size_t paramsSize( ) const;

        // Construct RPC response with id of this RpcRequest
        template<typename... Args>
        RpcResponse makeResponse( Args&&... result ) const {
            return RpcResponse{ getId( ), boost::none,
                std::forward<Args>( result )... };
        }

        // Construct RPC response error with id of this RpcRequest
        RpcResponse makeResponseError( RpcResponse::ErrorCode errorCode,
                                       std::string errorMessage ) const;
    private:
        static IdGenerator idGenerator_; // Thread safe
    };
}
