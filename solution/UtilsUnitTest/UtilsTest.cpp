#define CATCH_CONFIG_MAIN  // This tells Catch to provide a main() - only do this in one cpp file
#include "catch.hpp"
#include "Utils.hpp"
#include "Windows.h"

using namespace rmcmd;
using nlohmann::json;

TEST_CASE( "MaxNum", "[Utils]" ) {
    const auto make_test = [ ] ( auto maxNum ) {
        auto numStr = util::numToFullString( maxNum );
        auto backNum = boost::lexical_cast< decltype( maxNum ) >( numStr );

        bool isCorrectStrSize = numStr.size( ) == std::numeric_limits<decltype( maxNum )>::digits10 + 1;
        REQUIRE( isCorrectStrSize );
        REQUIRE( maxNum == backNum );
    };

    auto maxNum32 = std::numeric_limits<uint32_t>::max( );
    auto maxNum16 = std::numeric_limits<uint16_t>::max( );
    auto maxNum64 = std::numeric_limits<uint64_t>::max( );

    make_test( maxNum32 ); make_test( maxNum16 );
    make_test( maxNum64 );
}


TEST_CASE( "MinNum", "[Utils]" ) {
    auto num = 0u;
    auto numStr = util::numToFullString( num );
    auto backNum = boost::lexical_cast< decltype( num ) >( numStr );

    bool isCorrectStrSize = numStr.size( ) == std::numeric_limits<decltype( num )>::digits10 + 1;
    REQUIRE( isCorrectStrSize );
    REQUIRE( num == backNum );
}

TEST_CASE( "averageNum", "[Utils]" ) {
    auto num = std::numeric_limits<uint32_t>::max( ) / 10;
    auto numStr = util::numToFullString( num );
    auto backNum = boost::lexical_cast< decltype( num ) >( numStr );

    bool isCorrectStrSize = numStr.size( ) == std::numeric_limits<decltype( num )>::digits10 + 1;

    bool hasZeroChar = numStr.front( ) == '0';

    REQUIRE( hasZeroChar );
    REQUIRE( isCorrectStrSize );
    REQUIRE( num == backNum );
}

TEST_CASE( "Binary to|from json", "[Utils]" ) {
    struct S {
        struct B {
            int i = 2;
            int b = 1;
        } b;
        char c = 0;
    } source;

    json jto;
    util::bin::to_json( jto, source );

    S obtained;

    util::bin::from_json( jto, obtained );

    REQUIRE( util::isMemEqual( source, obtained ) );
}

TEST_CASE( "CONSOLE_SCREEN_BUFFER_INFOEX auto-bin-json-test", "[Utils]" ) {
    CONSOLE_SCREEN_BUFFER_INFOEX original{ 1, 2, 3, 4 };

    json j = original; // Automatical conversion to json in binary way

    decltype( original ) obtained = j; // Automatical conversion from json

    REQUIRE( rmcmd::util::isMemEqual( original, obtained ) );
}

TEST_CASE( "CopyMemory test", "[Utils]" ) {
    const CONSOLE_SCREEN_BUFFER_INFOEX original{ 1, 2, 3, 4 };
    CONSOLE_SCREEN_BUFFER_INFOEX obtained = { 0 };

    util::copyMemory( obtained, original );

    REQUIRE( rmcmd::util::isMemEqual( original, obtained ) );
}

TEST_CASE( "Vector of input records serialization test", "Console" ) {
    std::vector<INPUT_RECORD> original;
    original.emplace_back( INPUT_RECORD{ 1, 2, 3 } );
    original.emplace_back( INPUT_RECORD{ 5, 4, 6 } );

    json j = original; // Automatical conversion to json in binary way

    decltype( original ) obtained = j; // Automatical conversion from json

    REQUIRE( rmcmd::util::isVecEqual( original, obtained ) );
}