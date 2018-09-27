#pragma once

#include "Rpc.hpp"
#include "boost/optional.hpp"
#include "Utils.hpp"

namespace rmcmd {
    class RpcResponse final: public Rpc {
    public:
        using Rpc::Rpc; // Use parent constructors

        RpcResponse( ) = default;

        enum class ErrorCode: std::int32_t {
            parseError = -32700,
            invalidRequest = -32600,
            methodNotFound = -32601,
            invalidParams = -32602,
            internalError = -32603,
            serverError = -32000,
            invalidErrorCode = -1,
        };

        // Construct RPC response
        template<typename... Args>
        RpcResponse( const boost::uuids::uuid& id,
                     // error & message
                     boost::optional<std::pair<ErrorCode, std::string>> error,
                     Args&&... result ) {
            if( error )
                assert( sizeof...( result ) == 0 ); // No results if it is error

            if( error ) {
                json_[ "error" ][ "code" ] = util::toUType( std::get<ErrorCode>( *error ) );
                json_[ "error" ][ "message" ] = std::get<std::string>( *error );
                //json_[ "error" ][ "data" ] = result;
            } else {
                json_[ "result" ] = json_.array( );

                util::forEachArgument( [ & ] ( auto&& arg ) {
                    json_[ "result" ].emplace_back( 
                        std::forward<decltype( arg )>( arg ) );
                }, std::forward<Args>( result )... );
            }
            json_[ "id" ] = boost::lexical_cast< std::string >( id );
        }

        // @return parameter on 'number' position in this rpc
        template<typename T>
        T result( std::size_t number ) const {
            assert( !isError( ) );
            assert( number < resultSize( ) );
            
            return json_[ "result" ][ number ].get<T>( );
        }

        std::size_t resultSize( ) const;

        bool isError( ) const;

        // assert if this rpc isn't error rpc
        ErrorCode errorCode( ) const;
        std::string errorMessage( ) const;
    };
}