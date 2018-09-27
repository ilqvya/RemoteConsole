#pragma once

#include "Rpc.hpp"
#include "Utils.hpp"
#include <array>

namespace rmcmd {
    // Holds fixed size info of next Rpc
    // Example: "{ \"type\": 0, \"size\": 0000000000 }"
    class RpcHeader final: public Rpc {
    public:
        enum class RpcType {
            Response = 0, Request = 1, unused
        };

        using body_size_t = std::uint32_t;

        constexpr static body_size_t SIZE{
            static_cast<body_size_t>(util::cStringLength( R"({"size":"0000000000","type":"1"})" )) 
        };

        constexpr static body_size_t MAX_BODY_SIZE{
            1'048'576 * 5 // 5 Mgb
        };

        using buffer_t = std::array<char, RpcHeader::SIZE>;

        RpcHeader( ) = default;

        // Encode for sending
        RpcHeader( body_size_t rpcSize, RpcType rpcType = RpcType::unused );

        // Decode from buffer
        RpcHeader( const std::array<char, RpcHeader::SIZE>& buffer );

        void toBuffer( std::array<char, RpcHeader::SIZE>& buffer ) const;

        body_size_t bodySize( ) const;

        RpcType bodyType( ) const;
    };
}