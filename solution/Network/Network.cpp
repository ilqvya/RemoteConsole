#include "Network.hpp"

namespace errc = boost::system::errc;
namespace asio = boost::asio;
using namespace std::literals;

rmcmd::Network::Network( ): io_service_{ std::make_unique<asio::io_service>( ) } { }

bool rmcmd::Network::isConnected( ) const {
    return isConnected_;
}

void rmcmd::Network::setOnDisconnectCallback( OnDisconnect onDisconnect ) {
    onDisconnect_ = std::move( onDisconnect );
}

void rmcmd::Network::disconnect( ) {
    if( isConnected_ || std::any_of( begin( threads_ ), end( threads_ ), 
                                     [ ] ( auto& thread ) { return thread.joinable( ); } ) ) {
        if( io_service_ ) {
            io_service_->stop( );
        }
        isConnected_ = false;
        //onDisconnect_( { } ); Already invoked in network thread
    }

    for( auto& thread : threads_ )
        if( thread.joinable( ) )
            thread.join( );

    threads_.clear( );

    io_service_ = std::make_unique<asio::io_service>( );
}

rmcmd::Network::~Network( ) {
    disconnect( );
}

void rmcmd::Network::startNetworkThread( ) {
    if( isConnected_ ) return;

    isConnected_ = true;

    if( std::any_of( begin( threads_ ), end( threads_ ),
                     [ ] ( auto& thread ) { return thread.joinable( ); } ) ) {
        disconnect( );
    }

    auto cores = std::thread::hardware_concurrency( );
    if( 0 == cores )
        cores = 2;
    
    for( decltype( cores ) i{ 0 }; i < cores; ++i ) {
        threads_.emplace_back( [ this ] {
            boost::system::error_code ec;
            
            io_service_->run( ec );

            if( isConnected_ ) {
                onDisconnect_( ec );
                isConnected_ = false;
            }
        } );
    }
}

void rmcmd::Network::join( ) {
    for( auto& thread : threads_ )
        if( thread.joinable( ) )
            thread.join( );
}
