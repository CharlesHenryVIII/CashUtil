#pragma once

#include "CashMath.h"
#include "CashArrayView.h"
#include "CashString.h"
#include <vector>
#include <functional>

#define CONSOLE_FUNCTION(name) void name ()
typedef CONSOLE_FUNCTION((*CommandFunc));

#define CONSOLE_FUNCTIONA(name) void name (const std::vector<std::string>& args)
typedef CONSOLE_FUNCTIONA((*CommandFuncArgs));


//AddRectToRender(RenderType::DebugFill, log_rect, console_color, RenderPrio::Console, CoordinateSpace::UI);
using Console_FuncDrawRect = std::function<void(Rect, Color)>;
//DrawText(ConsoleFont(), color, s_console.font_scale, { i32(location.x), i32(location.y) }, UIX::left, UIY::bot, RenderPrio::Console, buffer.c_str());
                             //string, bot_left position, color, font scale
using Console_FuncDrawText = std::function<void(const char*, Vec2, Color, float)>;
//PushScissor(scissor_rect)
using Console_FuncPushScissor = std::function<void(Rect)>;
using Console_FuncPopScissor  = std::function<void(void)>;

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
void ConsoleInit(const std::string& logo, ArrayView<const u8> console_font_data);
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


