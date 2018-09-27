#include "boost/lexical_cast.hpp"

#include <iostream>
#include <string>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <chrono>
#include <regex>

#include "ConsoleServer.hpp"

using namespace std::literals;

void SetStdinEcho( bool enable = true ) {
    HANDLE hStdin = GetStdHandle( STD_INPUT_HANDLE );
    DWORD mode;
    GetConsoleMode( hStdin, &mode );

    if( !enable )
        mode &= ~ENABLE_ECHO_INPUT;
    else
        mode |= ENABLE_ECHO_INPUT;

    SetConsoleMode( hStdin, mode );
}

std::string getUserInput( const std::string& regex_pattern, bool isPassw = false ) {
    std::string userInput;
    std::regex regex{ regex_pattern };

    std::cin >> userInput;

    while( !std::regex_match( userInput, regex ) ) {
        if( isPassw ) std::cout << '\n';
        std::cout << "Invalid input. Try again: ";
        std::cin >> userInput;
    }

    return userInput;
}

int main( ) try {
    std::mutex cout_mutex;
    std::string login, password, port, updateInterval;

    std::cout << R"(Login (5-12): )"; login = getUserInput( R"(\w{5,12})" );

    SetStdinEcho( false );
    std::cout << "Password (5-20): "; password = getUserInput( R"(\w{5,20})", true ); std::cout << '\n';
    SetStdinEcho( true );

    auto portRegex = R"(([0-9]|[1-8][0-9]|9[0-9]|[1-8][0-9]{2}|9[0-8][0-9]|99[0-9]|[1-8][0-9]{3}|9[0-8][0-9]{2}|99[0-8][0-9]|999[0-9]|[1-5][0-9]{4}|6[0-4][0-9]{3}|65[0-4][0-9]{2}|655[0-2][0-9]|6553[0-5]))"s;

    std::cout << "Port (0-65535): "; port = getUserInput( portRegex );

    auto intervalRegex = R"(([5-9]|[1-8][0-9]|9[0-9]|[1-8][0-9]{2}|9[0-8][0-9]|99[0-9]|[1-4][0-9]{3}|5000))";
    std::cout << "Update interval (5-5000 ms): "; updateInterval = getUserInput( intervalRegex );

    rmcmd::ConsoleServer server{ std::move( login ), std::move( password ) };

    if( !updateInterval.empty( ) )
        server.setUpdateInterval( std::chrono::milliseconds{ boost::lexical_cast< uint32_t >( updateInterval ) } );

    server.setOnSessionLoginCallback( [ & ] ( auto err, auto addr, auto port, auto ) {
        if( err ) {
            std::lock_guard<std::mutex>{ cout_mutex };
            std::cout << "Unsuccessful attempt to log in. Ip=" << addr << "Port=" << port << '\n';
        } else {
            std::lock_guard<std::mutex>{ cout_mutex };
            std::cout << "New session started. Ip=" << addr << "; Port=" << port << '\n';
        }
    } );

    server.setOnSessionEndInfoCallback( [ & ] ( auto addr, auto port ) {
        std::lock_guard<std::mutex>{ cout_mutex };
        std::cout << "Session was stopped. Ip=" << addr << "; Port=" << port << '\n';
    } );

    server.createServer( boost::lexical_cast<uint16_t>( port ),
        [ & ] ( const auto& error ) {
        if( error ) {
            std::lock_guard<std::mutex>{ cout_mutex };
            std::cout << "Error: " << error.message( );
            std::quick_exit( 0 );
        } else
            std::lock_guard<std::mutex>{ cout_mutex };
            std::cout << "Server is running.\n";
    } );

    while( true )
        std::this_thread::sleep_for( 50000h );
} catch( const std::exception& err ) {
    std::cerr << "\n\n\nException: " << err.what( );
} catch( ... ) {
    std::cerr << "\n\n\nUndefined exception";
}
