#include "IdGenerator.hpp"
#include <mutex>

boost::uuids::uuid rmcmd::IdGenerator::generateId( ) {
    std::lock_guard<std::mutex> lock{ mutex_ };
    return idGenerator_( );
}