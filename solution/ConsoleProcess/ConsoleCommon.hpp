#pragma once


#include "ConsoleProcess.hpp"

namespace rmcmd {
    class ConsoleCommon: public ConsoleProcess {
    public:
        using ConsoleProcess::ConsoleProcess;
    protected:
        virtual void updateScreenBufferInfo( );

        CONSOLE_CURSOR_INFO cursorInfo_ = { { 0 } };
        void updateConsoleCursorInfo( );
        void setConsoleCursorInfo( const CONSOLE_CURSOR_INFO cursorInfo );
    protected:
        CONSOLE_SCREEN_BUFFER_INFO bufferInfo_;

        HANDLE hIn_{ nullptr }, hOut_{ nullptr };

        WORD oldColor_{ 0u };
    };
}