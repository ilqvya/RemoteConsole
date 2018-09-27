#include "catch.hpp"
#include "IdGenerator.hpp"
#include <algorithm>
#include <set>
#include <thread>

TEST_CASE( "Id is unique", "[rmcmd::IdGenerator]" ) {
    rmcmd::IdGenerator idGenerator;

    std::set<boost::uuids::uuid> ids;
    constexpr size_t insertion_number{ 1000 };

    std::generate_n( std::inserter( ids, ids.begin( ) ),
                     insertion_number, [ & ] {
        return idGenerator.generateId( );
    } );

    REQUIRE( ids.size( ) == insertion_number ); // insertion_number of unique ids
}
