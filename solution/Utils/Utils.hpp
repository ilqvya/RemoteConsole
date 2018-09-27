#pragma once

#include <utility>
#include <type_traits>
#include <limits>
#include <vector>
#include "boost/lexical_cast.hpp"
#include "json.hpp"

namespace rmcmd {
    namespace util {
        namespace wtf {
            // https://stackoverflow.com/a/9407521/5734836
            template<typename T>
            struct has_const_iterator {
            private:
                typedef char                      yes;
                typedef struct { char array[ 2 ]; } no;

                template<typename C> static yes test( typename C::const_iterator* );
                template<typename C> static no  test( ... );
            public:
                static const bool value = sizeof( test<T>( 0 ) ) == sizeof( yes );
                typedef T type;
            };

            template <typename T>
            struct has_begin_end {
                template<typename C> static char ( &f( typename std::enable_if<
                                                       std::is_same<decltype( static_cast<typename C::const_iterator ( C::* )( ) const>( &C::begin ) ),
                                                       typename C::const_iterator( C::* )( ) const>::value, void>::type* ) )[ 1 ];

                template<typename C> static char ( &f( ... ) )[ 2 ];

                template<typename C> static char ( &g( typename std::enable_if<
                                                       std::is_same<decltype( static_cast<typename C::const_iterator ( C::* )( ) const>( &C::end ) ),
                                                       typename C::const_iterator( C::* )( ) const>::value, void>::type* ) )[ 1 ];

                template<typename C> static char ( &g( ... ) )[ 2 ];

                static bool const beg_value = sizeof( f<T>( 0 ) ) == 1;
                static bool const end_value = sizeof( g<T>( 0 ) ) == 1;
            };

            template<typename T>
            struct is_container: std::integral_constant<bool, has_const_iterator<T>::value && has_begin_end<T>::beg_value && has_begin_end<T>::end_value> { };
        }

        // Invoke 'func' for each argument 'args'.
        template <typename Func, typename... Args>
        inline void forEachArgument( Func&& func, Args&&... args ) {
            int unused[ sizeof...(args) + 1 ] = { 
                (0,
                std::forward<Func>( func )
                ( std::forward<Args>( args ) ),
                  0 )... };
        }

        // Return value of an enum-class member
        template<typename E>
        inline constexpr auto toUType( E enumerator ) noexcept {
            static_assert( std::is_enum<E>{}, "E type must be an enum only" );
            return static_cast<std::underlying_type_t<E>>( enumerator );
        }

        template<typename T>
        using IsUnsignedIntegral = std::enable_if_t<
            std::is_integral<T>::value && std::is_unsigned<T>::value>;

        // Converts an unsigned integer number to a string with zero-fill
        template<typename Number, typename = IsUnsignedIntegral<Number>>
        inline std::string numToFullString( Number number ) {
            constexpr auto maxDigits = std::numeric_limits<Number>::digits10 + 1;

            auto numberStr = boost::lexical_cast< std::string >( number );
            auto zeroNum = maxDigits - numberStr.size( );

            numberStr = std::string( zeroNum, '0' ) + numberStr;

            return numberStr;
        };

        // Compile time c-string length
        inline constexpr std::size_t cStringLength( const char* str ) {
            return *str ? 1 + cStringLength( str + 1 ) : 0;
        }

        // Memory operations is valid only for objects with standard layout type
        template<typename T>
        using IsStandardLayout = std::enable_if<std::is_standard_layout<T>::value>;

        template<typename T>
        using IsGoodBinType = std::enable_if_t<!rmcmd::util::wtf::is_container<T>::value
                                               && std::is_standard_layout<T>::value>;

        template<typename T, typename = IsGoodBinType<T>>
        inline bool isMemEqual( const T& lhs, const T& rhs ) {
            return 0 == std::memcmp( &lhs, &rhs, sizeof( T ) );
        }

        template<typename T, typename = IsGoodBinType<T>>
        inline bool isVecEqual( const std::vector<T>& lhs, const std::vector<T>& rhs ) {
            if( lhs.size( ) != rhs.size( ) ) return false;
            
            for( std::vector<T>::size_type i{ 0u }; i < lhs.size( ); ++i )
                if( !isMemEqual( lhs[ 0 ], rhs[ 0 ] ) )
                    return false;

            return true;
        }

        template<typename T, typename = IsGoodBinType<T>>
        inline void copyMemory( T& dest, const T& src ) {
            std::memcpy( &dest, &src, sizeof( T ) );
        }

        // binary serialization of plain type objects
        namespace bin {
            // serialize bytes of T object and pass them into json as array
            template<typename T, typename = IsGoodBinType<T>>
            inline void to_json( nlohmann::json& j, const T& src ) {
                std::copy_n( reinterpret_cast< const std::uint8_t* >( &src ),
                             sizeof( T ), std::back_inserter( j ) );
            }

            // deserialize bytes of T object from json array
            template<typename T, typename = IsGoodBinType<T>>
            inline void from_json( const nlohmann::json& j, T& dest ) {
                if( j.size( ) != sizeof( T ) )
                    throw std::invalid_argument{ "Invalid buffer size in json" };

                std::copy_n( cbegin( j ), sizeof( T ),
                             reinterpret_cast< std::uint8_t* >( &dest ) );
            }
        }
    }
}

// Example&Info: https://github.com/nlohmann/json#basic-usage

// THE DARK MAGIC ! 
// Automatic serialization/deserialization into/from json for ALL global structures

// Warning: Structures should be fixed size for different architectures

template<typename T, typename = rmcmd::util::IsGoodBinType<T>>
inline void to_json( nlohmann::json& json, const T& object ) {
    rmcmd::util::bin::to_json( json, object );
}

template<typename T, typename = rmcmd::util::IsGoodBinType<T>>
inline void from_json( const nlohmann::json& json, T& object ) {
    rmcmd::util::bin::from_json( json, object );
}
