#include "ConsoleCommon.hpp"
#include "Utils.hpp"

void rmcmd::ConsoleCommon::updateScreenBufferInfo( ) {
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

    if(    !util::isMemEqual( bufferInfo.dwCursorPosition, bufferInfo_.dwCursorPosition )
        || !util::isMemEqual( bufferInfo.dwSize, bufferInfo_.dwSize )
        || !util::isMemEqual( bufferInfo.srWindow, bufferInfo_.srWindow )
        || !util::isMemEqual( bufferInfo.wAttributes, bufferInfo_.wAttributes ) )
    {
        bufferInfo_.dwCursorPosition = bufferInfo.dwCursorPosition;
        bufferInfo_.srWindow = bufferInfo.srWindow;
        bufferInfo_.wAttributes = bufferInfo.wAttributes;

        bufferInfo_.dwSize = bufferInfo.dwSize;

        sendRequest( { "SetScreenBufferInfo", bufferInfo_ } );
    }
}

void rmcmd::ConsoleCommon::updateConsoleCursorInfo( ) {
    CONSOLE_CURSOR_INFO cursorInfo;
    GetConsoleCursorInfo( hOut_, &cursorInfo );
    if( !util::isMemEqual( cursorInfo, cursorInfo_ ) ) {
        util::copyMemory( cursorInfo_, cursorInfo );
        sendRequest( { "SetConsoleCursorInfo", cursorInfo_ } );
    }
}

void rmcmd::ConsoleCommon::setConsoleCursorInfo( const CONSOLE_CURSOR_INFO cursorInfo ) {
    util::copyMemory( cursorInfo_, cursorInfo );
    ::SetConsoleCursorInfo( hOut_, &cursorInfo_ );
}
