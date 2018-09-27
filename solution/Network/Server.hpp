#pragma once

#include "Network.hpp"
#include <chrono>

namespace rmcmd {
    template<typename SessionType>
    class Server: public Network {
    public:
        Server( ): acceptor_{ std::make_unique<boost::asio::ip::tcp::acceptor>( *io_service_ ) } {
            ssl_context_.set_options(
                boost::asio::ssl::context::default_workarounds
                | boost::asio::ssl::context::no_sslv2
                | boost::asio::ssl::context::no_sslv3
                | boost::asio::ssl::context::no_tlsv1
                | boost::asio::ssl::context::no_tlsv1_1
                /*| boost::asio::ssl::context::single_dh_use*/ );

            //ssl_context_.set_password_callback( boost::bind( &server::get_password, this ) );

            try {
                ssl_context_.use_certificate_chain_file( "server.crt" );
            } catch( ... ) {
                ::MessageBox( NULL, L"Программа не смогла найти файл сертификата. Пожалуйста, поместите"
                              "сертификат с именем \"server.crt\" в папку с исполняемым файлом.",
                              L"RemoteConsole", MB_OK | MB_ICONERROR );
                std::quick_exit( 0 );
            }
            try {
                ssl_context_.use_private_key_file( "server.key", boost::asio::ssl::context::pem );
            } catch( ... ) {
                ::MessageBox( NULL, L"Программа не смогла найти файл приватного ключа. Пожалуйста, поместите"
                              "ключ с именем \"server.key\" в папку с исполняемым файлом.",
                              L"RemoteConsole", MB_OK | MB_ICONERROR );
                std::quick_exit( 0 );
            }
        }

        void disconnect( ) override {
            acceptor_.reset( );
            socket_.reset( );
            Network::disconnect( );
            //acceptor_ = std::make_unique<boost::asio::ip::tcp::acceptor>( *io_service_ );
        }

        using OnSessionStart = std::function<void( std::shared_ptr<SessionType> session )>;
        using OnSessionStartInfo = std::function<void( const std::string& address,
                                                       std::uint16_t port )>;
        using OnSessionEndInfo = OnSessionStartInfo;
        using OnSessionLogin = std::function<void( const boost::system::error_code& isOk,
                                                   const std::string& address,
                                                   std::uint16_t port,
                                                   std::uint32_t consoleProcessId )>;

        void setOnSessionStartInfoCallback( OnSessionStartInfo onSessionStart ) {
            assert( onSessionStart );
            onSessionStartInfo_ = std::move( onSessionStart );
        }

        void setOnSessionEndInfoCallback( OnSessionEndInfo onSessionEnd ) {
            assert( onSessionEnd );
            onSessionEndInfo_ = std::move( onSessionEnd );
        }

        void setOnSessionStartCallback( OnSessionStart onSessionStart ) {
            assert( onSessionStart );
            onSessionStart_ = std::move( onSessionStart );
        }

        void setOnSessionLoginCallback( OnSessionLogin callback ) {
            assert( callback );
            onSessionLogin_ = std::move( callback );
        }

        virtual void onSessionStart( std::shared_ptr<SessionType> session ) { }

        void createServer(
            unsigned short port,
            OnFinish onFinish ) 
        {
            using boost::asio::ip::tcp;
            namespace ip = boost::asio::ip;
            namespace errc = boost::system::errc;
            namespace ssl = boost::asio::ssl;
            using namespace std::literals;

            assert( !isConnected_ );
            assert( onFinish );

            endpoint_ = { boost::asio::ip::tcp::v4( ), port };
            try {
                // Can throw if port is busy.

                bool reuse_address = false;
                acceptor_ = std::make_unique<boost::asio::ip::tcp::acceptor>( *io_service_,
                                                                              endpoint_,
                                                                              reuse_address );

                onFinish( errc::make_error_code( errc::success ) );

                startAccept( );

                startNetworkThread( );
            } catch( const boost::system::system_error& err ) {
                onFinish( err.code( ) );
            }
        }
    
        void setUpdateInterval( const std::chrono::milliseconds& updateInterval ) {
            using namespace std::literals;
            if( updateInterval < 5ms || updateInterval > 5s )
                return;
            updateInterval_ = updateInterval;
        }
    private:
        void startAccept( ) {
            namespace ssl = boost::asio::ssl;
            using boost::asio::ip::tcp;

            socket_ = std::make_unique<ssl::stream<tcp::socket>>( *io_service_, ssl_context_ );

            if( acceptor_->is_open( ) )
            acceptor_->async_accept( socket_->lowest_layer( ), [ = ] ( auto& err ) {
                if( err ) {

                } else {
                    socket_->async_handshake( boost::asio::ssl::stream_base::server, [ = ] ( auto& err ) {
                        if( err ) {

                        } else {
                            auto remoteEp = socket_->lowest_layer( ).remote_endpoint( );
                            onSessionStartInfo_( remoteEp.address( ).to_string( ),
                                                 remoteEp.port( ) );

                            auto session = std::make_shared<SessionType>( std::move( socket_ ) );

                            session->setOnSessionEndCallback( onSessionEndInfo_ ); // don't move
                            session->setOnSessionLoginCallback( onSessionLogin_ );
                            session->setUpdateInterval( updateInterval_ );

                            onSessionStart_( session );
                            onSessionStart( session );

                            session->start( );

                            startAccept( );
                        }
                    } );
                }
            } );
        }
    private:
        OnSessionStart onSessionStart_{ [ ] ( auto ) { } };
        OnSessionStartInfo onSessionStartInfo_{ [ ] ( auto... ) { } };
        OnSessionEndInfo onSessionEndInfo_{ [ ] ( auto... ) { } };
        OnSessionLogin onSessionLogin_{ [ ] ( auto... ) { } };

        boost::asio::ssl::context ssl_context_{
            boost::asio::ssl::context::method::tlsv12_server };
    private:
        boost::asio::ip::tcp::endpoint endpoint_;
        std::unique_ptr<boost::asio::ip::tcp::acceptor> acceptor_;
        std::unique_ptr<boost::asio::ssl::stream<boost::asio::ip::tcp::socket>> socket_;
    private:
        std::chrono::milliseconds updateInterval_{ 50 };
    };
}