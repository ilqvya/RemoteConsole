#pragma once

#include "boost/uuid/uuid_generators.hpp"
#include "boost/uuid/uuid_io.hpp" // boost::uuids::to_string
#include "boost/lexical_cast.hpp"
#include <mutex>
#include <memory>

namespace rmcmd {
    // Thread-safe id generator for Rpc
    class IdGenerator final {
    public:
        // Thread safe!
        boost::uuids::uuid generateId( );
    private:
        boost::uuids::random_generator idGenerator_;
        std::mutex mutex_;
    };
}
