#include "boost/process.hpp"
#include "boost/lexical_cast.hpp"
#include "ConsoleProcess.hpp"
#include "ConsoleCommon.hpp"
#include "Rpc.hpp"
#include "RpcRequest.hpp"
#include "RpcHeader.hpp"
#include "Utils.hpp"
#include "boost/lexical_cast.hpp"

using namespace std::literals;
using namespace rmcmd;

static bool operator==( const CHAR_INFO& lhs, const CHAR_INFO& rhs ) {
    return rmcmd::util::isMemEqual( lhs, rhs );
}

static bool operator!=( const CHAR_INFO& lhs, const CHAR_INFO& rhs ) {
    return !rmcmd::util::isMemEqual( lhs, rhs );
}

namespace {
    class ConsoleServer final: public rmcmd::ConsoleCommon {
    public:
        ConsoleServer( std::uint16_t port ): ConsoleCommon{ port } {
            SetConsoleCtrlHandler( [ ] ( DWORD ) -> BOOL { return TRUE; }, TRUE );

            createCmdProcess( );

            hIn_ = CreateFile( L"CONIN$", GENERIC_READ | GENERIC_WRITE, FILE_SHARE_READ, 0, OPEN_EXISTING, 0, 0 );
            hOut_ = CreateFile( L"CONOUT$", GENERIC_READ | GENERIC_WRITE, FILE_SHARE_READ, 0, OPEN_EXISTING, 0, 0 );

            bindRpcFunc( "CTRL_C", [ this ] ( auto ) {
                GenerateConsoleCtrlEvent( CTRL_C_EVENT, 0 );
            } );

            bindRpcFunc( "CTRL_BREAK", [ this ] ( auto ) {
                GenerateConsoleCtrlEvent( CTRL_BREAK_EVENT, 0 );
            } );

            bindRpcFunc( "writeConsoleInput", [ this ] ( const RpcRequest& req ) {
                writeConsoleInput( req.param<std::vector<INPUT_RECORD>>( 0 ) );
            } );

            bindRpcFunc( "SetScreenBufferInfo", [ this ] ( const RpcRequest& req ) {
                setScreenBufferInfo( req.param<CONSOLE_SCREEN_BUFFER_INFO>( 0 ) );
            } );

            bindRpcFunc( "SetConsoleCursorInfo", [ this ] ( const RpcRequest& req ) {
                setConsoleCursorInfo( req.param<CONSOLE_CURSOR_INFO>( 0 ) );
            } );
        }

        ~ConsoleServer( ) {
            if( cmdProcessInfo_.hProcess ) {
                FreeConsole( );

                TerminateProcess( cmdProcessInfo_.hProcess, 0 );
                WaitForSingleObject( cmdProcessInfo_.hProcess, INFINITE );

                CloseHandle( cmdProcessInfo_.hThread );
                CloseHandle( cmdProcessInfo_.hProcess );
            }
        }

        bool isRunning( ) const override {
            if( WaitForSingleObject( cmdProcessInfo_.hProcess, 0 ) != WAIT_OBJECT_0 )
                return ConsoleCommon::isRunning( );
            return false;
        }
    private:
        void updateScreenBufferInfo( ) override {
            CONSOLE_SCREEN_BUFFER_INFO bufferInfo = { 0 };
            GetConsoleScreenBufferInfo( hOut_, &bufferInfo );

            constexpr auto maxBuffSize = 499;

            if( bufferInfo.dwSize.X > maxBuffSize || bufferInfo.dwSize.Y > maxBuffSize ) {
                if( bufferInfo.dwSize.X > maxBuffSize )
                    bufferInfo.dwSize.X = maxBuffSize;

                if( bufferInfo.dwSize.Y > maxBuffSize )
                    bufferInfo.dwSize.Y = maxBuffSize;

                SetConsoleScreenBufferSize( hOut_, bufferInfo.dwSize );
            }


            SHORT newWndX = bufferInfo.srWindow.Right - bufferInfo.srWindow.Left + 1;
            SHORT oldWndX = bufferInfo_.srWindow.Right - bufferInfo_.srWindow.Left + 1;

            if( newWndX == bufferInfo.dwSize.X &&
                newWndX != oldWndX ) 
            {
                fullBufferUpdateRequired_ = true;
            }

            if( !util::isMemEqual( bufferInfo.dwCursorPosition, bufferInfo_.dwCursorPosition )
                || !util::isMemEqual( bufferInfo.dwSize, bufferInfo_.dwSize )
                || !util::isMemEqual( bufferInfo.srWindow, bufferInfo_.srWindow )
                || !util::isMemEqual( bufferInfo.wAttributes, bufferInfo_.wAttributes ) ) {
                bufferInfo_.dwCursorPosition = bufferInfo.dwCursorPosition;
                bufferInfo_.srWindow = bufferInfo.srWindow;
                bufferInfo_.wAttributes = bufferInfo.wAttributes;

                bufferInfo_.dwSize = bufferInfo.dwSize;

                sendRequest( { "SetScreenBufferInfo", bufferInfo_ } );
            }
        }
        bool isFirstUpdate_{ true };

        void update( ) override {
            updateConsoleMode( );
            updateScreenBufferInfo( );
            updateScreenBuffer( );
            updateConsoleTitle( );
            updateConsoleCursorInfo( );

            if( isFirstUpdate_ ) {
                sendRequest( { "FinishFirstUpdate" } );
                isFirstUpdate_ = false;
            }
        }

        std::wstring consoleTitle_ = L"$";
        void updateConsoleTitle( ) {
            constexpr size_t maxTitleSize{ 256u };
            std::wstring consoleTitle;
            consoleTitle.resize( maxTitleSize );
            auto len = GetConsoleTitle( &consoleTitle[ 0 ], maxTitleSize );
            if( 0 != len ) {
                consoleTitle.resize( len );

                auto firstHyphen = consoleTitle.find( L'-', 0 );

                if( firstHyphen != std::wstring::npos && firstHyphen != 0 ) {
                    consoleTitle.erase( consoleTitle.begin( ), consoleTitle.begin( ) + firstHyphen - 1 );
                } else {
                    consoleTitle.clear( );
                }

                if( consoleTitle_ != consoleTitle ) {
                    consoleTitle_ = std::move( consoleTitle );

                    sendRequest( { "SetConsoleTitle", consoleTitle_ } );
                }
            }
        }

        DWORD consoleMode_{ 0 };
        void updateConsoleMode( ) {
            DWORD consoleMode{ 0 };
            GetConsoleMode( hIn_, &consoleMode );
            if( !util::isMemEqual( consoleMode_, consoleMode ) ) {
                consoleMode_ = consoleMode;
                sendRequest( { "SetConsoleMode", consoleMode_ } );
            }
        }

    private: // Screen buffer processing
        void updateScreenBuffer( ) {
            if( bufferInfo_.dwSize.X > 999 || bufferInfo_.dwSize.Y > 999 )
                return;

            auto bufferSize{ bufferInfo_.dwSize.X * bufferInfo_.dwSize.Y };

            try {
                onHold_.screenBuffer.resize( static_cast< std::vector<CHAR_INFO>::size_type >(
                    bufferInfo_.dwSize.X * bufferInfo_.dwSize.Y ) );
            } catch( const std::bad_alloc& ) {
                ::MessageBox( NULL, L"Недостаточно оперативной памяти для обработки буфера консоли.",
                              L"RemoteConsole", MB_OK | MB_ICONERROR );
                this->stop( );
                return;
            }

            auto bufferPtr{ onHold_.screenBuffer.data( ) };

            SMALL_RECT srctReadRect;

            const auto maxBot{ bufferInfo_.dwSize.Y - 1 };
            const auto maxRight{ bufferInfo_.dwSize.X - 1 };

            srctReadRect.Top = 0;    // top left: row 0, col 0 
            srctReadRect.Left = 0;
            srctReadRect.Bottom = maxBot; // bot. right: row 1, col 79 
            srctReadRect.Right = maxRight;

            size_t pices{ 1 };
            size_t totalPices{ pices };
            size_t bufferPice{ 0u };

            bufferPtr = onHold_.screenBuffer.data( );

            while( 0 != pices ) {
                auto copyReadRect{ srctReadRect };
                if( ReadConsoleOutput( hOut_,
                    bufferPtr,
                    bufferInfo_.dwSize,
                    { 0, 0 },
                    &copyReadRect ) ) {
                    --pices;

                    bufferPtr += ( ( ( srctReadRect.Bottom - srctReadRect.Top + 1 ) ) * ( maxRight + 1 ) );
                    srctReadRect.Top = srctReadRect.Bottom + 1;
                    srctReadRect.Bottom += static_cast<SHORT>( ( maxBot + 1 ) / totalPices );

                    if( 0 == pices && srctReadRect.Top != ( maxBot + 1 ) ) {
                        ++pices;
                        srctReadRect.Bottom = maxBot;
                    }
                } else {
                    pices *= 2;
                    totalPices = pices;
                    srctReadRect.Bottom /= 2;
                }
            }

            if( onHold_.screenBuffer != screenBuffer_ ) {

                makeScreenBufferDiff( screenBuffer_,
                                      screenBufferCoord_,
                                      onHold_.screenBuffer,
                                      bufferInfo_.dwSize );

                screenBufferCoord_ = bufferInfo_.dwSize;

                using std::swap;
                swap( screenBuffer_, onHold_.screenBuffer );

                sendRequest( { "UpdateScreenBuffer", onHold_.bufferDiff } );
                fullBufferUpdateRequired_ = false;
            }
        }

        void makeScreenBufferDiff(
            std::vector<CHAR_INFO>& oldBuffer,
            const COORD& oldBufferSize,
            const std::vector<CHAR_INFO>& newBuffer,
            const COORD& newBufferSize ) {
            onHold_.bufferDiff.clear( );

            // Do not send default-created char info for new buffer cells
            CHAR_INFO baseCharInfo_ = { { 0 } };
            baseCharInfo_.Char.AsciiChar = 32;
            baseCharInfo_.Attributes = bufferInfo_.wAttributes;

            for( size_t i = 0; i < newBuffer.size( ); ++i ) {
                bool oldCoordExist{ false };

                if( 0 != oldBufferSize.X || 0 != oldBufferSize.Y ) { // For the first time it is zero
                    COORD oldCoord{ static_cast<SHORT>( i % oldBufferSize.X ),
                        static_cast<SHORT>( i / oldBufferSize.X ) };

                    oldCoordExist = {
                        oldCoord.X < oldBufferSize.X
                        && oldCoord.Y < oldBufferSize.Y
                    };

                }

                // Do not send whole buffer after screen buffer color changes
                if( oldCoordExist
                    && oldColor_ != bufferInfo_.wAttributes
                    && oldBuffer[ i ].Attributes == oldColor_ ) {
                    oldBuffer[ i ].Attributes = bufferInfo_.wAttributes;
                }

                if( ( oldCoordExist && newBuffer[ i ] != oldBuffer[ i ] ) ||
                    ( !oldCoordExist && newBuffer[ i ] != baseCharInfo_ ) || 
                    fullBufferUpdateRequired_ ) 
                {
                    COORD newCoord{ static_cast<SHORT>( i % newBufferSize.X ),
                        static_cast<SHORT>( i / newBufferSize.X ) };
                    onHold_.bufferDiff.emplace_back( newBuffer[ i ], newCoord );
                }
            }

            oldColor_ = bufferInfo_.wAttributes;
        }

    private:
        void writeConsoleInput( std::vector<INPUT_RECORD> inputRecord ) {
            DWORD numberOfEventsWritten = 0;

            if( !WriteConsoleInput(
                hIn_,
                inputRecord.data( ),
                static_cast< DWORD >( inputRecord.size( ) ),
                &numberOfEventsWritten
            ) ) throw std::runtime_error{ "WriteConsoleInput error" };
        }
    private:
        void createCmdProcess( ) {
            STARTUPINFO si = { 0 };
            si.cb = sizeof( si );

            LPTSTR cmdPath = TEXT( "C:\\Windows\\System32\\cmd.exe" );
            LPTSTR cmdArgs = TEXT( "C:\\Windows\\System32\\cmd.exe" );

            if( !CreateProcess(
                cmdPath, cmdArgs,
                NULL, NULL, TRUE, CREATE_UNICODE_ENVIRONMENT,
                NULL, NULL, &si, &cmdProcessInfo_ ) ) {
                throw std::runtime_error{ "Fail to create process" };
            }

            // Wait for process initialization
            WaitForInputIdle( cmdProcessInfo_.hProcess, INFINITE );

            // Wait for cmd creation
            Sleep( 50 );
        }

        void setScreenBufferInfo( const CONSOLE_SCREEN_BUFFER_INFO& bufferInfo ) {
            if( !util::isMemEqual( bufferInfo_.dwSize, bufferInfo.dwSize ) )
                SetConsoleScreenBufferSize( hOut_, bufferInfo.dwSize );

            /*if( !util::isMemEqual( bufferInfo_.dwCursorPosition, bufferInfo.dwCursorPosition ) )
                SetConsoleCursorPosition( hOut_, bufferInfo.dwCursorPosition );*/

            if( !util::isMemEqual( bufferInfo_.srWindow, bufferInfo.srWindow ) )
                SetConsoleWindowInfo( hOut_, TRUE, &bufferInfo.srWindow );

            if( !util::isMemEqual( bufferInfo_.wAttributes, bufferInfo.wAttributes ) ) {
                SetConsoleTextAttribute( hOut_, bufferInfo.wAttributes );

                // refresh
                DWORD numberOfAttrsWritten;
                FillConsoleOutputAttribute(
                    hOut_,
                    bufferInfo.wAttributes,
                    bufferInfo.dwSize.X * bufferInfo.dwSize.Y,
                    { 0, 0 },
                    &numberOfAttrsWritten
                );
            }

            SHORT newWndX = bufferInfo.srWindow.Right - bufferInfo.srWindow.Left + 1;
            SHORT oldWndX = bufferInfo_.srWindow.Right - bufferInfo_.srWindow.Left + 1;

            if( newWndX == bufferInfo.dwSize.X &&
                newWndX != oldWndX ) 
            {
                fullBufferUpdateRequired_ = true;
            }

            bufferInfo_.dwCursorPosition = bufferInfo.dwCursorPosition;
            bufferInfo_.dwSize = bufferInfo.dwSize;
            bufferInfo_.srWindow = bufferInfo.srWindow;
            bufferInfo_.wAttributes = bufferInfo.wAttributes;
        }

        bool fullBufferUpdateRequired_ = false;
    private:
        PROCESS_INFORMATION cmdProcessInfo_ = { 0 };

        // member for processing screen buffer
        std::vector<CHAR_INFO> screenBuffer_;
        COORD screenBufferCoord_ = { { 0 } };
        struct onHold { // Do not reallocate memory for vectors
            std::vector<CHAR_INFO> screenBuffer;
            std::vector<std::pair<CHAR_INFO, COORD>> bufferDiff;
        } onHold_;
    };
}

int main( int argc, char* argv[ ] ) try {
    if( argc != 2 ) return 1;

    return ConsoleServer{ boost::lexical_cast<std::uint16_t>( argv[ 1 ] ) }.run( );
} catch( const std::exception& err ) {
    ::MessageBoxA( NULL, err.what( ),
                  "RemoteConsole", MB_OK | MB_ICONERROR );
}
