#pragma once

#include "CashMath.h"
#include "CashArrayView.h"
#include "CashString.h"
#include <vector>
//#include <functional>

#define CONSOLE_FUNCTION(name) void name ()
typedef CONSOLE_FUNCTION((*CommandFunc));

#define CONSOLE_FUNCTIONA(name) void name (const std::vector<std::string>& args)
typedef CONSOLE_FUNCTIONA((*CommandFuncArgs));


enum LogLevel : i32
{
    LogLevel_Info,
    LogLevel_Warning,
    LogLevel_Error,
    LogLevel_Internal,
    LogLevel_Count,
};
ENUMOPS_PURE(LogLevel);

//void ConsoleInit(const std::string& logo, const Vec2 font_size, Console_FuncDrawRect* DrawRect, Console_FuncDrawText* DrawText, Console_FuncPushScissor* PushScissor, Console_FuncPopScissor* PopScissor);
void ConsoleInit(const ArrayView<const char*>& logo);
void ConsoleRun();
void ConsoleLog(LogLevel level, const char* fmt, ...);
void ConsoleLog(const char* fmt, ...);
void ConsoleSetLogLevel(LogLevel level);
void ConsoleAddCommand(const char* name, CommandFunc func);
void ConsoleAddCommand(const char* name, CommandFuncArgs func);

void Console_OnWindowSize(Vec2I size);

void Log(const char*    category, const LogLevel level, const char*     fmt, ...);
void Log(const wchar_t* category, const LogLevel level, const wchar_t*  fmt, ...);
#define LOG(_level, ...) Log(__FILENAME__, _level, __VA_ARGS__)
