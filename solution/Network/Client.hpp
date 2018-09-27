#pragma once

#include "Network.hpp"

namespace rmcmd {
    template<typename SessionType>
    class Client: public Network {
    public:
        Client( ): resolver_{ std::make_unique<boost::asio::ip::tcp::resolver>( *io_service_ ) } {
            try {
                ssl_context_.load_verify_file( "server.crt" );
            } catch( ... ) {
                ::MessageBox( NULL, L"Программа не смогла найти файл сертификата. Пожалуйста, поместите"
                              "сертификат с именем \"server.crt\" в папку с исполняемым файлом.",
                              L"RemoteConsole", MB_OK | MB_ICONERROR );
                std::quick_exit( 0 );
            }
        }

        using OnSessionStart = std::function<void( std::shared_ptr<SessionType> session )>;

        using OnSessionStartInfo = std::function<void( const std::string& address,
                                                       std::uint16_t port )>;
        using OnSessionEndInfo = OnSessionStartInfo;
        using OnSessionLogin = std::function<void( const boost::system::error_code& isOk,
                                                   const std::string& address,
                                                   std::uint16_t port,
                                                   std::uint32_t consoleProcessId )>;

        void setOnSessionStartCallback( OnSessionStart onSessionStart ) {
            assert( onSessionStart );
            onSessionStart_ = std::move( onSessionStart );
        }

        void setOnSessionStartInfoCallback( OnSessionStartInfo onSessionStart ) {
            assert( onSessionStart );
            onSessionStartInfo_ = std::move( onSessionStart );
        }

        void setOnSessionEndInfoCallback( OnSessionEndInfo onSessionEnd ) {
            assert( onSessionEnd );
            onSessionEndInfo_ = std::move( onSessionEnd );
        }

        void setOnSessionLoginCallback( OnSessionLogin callback ) {
            assert( callback );
            onSessionLogin_ = std::move( callback );
        }

        void disconnect( ) override {
            resolver_.reset( );
            socket_.reset( );
            worker_.reset( );
            Network::disconnect( ); // reset io_service
            assert( io_service_ );
            worker_ = std::make_unique<boost::asio::io_service::work>( *io_service_ );
            resolver_ = std::make_unique<boost::asio::ip::tcp::resolver>( *io_service_ );
        }

        virtual void onSessionStart( std::shared_ptr<SessionType> session ) { }

        void setUpdateInterval( const std::chrono::milliseconds& updateInterval ) {
            using namespace std::literals;
            if( updateInterval < 5ms || updateInterval > 5s )
                return;
            updateInterval_ = updateInterval;
        }

        void connectToServer(
            const std::string& address,
            std::uint16_t port,
            OnFinish onFinish ) 
        {
            using boost::asio::ip::tcp;
            namespace ip = boost::asio::ip;
            namespace asio = boost::asio;
            namespace errc = boost::system::errc;
            namespace ssl = boost::asio::ssl;

            //assert( !isConnected_ );
            assert( onFinish );

            if( resolver_ && !isConnected_ )
                disconnect( );

            endpoint_ = { ip::address::from_string( address ), port };
            resolver_->async_resolve( endpoint_, [ = ] ( auto& err, auto addressIt ) {
                if( err ) {
                    onFinish( err );
                } else {
                    socket_ = std::make_unique<ssl::stream<tcp::socket>>( *io_service_, ssl_context_ );
                    asio::async_connect( socket_->lowest_layer( ), addressIt, [ = ] ( auto& err, auto it ) {
                        if( err ) {
                            onFinish( err );
                        } else {
                            socket_->async_handshake( ssl::stream_base::client, [ = ] ( auto& err ) {
                                if( err ) {
                                    onFinish( err );
                                } else {
                                    auto localEp = socket_->lowest_layer( ).local_endpoint( );

                                    auto session = std::make_shared<SessionType>( std::move( socket_ ) );

                                    session->setOnSessionEndCallback( onSessionEndInfo_ ); // don't move
                                    session->setOnSessionLoginCallback( onSessionLogin_ );

                                    onSessionStart_( session );
                                    onSessionStart( session );
                                    session->setUpdateInterval( updateInterval_ );

                                    session->start( );

                                    onFinish( errc::make_error_code( errc::success ) );

                                    onSessionStartInfo_( localEp.address( ).to_string( ),
                                                         localEp.port( ) );
                                }
                            } );
                        }
                    } );
                }
            } );

            startNetworkThread( );
        }
    private:
        OnSessionStart onSessionStart_{ [ ] ( auto ) { } };
        OnSessionStartInfo onSessionStartInfo_{ [ ] ( auto... ) { } };
        OnSessionEndInfo onSessionEndInfo_{ [ ] ( auto... ) { } };
        OnSessionLogin onSessionLogin_{ [ ] ( auto... ) { } };

        boost::asio::ssl::context ssl_context_{ 
            boost::asio::ssl::context::method::tlsv12_client };
    private:
        std::unique_ptr<boost::asio::ssl::stream<boost::asio::ip::tcp::socket>> socket_;
        std::unique_ptr<boost::asio::ip::tcp::resolver> resolver_;
        boost::asio::ip::tcp::endpoint endpoint_;
        std::unique_ptr<boost::asio::io_service::work> worker_;
        std::chrono::milliseconds updateInterval_{ 50 };
    };
}