#pragma once

#include <vector>
#include <cstdint>
#include <type_traits>
#include "IdGenerator.hpp"
#include "boost/uuid/uuid.hpp"
#include "json.hpp"

namespace rmcmd {
    class RpcRequest;
    class RpcResponse;

    // Powered by https://ru.wikipedia.org/wiki/JSON-RPC
    // and https://github.com/nlohmann/json
    class Rpc {
    public:
        class Exception final: std::invalid_argument {
        public:
            using std::invalid_argument::invalid_argument;
        };

        Rpc( ) = default;
        virtual ~Rpc( ) = default;

        Rpc( Rpc&& rpc ) = default;
        Rpc& operator=( Rpc&& rpc ) = default;
        Rpc( const Rpc& rpc ) = default;
        Rpc& operator=( const Rpc& rpc ) = default;

        // Remove id from json
        void responseIsNotRequired( );

        // Check for id field in json
        bool isResponseRequired( ) const;

        // Construct RPC from bytes compressed by Rpc::toBytes method"
        Rpc( const std::vector<std::uint8_t>& bytes );

        // What need to do, if json is invalid rpc? throw ? assert ?
        explicit Rpc( nlohmann::json&& json );
        explicit Rpc( const nlohmann::json& json );

        // Encode/compress for sending
        // info: https://github.com/nlohmann/json#binary-formats-cbor-and-messagepack
        std::vector<std::uint8_t> toBytes( ) const;

        // @return true, if the rpc is response rpc.
        bool isRequest( ) const;

        // @return true, if the rpc is request rpc.
        bool isResponse( ) const;

        // move internal json_ object to a newly constructed RpcRequest
        // Assert if isRequest is false
        RpcRequest moveToRequest( );

        // move internal json_ object to a newly constructed RpcResponse
        // Assert if isResponse is false
        RpcResponse moveToResponse( );

        // @return id of the current rpc
        boost::uuids::uuid getId( ) const;

        bool isValidRpc( ) const;

        nlohmann::json getJson( ) const;
    protected:
        nlohmann::json json_;
    };

    static_assert( std::is_move_constructible<Rpc>::value, "ERROR" );
    static_assert( std::is_move_assignable<Rpc>::value, "ERROR" );
    static_assert( std::is_nothrow_move_constructible<Rpc>::value, "ERROR" );
    static_assert( std::is_nothrow_move_assignable<Rpc>::value, "ERROR" );
}