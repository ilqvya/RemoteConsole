#include "ConsoleServer.hpp"
#include "argon2.h"
#include "random.hpp"

using Random_t = effolkronium::random_local;

rmcmd::ConsoleServer::ConsoleServer( std::string login, std::string password )
    : login_{ std::move( login ) } {

    // https://habrahabr.ru/post/281569/

    //TODO: Refactor
    Random_t random;

    std::vector<std::uint8_t> saltVec;
    saltVec.resize( random.get( 100, 200 ) );
    std::generate( begin( saltVec ), end( saltVec ), [ & ] { return random.get<std::uint8_t>( ); } );

    std::vector<std::uint8_t> out; // // Buffer where to write the raw hash - updated by the function
    out.resize( random.get( 32, 126 ), 0 );

    uint32_t t_cost = 2; // Number of iterations
    uint32_t m_cost = 1 << 16; // memory usage to m_cost kibibytes
    uint32_t parallelism = 2; //  Number of threads and compute lanes
    void* pwd = &password[ 0 ]; // Pointer to password
    size_t pwdlen = password.size( ); // Password size in bytes
    void* salt = &saltVec[ 0 ]; // Pointer to salt
    size_t saltlen = saltVec.size( ); // Salt size in bytes
    void* hash = &out[ 0 ]; // Buffer where to write the raw hash - updated by the function
    size_t hashlen = out.size( ); // Desired length of the hash in bytes

    auto encodedLen = argon2_encodedlen( t_cost, m_cost, parallelism, 
                                         static_cast<uint32_t>( saltlen ),
                                         static_cast<uint32_t>( hashlen ) );
    passwordHash_.resize( encodedLen, 0 );

    char* encoded = &passwordHash_[ 0 ];
    size_t encodedlen = passwordHash_.size( );

    int ret = 0;


    ret = argon2_hash( t_cost, m_cost, parallelism,
                        pwd, pwdlen,
                        salt, saltlen,
                        hash, hashlen,
                        encoded, encodedlen,
                        Argon2_i, ARGON2_VERSION_NUMBER );

    if( ARGON2_OK != ret ) {
        std::string what{ argon2_error_message( ret ) };
        throw std::runtime_error{ "argon2i_hash_raw fail" };
    }

    ret = argon2i_verify( &passwordHash_[ 0 ], &password[ 0 ], password.size( ) );
    if( ARGON2_OK != ret ) {
        std::string what{ argon2_error_message( ret ) };
        throw std::runtime_error{ "argon2i_hash_raw fail" };
    }
}

void rmcmd::ConsoleServer::onSessionStart( std::shared_ptr<ClientSession> session ) {
    session->setLogin( login_ );
    session->setPasswordHash( passwordHash_ );
}