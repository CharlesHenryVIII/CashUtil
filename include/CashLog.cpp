#include "CashLog.h"
//#include "CashRendering.h"
#include "CashSystem.h"
//#include "Resource.h"
#include "CashConsole.h"

#include "SDL3/SDL.h"
#include <cstdint>






//========================
//       LOGGING
//========================

static const char* s_log_level_str[LogLevel_Count] = {
    "Info____",
    "Warning_",
    "Error___",
    "Internal",
};

void LogInternal(const char* category, const LogLevel level, const char* message)
{
    SDL_Time ticks;
    SDL_GetCurrentTime(&ticks);
    SDL_DateTime dt;
    SDL_TimeToDateTime(ticks, &dt, true);

    char time_str[32] = {};
#if 1
    Sprintf(time_str, sizeof(time_str), "%02d:%02d:%02d",
            dt.hour, dt.minute, dt.second);
#else
    Sprintf(time_str, sizeof(time_str), "%04d-%02d-%02d %02d:%02d:%02d",
            dt.year, dt.month, dt.day, dt.hour, dt.minute, dt.second);
#endif

#if 1
    //                 time|frame|verb|cat|message
    const char* fmt = "[%s][%3llu][%s]%s: %s";

    DebugPrint(         fmt, time_str, g_frame_index, s_log_level_str[level], category, message);
    ConsoleLog(level,   fmt, time_str, g_frame_index, s_log_level_str[level], category, message);
#else
    //                                  time|frame|verb|cat|message
    const std::string str = std::format("[{}][{:3}][{}]{}: {}",
        time_str, g_frame_index, s_log_level_str[level], category, message);

    DebugPrint("%s", str.c_str());
    ConsoleLog(level, "%s", str.c_str());
#endif
}

void Log(const char* category, LogLevel level, const char* fmt, ...)
{
    va_list list;
    va_start(list, fmt);
    char format_string[4096] = {};
    Sprintf(format_string, arrsize(format_string), fmt, list);
    va_end(list);

    LogInternal(category, level, format_string);
}

void Log(const char* category, u32 line, const LogLevel level, const char* fmt, ...)
{
    va_list list;
    va_start(list, fmt);
    char format_string[Kibibytes(4)] = {};
    const i32 end = Sprintf(format_string, arrsize(format_string), fmt, list);
    va_end(list);

    char filenumber[256] = {};
    Sprintf(filenumber, sizeof(filenumber), "%s(%d)", category, line);
    LogInternal(filenumber, level, format_string);
}

//void Log(const wchar_t* category, const LogLevel level, const wchar_t* fmt, ...)
//{
//    va_list list;
//    va_start(list, fmt);
//    wchar_t buffer[4096] = {};
//    SYS_VSNWPRINTF(buffer, arrsize(buffer), fmt, list);
//    va_end(list);
//
//    std::string cat;
//    std::string message;
//    SysConvertWideCharToMultiByte(cat, category);
//    SysConvertWideCharToMultiByte(message, buffer);
//    LogInternal(cat, level, message);
//}
