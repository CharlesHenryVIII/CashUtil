#pragma once
#include "CashMath.h"






//========================
//       LOGGING
//========================

enum LogLevel : i32
{
    LogLevel_Info,
    LogLevel_Warning,
    LogLevel_Error,
    LogLevel_Internal,
    LogLevel_Count,
};
ENUMOPS_PURE(LogLevel);


//void Log(const wchar_t* category, const LogLevel level, const wchar_t*  fmt, ...);
void Log(const char*    category,                   LogLevel level, const char* fmt, ...);
void Log(const char*    category, u32 line_number,  LogLevel level, const char* fmt, ...);
#define LOG(_level, ...) Log(__FILENAME__, __LINE__, _level, __VA_ARGS__)
