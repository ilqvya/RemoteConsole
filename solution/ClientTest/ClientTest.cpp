#include "ConsoleClient.hpp"

#include "boost/asio.hpp"
#include "boost/lexical_cast.hpp"

#include <iostream>
#include <string>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <chrono>
#include <regex>

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
    bool isSessionAtStart = false;
    std::mutex cout_mutex;
    std::mutex mutex;
    std::condition_variable cond_var;
    std::string login, password, ip, port, updateInterval;

    std::cout << "Login (5-12): "; login = getUserInput( R"(\w{5,12})" );

    SetStdinEcho( false );
    std::cout << "Password (5-20): "; password = getUserInput( R"(\w{5,20})", true ); std::cout << '\n';
    SetStdinEcho( true );

    auto ipRegex = R"((([0-9]|[1-9][0-9]|1[0-9]{2}|2[0-4][0-9]|25[0-5])\.){3}([0-9]|[1-9][0-9]|1[0-9]{2}|2[0-4][0-9]|25[0-5]))";
    std::cout << "Ip: "; ip = getUserInput( ipRegex );

    auto portRegex = R"(([0-9]|[1-8][0-9]|9[0-9]|[1-8][0-9]{2}|9[0-8][0-9]|99[0-9]|[1-8][0-9]{3}|9[0-8][0-9]{2}|99[0-8][0-9]|999[0-9]|[1-5][0-9]{4}|6[0-4][0-9]{3}|65[0-4][0-9]{2}|655[0-2][0-9]|6553[0-5]))"s;

    std::cout << "Port (0-65535): "; port = getUserInput( portRegex );

    auto intervalRegex = R"(([5-9]|[1-8][0-9]|9[0-9]|[1-8][0-9]{2}|9[0-8][0-9]|99[0-9]|[1-4][0-9]{3}|5000))";
    std::cout << "Update interval (5-5000 ms): "; updateInterval = getUserInput( intervalRegex );

    rmcmd::ConsoleClient client{ std::move( login ), std::move( password ) };

    client.setOnSessionEndInfoCallback( [ & ] ( auto addr, auto port ) {
        std::lock_guard<std::mutex> guard{ cout_mutex };
        std::cout << "Session was stopped. Ip=" << addr << "; Port=" << port << '\n';
    } );

    client.setOnSessionStartInfoCallback( [ & ] ( auto addr, auto port ) {
        //        std::cout << "New session started. Ip=" << addr << "; Port=" << port << '\n';
    } );

    client.setOnSessionLoginCallback( [ & ] ( auto err, auto addr, auto port, auto ) {
        if( err ) {
            std::lock_guard<std::mutex> guard{ cout_mutex };
            std::cout << "\nLogin or password is incorrect.";
            std::quick_exit( 0 );
        } else {
            {
                std::unique_lock<std::mutex> lock( mutex );
                isSessionAtStart = false;
                cond_var.notify_one( );
            }
            std::lock_guard<std::mutex> guard{ cout_mutex };
            std::cout << "New session started. Ip=" << addr << "; Port=" << port << '\n';
        }
    } );

    {
        std::lock_guard<std::mutex> guard{ cout_mutex };
        std::cout << "!!! Press space to spawn new console !!!\n";
    }

    client.connectToServer( ip, boost::lexical_cast<uint16_t>( port ),
        [ & ] ( const boost::system::error_code& error ) {
        if( error ) {
            std::lock_guard<std::mutex> guard{ cout_mutex };
            std::cout << "Error: " << error.message( );
            std::quick_exit( 0 );
        } else {

        }
    } );

    {
        std::unique_lock<std::mutex> lock( mutex );
        isSessionAtStart = true;
        cond_var.wait( lock, [ & ] { return false == isSessionAtStart; } );
        FlushConsoleInputBuffer( GetStdHandle( STD_INPUT_HANDLE ) );
    }

    std::vector<INPUT_RECORD> buffer( 32 );
    DWORD readed=0;
    while( ReadConsoleInput( GetStdHandle( STD_INPUT_HANDLE ), buffer.data( ), 32, &readed ) ) {
        for( DWORD i{ 0u }; i < readed; ++i ) {
            if( KEY_EVENT == buffer[ i ].EventType &&
                TRUE == buffer[ i ].Event.KeyEvent.bKeyDown &&
                VK_SPACE == buffer[ i ].Event.KeyEvent.wVirtualKeyCode ) 
            {
                client.connectToServer( ip, boost::lexical_cast<uint16_t>( port ),
                                        [ & ] ( const boost::system::error_code& error ) {
                    if( error ) {
                        std::lock_guard<std::mutex> guard{ cout_mutex };
                        std::cout << "Error: " << error.message( );
                    } else {

                    }
                } );
                std::unique_lock<std::mutex> lock( mutex );
                isSessionAtStart = true;
                cond_var.wait( lock, [ & ] { return false == isSessionAtStart; } );
                FlushConsoleInputBuffer( GetStdHandle( STD_INPUT_HANDLE ) );
            }
        }
    }
} catch( const std::exception& err ) {
    std::cerr << "\n\n\nException: " << err.what( );
} catch( ... ) {
    std::cerr << "\n\n\nUndefined exception";
}
