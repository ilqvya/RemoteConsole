#include "boost/process.hpp"
#include "boost/lexical_cast.hpp"
#include "ConsoleProcess.hpp"
#include "ConsoleCommon.hpp"
#include "Rpc.hpp"
#include "RpcRequest.hpp"
#include "RpcHeader.hpp"

using namespace rmcmd;

namespace {
    class ConsoleClient final: public rmcmd::ConsoleCommon {
    public:
        ConsoleClient( std::uint16_t port ): ConsoleCommon{ port } {

            static auto self = this;
            SetConsoleCtrlHandler( [ ] ( DWORD CtrlType ) -> BOOL {
                switch( CtrlType ) {
                    case CTRL_C_EVENT:
                    self->sendRequestInNetworkThread( { "CTRL_C" } );
                    break;
                    case CTRL_BREAK_EVENT:
                    self->sendRequestInNetworkThread( { "CTRL_BREAK" } );
                    break;
                }
                return TRUE;
            }, TRUE );

            hIn_ = CreateFile( L"CONIN$", GENERIC_READ | GENERIC_WRITE, FILE_SHARE_READ, 0, OPEN_EXISTING, 0, 0 );
            hOut_ = CreateFile( L"CONOUT$", GENERIC_READ | GENERIC_WRITE, FILE_SHARE_READ, 0, OPEN_EXISTING, 0, 0 );

            bindRpcFunc( "SetScreenBufferInfo", [ this ] ( const RpcRequest& req ) {
                auto bufferInfo = req.param<CONSOLE_SCREEN_BUFFER_INFO>( 0 );
                setScreenBufferInfo( bufferInfo );
            } );

            bindRpcFunc( "FinishFirstUpdate", [ this ] ( const RpcRequest& req ) {
                isServerSentFirstUpdate_ = true;
            } );

            bindRpcFunc( "UpdateScreenBuffer", [ this ] ( const RpcRequest& req ) {
                setScreenBuffer( req.param<std::vector<std::pair<CHAR_INFO, COORD>>>( 0 ) );
            } );

            bindRpcFunc( "SetConsoleMode", [ this ] ( const RpcRequest& req ) {
                SetConsoleMode( hIn_, req.param<DWORD>( 0 ) );
            } );

            bindRpcFunc( "SetConsoleTitle", [ this ] ( const RpcRequest& req ) {
                ::SetConsoleTitle( (L"RemoteConsole" + req.param<std::wstring>( 0 )).c_str( ) );
            } );

            bindRpcFunc( "SetConsoleCursorInfo", [ this ] ( const RpcRequest& req ) {
                setConsoleCursorInfo( req.param<CONSOLE_CURSOR_INFO>( 0 ) );
            } );
        }

        ~ConsoleClient( ) {
            CloseHandle( hIn_ );
            CloseHandle( hOut_ );
        }
    private:
        bool isServerSentFirstUpdate_{ false };

        void update( ) override {
            if( isServerSentFirstUpdate_ ) {
                updateScreenBufferInfo( );
                updateConsoleInput( );
                updateConsoleCursorInfo( );
            }
        }

        void updateConsoleInput( ) {
            DWORD inputNumber{ 0u };

            if( !GetNumberOfConsoleInputEvents( hIn_, &inputNumber ) )
                throw std::runtime_error{ "GetNumberOfConsoleInputEvents error" };

            if( inputNumber != 0 ) {
                std::vector<INPUT_RECORD> inputRecords{ inputNumber, INPUT_RECORD{ { 0 } } };

                DWORD cNumReaded{ 0 };

                if( !ReadConsoleInput( hIn_, inputRecords.data( ), inputNumber, &cNumReaded ) )
                    throw std::runtime_error{ "ReadConsoleInput error" };

                sendRequest( { "writeConsoleInput", inputRecords } );
            }
        }

        void setScreenBufferInfo( const CONSOLE_SCREEN_BUFFER_INFO& bufferInfo ) {
            if( !util::isMemEqual( bufferInfo_.dwSize, bufferInfo.dwSize ) )
                SetConsoleScreenBufferSize( hOut_, bufferInfo.dwSize );

            if( !util::isMemEqual( bufferInfo_.dwCursorPosition, bufferInfo.dwCursorPosition ) )
                SetConsoleCursorPosition( hOut_, bufferInfo.dwCursorPosition );

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

            bufferInfo_.dwCursorPosition = bufferInfo.dwCursorPosition;
            bufferInfo_.dwSize = bufferInfo.dwSize;
            bufferInfo_.srWindow = bufferInfo.srWindow;
            bufferInfo_.wAttributes = bufferInfo.wAttributes;
        }

        void setScreenBuffer( std::vector<std::pair<CHAR_INFO, COORD>>&& screenBuffer ) {
            for( auto& elem : screenBuffer ) {
                SMALL_RECT srctReadRect;

                srctReadRect.Top = elem.second.Y;    // top left: row 0, col 0 
                srctReadRect.Left = elem.second.X;
                srctReadRect.Bottom = elem.second.Y;
                srctReadRect.Right = elem.second.X;

                if( !WriteConsoleOutput( hOut_, &elem.first, { 1, 1 }, { 0, 0 }, &srctReadRect ) ) {
                    ::MessageBoxA( NULL, ( "1WriteConsoleOutput fail with code: " +
                                   std::to_string( GetLastError( ) ) ).c_str( ), "", 0 );
                }
            }
        }
    };
}

int main( int argc, char* argv[ ] ) try {
    if( argc != 2 ) return 1;

    return ConsoleClient{ boost::lexical_cast<std::uint16_t>( argv[ 1 ] ) }.run( );
} catch( const std::exception& err ) {
    ::MessageBoxA( NULL, err.what( ),
                   "RemoteConsole", MB_OK | MB_ICONERROR );
}
