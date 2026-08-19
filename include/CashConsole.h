#pragma once

#include "CashMath.h"
#include <vector>
#include <string>

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

void ConsoleInit(const std::string& logo);
void ConsoleRun();
void ConsoleLog(LogLevel level, const char* fmt, ...);
void ConsoleLog(const char* fmt, ...);
void ConsoleSetLogLevel(LogLevel level);
void ConsoleAddCommand(const char* name, CommandFunc func);
void ConsoleAddCommand(const char* name, CommandFuncArgs func);

bool Console_OnCharacter(i32 c);
bool Console_OnKeyboard(i32 c, i32 mods, bool pressed, bool repeat);
bool Console_OnMouseButton(i32 button, bool pressed);
bool Console_OnMouseWheel(float scroll);
void Console_OnWindowSize(i32 width, i32 height);

