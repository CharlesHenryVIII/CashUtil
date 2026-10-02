#include "CashConsole.h"

#include "CashMath.h"
#include "CashRendering.h"
#include "CashSystem.h"
#include "Resource.h"

#include "SDL3/SDL.h"
#include <cstdint>

typedef uint32_t Pos;
#define NOT_IMPLEMENTED FAIL

// STB TextEdit implementation defines
#define STB_TEXTEDIT_CHARTYPE char
#define STB_TEXTEDIT_POSITIONTYPE int
#define STB_TEXTEDIT_UNDOSTATECOUNT 32
#define STB_TEXTEDIT_UNDOCHARCOUNT 1024

#include "stb/stb_textedit.h"

typedef std::string TextEditString;

static void  stbLayoutRow(StbTexteditRow* row, TextEditString* string, Pos pos);
static float stbCharWidth(TextEditString* string, int start, int i);
static int   stbKeyToText(int c);
static void  stbDeleteChars(TextEditString* string, int start, int count);
static bool  stbInsertChars(TextEditString* string, int start, char* characters, int count);


#define STB_TEXTEDIT_STRING                     TextEditString
#define STB_TEXTEDIT_STRINGLEN(obj)             ((int)((obj)->size()))
#define STB_TEXTEDIT_LAYOUTROW(r, obj, n)       stbLayoutRow(r, obj, n)
#define STB_TEXTEDIT_GETWIDTH(obj,n,i)          stbCharWidth(obj, n, i)
#define STB_TEXTEDIT_KEYTOTEXT(k)               stbKeyToText(k)
#define STB_TEXTEDIT_GETCHAR(obj,i)             ((*obj)[i])
#define STB_TEXTEDIT_NEWLINE                    ('\n')
#define STB_TEXTEDIT_DELETECHARS(obj,i,n)       stbDeleteChars(obj, i, n)
#define STB_TEXTEDIT_INSERTCHARS(obj,i,c,n)     stbInsertChars(obj, i, c, n)


#define STB_TEXTEDIT_K_SHIFT       (SDL_KMOD_SHIFT << 16)
#define STB_TEXTEDIT_K_LEFT        SDLK_LEFT
#define STB_TEXTEDIT_K_RIGHT       SDLK_RIGHT
#define STB_TEXTEDIT_K_UP          SDLK_UP
#define STB_TEXTEDIT_K_DOWN        SDLK_DOWN
#define STB_TEXTEDIT_K_LINESTART   SDLK_HOME
#define STB_TEXTEDIT_K_LINEEND     SDLK_END
#define STB_TEXTEDIT_K_TEXTSTART   (SDLK_HOME | (SDL_KMOD_CTRL << 16))
#define STB_TEXTEDIT_K_TEXTEND     (SDLK_END| (SDL_KMOD_CTRL  << 16))
#define STB_TEXTEDIT_K_DELETE      SDLK_DELETE
#define STB_TEXTEDIT_K_BACKSPACE   SDLK_BACKSPACE
#define STB_TEXTEDIT_K_UNDO        (SDLK_Z | (SDL_KMOD_CTRL << 16))
#define STB_TEXTEDIT_K_REDO        (SDLK_Z | (SDL_KMOD_CTRL << 16) | (SDL_KMOD_SHIFT << 16))
//NOTE(CSH): Using vim keybinds for this:
#define STB_TEXTEDIT_K_PGDOWN      (SDLK_J | (SDL_KMOD_CTRL << 16))
#define STB_TEXTEDIT_K_PGUP        (SDLK_K | (SDL_KMOD_CTRL << 16))

// Optional:
#define STB_TEXTEDIT_K_INSERT              SDLK_INSERT
#define STB_TEXTEDIT_IS_SPACE(ch)          ((ch) == ' ')
//    STB_TEXTEDIT_MOVEWORDLEFT(obj,i)   custom handler for WORDLEFT, returns index to move cursor to
//    STB_TEXTEDIT_MOVEWORDRIGHT(obj,i)  custom handler for WORDRIGHT, returns index to move cursor to
#define STB_TEXTEDIT_K_WORDLEFT            SDLK_LEFT  | (SDL_KMOD_CTRL << 16)
#define STB_TEXTEDIT_K_WORDRIGHT           SDLK_RIGHT | (SDL_KMOD_CTRL << 16)
//    STB_TEXTEDIT_K_LINESTART2          secondary keyboard input to move cursor to start of line
//    STB_TEXTEDIT_K_LINEEND2            secondary keyboard input to move cursor to end of line
//    STB_TEXTEDIT_K_TEXTSTART2          secondary keyboard input to move cursor to start of text
//    STB_TEXTEDIT_K_TEXTEND2            secondary keyboard input to move cursor to end of text

// Include in header mode to get struct definitions
#define STB_TEXTEDIT_IMPLEMENTATION
#include "stb/stb_textedit.h"

// CONFIG:
static const float OPEN_TIME          = 0.5f;  // Seconds to fully open the console
static const float OPEN_STANDARD      = 0.3f;  // Screen ratio when opening with `
static const float OPEN_LARGE         = 0.8f;  // Screen ratio when opening with shift+`
static const float SCROLL_SPEED       = 3.0f;  // Number of lines to jump with the mouse scroll wheel
static const float CARET_BLINK        = 0.5f;  // Input cursor blink time
static const float SCROLLBAR_WIDTH    = 15.0f; // Width of the clickable scrollbar region
static const float DEFAULT_FONT_SCALE = 0.5f;  // Controls the size of the bitmap font. TODO: Could switch to setting a pixel height, which would be better for TTF anyway.

static const Color console_color              = Color{0.1f, 0.1f, 0.1f, 0.95f};
static const Color input_color                = Color{0.2f, 0.2f, 0.2f, 0.95f};
static const Color caret_color                = Color{0.8f, 0.8f, 0.8f, 0.6f};
static const Color selection_color            = Color{0.8f, 0.8f, 0.8f, 0.4f};
static const Color font_color                 = Color{0.5f, 0.9f, 0.5f, 1.0f};
static const Color scroll_background_color    = Color{0.5f, 0.5f, 0.5f, 0.8f};
static const Color scroll_handle_color        = Color{0.7f, 0.7f, 1.0f, 0.8f};
static const Color scroll_handle_active_color = Color{0.8f, 0.8f, 1.0f, 1.0f};

static const Color log_colors[] = {
    Color{0.9f, 0.9f, 0.9f, 1.0f}, // LogLevel_Info
    Color{0.9f, 0.9f, 0.2f, 1.0f}, // LogLevel_Warning
    Color{0.9f, 0.1f, 0.1f, 1.0f}, // LogLevel_Error
    Color{0.5f, 0.5f, 0.8f, 1.0f}, // LogLevel_Internal
};
static_assert(arrsize(log_colors) == LogLevel_Count);
static_assert(LogLevel_Count == 4);
const char* log_level_str[LogLevel_Count] = {
    "Info____",
    "Warning_",
    "Error___",
    "Internal",
};

std::vector<char*> logStrings;
std::vector<LogLevel> logLevels;
// :CONFIG


struct ConsoleCommand
{
    char            name[128];
    CommandFunc     callback;
    CommandFuncArgs callback_args;
};

struct ConsoleItem
{
    LogLevel     level;
    Color        color;
    std::string  text;
    char         preamble[4] = {};
};

struct Console
{
    bool                        initialized = false;

    TextEditString              input_buf;
    std::vector<ConsoleItem>    items;
    std::vector<std::string>    history;
    int                         history_pos = 0;    // -1: new line, 0..History.Size-1 browsing history.
    LogLevel                    log_level = LogLevel_Info;

    std::vector<ConsoleCommand> commands;

    float                       visible_height = 0;
    float                       delta = 0;
    Tween                       tween = {};
    Tween                       caret_tween;

    float                       font_height = 16;// = font_scale = DEFAULT_FONT_SCALE;
    float                       font_scale  = 1;// = font_scale = DEFAULT_FONT_SCALE;
    float                       font_mono_width = -1;
    FontID                      font = {};
    Vec2I                       window_size;

    // Scrollbar
    float                       scroll_position = 0.0f;
    float                       scroll_target = 0.0f;
    float                       mouse_scroll_handle_t = 0.0f;
    bool                        mouse_scrolling = false;

    // Autocomplete
    int                         ac_index;
    std::vector<std::string>    ac_current_matches;
    bool                        ac_active;
    // The string we had before starting autocomplete (hitting backspace returns to this)
    std::string                 ac_pre_string;

    //ConsoleInputHandler*        input_handler = nullptr;
    bool                        wants_input;

    // STB:
    STB_TexteditState           te_state = {};
};
static Console s_console;

static ArrayView<const char*> s_logo_string;
//static Vec2 s_font_size;


static Color LogColor(LogLevel level)
{
    i32 index = Clamp<i32>(level, 0, arrsize(log_colors));
    assert(index == level); // Request was out of bounds
    return log_colors[index];
}


static void ConsoleClearInput()
{
    s_console.input_buf.clear();
    s_console.te_state.select_end = 0;
    s_console.te_state.select_start = 0;
    s_console.te_state.cursor = 0;
}

static SimpleRect ConsoleRect()
{
    const Vec2I window_size = s_console.window_size;// GetWindowSize();

    SimpleRect console_rect;
    console_rect.left = 0.0f;
    console_rect.right = float(window_size.x);
    console_rect.top = 0.0f;
    console_rect.bot = s_console.visible_height;
    return console_rect;
}

static float ItemHeight()
{
    return s_console.font_height;// s_font_size.y* s_console.font_scale;
    //return AppDefaultFont()->AdvanceY();
}

// The input rect is the bottom portion of the console rect
static SimpleRect InputRect()
{
    const SimpleRect console_rect = ConsoleRect();
    const float line_height = ItemHeight();
    const SimpleRect input_rect = {
        .left   = console_rect.left,
        .bot    = console_rect.bot,
        .right  = console_rect.right,
        .top    = console_rect.bot - line_height,
    };
    return input_rect;
}

static SimpleRect LogRect()
{
    const SimpleRect console_rect = ConsoleRect();
    const float line_height = ItemHeight();
    SimpleRect log_rect = {
        .left   = console_rect.left,
        .bot    = console_rect.bot - line_height,
        .right  = console_rect.right,
        .top    = console_rect.top,
    };
    return log_rect;
}

static float NumVisibleItems()
{
    SimpleRect log_rect = LogRect();
    float visible_items = std::fabsf(log_rect.Height()) / ItemHeight();
    return visible_items;
}

static float NumItems()
{
    return static_cast<float>(s_console.items.size());
}

static float MaxScroll()
{
    float max_scroll = Max(NumItems() - NumVisibleItems(), 0.0f);
    return max_scroll;
}

static SimpleRect ScrollBackgroundRect()
{
    SimpleRect log_rect = LogRect();
    float visible_items = NumVisibleItems();
    if (NumItems() < visible_items)
        return {};

    Vec2 min = log_rect.BotLeft();
    Vec2 max = log_rect.TopRight();
    min.x = max.x - SCROLLBAR_WIDTH;

    SimpleRect result;
    result.BotLeft() = min;
    result.TopRight() = max;
    return result;
}

static SimpleRect ScrollbarRect()
{
    SimpleRect result = {};

    SimpleRect log_rect = ScrollBackgroundRect();
    float visible_items = NumVisibleItems();
    float items = NumItems();
    if (items == 0 || items < visible_items)
        return {};

    float min_height = 10.0f;
    float visible_ratio = visible_items / items;
    float size = std::fabsf(log_rect.Height()) * visible_ratio;
    float height = Max(size, min_height);

    Vec2 min = log_rect.BotLeft();
    Vec2 max = log_rect.TopRight();

    // Positioning
    // We have the position calculated for the end, do a lerp from the start
    float top = max.y + height;
    min.y = Lerp(min.y, top, s_console.scroll_target / MaxScroll());
    max.y = min.y - height;

    result.BotLeft() = min;
    result.TopRight() = max;
    return result;
}

static void sConsoleLog(LogLevel level, const char* string, const char* preamble = nullptr, Color* opt_color = nullptr)
{
    Color color = opt_color != nullptr ? *opt_color : LogColor(level);
    s_console.items.push_back(ConsoleItem{ level, color, std::string(string) });
    if (preamble && preamble[0])
    {
        ConsoleItem& item = s_console.items.back();
        StringCopy(item.preamble, preamble);
    }

    // NOTE: This keeps the scroll position the same if we are not scrolled to the bottom, but doesn't handle multiple
    // lines being inserted at the same time.
    const float MIN_VALUE = 0.01f;
    float max_scroll = MaxScroll();
    if (s_console.scroll_target > max_scroll)
    {
        s_console.scroll_target = max_scroll;
    }
    else if (s_console.scroll_target >= MIN_VALUE)
    {
        s_console.scroll_target += 1.0f;
        s_console.scroll_position += 1.0f;
    }
}

static void AddLog(const char* preamble, const char* fmt, ...)
{
    char buf[1024];
    va_list args;
    va_start(args, fmt);
    vsnprintf(buf, arrsize(buf), fmt, args);
    buf[arrsize(buf) - 1] = 0;
    va_end(args);
    sConsoleLog(LogLevel_Internal, buf, preamble, nullptr);
}

static void ConsoleClearAutoComplete()
{
    s_console.ac_index = -1;
    s_console.ac_current_matches.clear();
    s_console.ac_active = false;
    s_console.ac_pre_string.clear();
}

static void ConsoleMoveAutoCompleteIndex(bool next_alphabetically)
{
    Console* console = &s_console;

    if (next_alphabetically)
    {
        console->ac_index = (console->ac_index + 1) % static_cast<int>(console->ac_current_matches.size());
    }
    else
    {
        console->ac_index -= 1;
        if (console->ac_index < 0)
        {
            console->ac_index = int(console->ac_current_matches.size()) - 1;
        }
    }

    console->input_buf = console->ac_current_matches[console->ac_index];
    console->te_state.cursor = static_cast<int>(console->input_buf.length());
}

static void ConsoleBeginAutocomplete()
{
    Console* console = &s_console;
    std::string& current = console->input_buf;
    int current_len = static_cast<int>(current.length());

    std::vector<std::string> all_matches;
    const int matches_per_line = 6;
    int current_matches = 0;
    for (auto& command : console->commands)
    {

        if (StringCompare(StringCase_Insensitive, current.c_str(), command.name, current_len))
        {
            if (current_matches++ % matches_per_line == 0)
            {
                all_matches.push_back("");
            }

            std::string& line = all_matches.back();
            console->ac_current_matches.push_back(command.name);
            line += " " + console->ac_current_matches.back();
        }
    }

    if (console->ac_current_matches.size() >= 1)
    {
        if (console->ac_current_matches.size() > 1)
        {
            console->ac_pre_string = current;
            console->ac_active = true;

            AddLog("@ ", "Matches:");
            for (auto& line : all_matches)
                AddLog("   ", "%s", line.c_str());
        }
        else
        {
            console->ac_pre_string.clear();
        }

        console->ac_index = -1;
        ConsoleMoveAutoCompleteIndex(true);
    }
    else
    {
        AddLog("! ", "No matches for %s", current.c_str());
    }
}

void ExecCommand(const char* command_line)
{
    command_line = StringEatWhitespace(command_line);
    if (command_line[0] == 0)
        return;
    s_console.scroll_target = 0.0f;
    AddLog("# ", "%s", command_line);

    // Insert into history. First find match and delete it so it can be pushed to the back. This isn't trying to be smart or optimal.
    s_console.history_pos = -1;
    for (int i = static_cast<int>(s_console.history.size()); i--;)
    {
        if (StringCompare(StringCase_Insensitive, s_console.history[i].c_str(), command_line))
        {
            s_console.history.erase(s_console.history.begin() + i);
            break;
        }
    }
    s_console.history.push_back(command_line);

    char command[512] = {};
    {
        const char* tc = command_line;
        int len = StringGetToken(command, &tc);
        if (!len)
            return;
    }

    for (auto& cmd : s_console.commands)
    {

        if (StringCompare(StringCase_Insensitive, cmd.name, command))
        {
            if (cmd.callback_args)
            {
                std::vector<std::string> args;
                char arg[512] = {};
                const char* command_arguments = StringEatWordAndWhitespace(command_line);
                while (StringGetToken(arg, arrsize(arg), &command_arguments))
                    args.push_back(arg);
                cmd.callback_args(args);
            }
            else if (cmd.callback)
            {
                cmd.callback();
            }
            else
            {
                assert(!"No function was registered with the command");
            }
            return;
        }
    }
    ConsoleLog(LogLevel_Error, "Unknown command: %s", command);
}

CONSOLE_FUNCTION(ShowHelp)
{
    for (auto& command : s_console.commands)
    {
        ConsoleLog(LogLevel_Internal, command.name);
    }
}

CONSOLE_FUNCTION(ConsoleClear)
{
    s_console.items.clear();
}

//CONSOLE_FUNCTIONA(ConsoleFontScale)
//{
//    float scale = DEFAULT_FONT_SCALE;
//    if (args.size() >= 1)
//        scale = static_cast<float>(atof(args[0].c_str()));
//
//    s_console.font_scale = std::clamp(scale, 0.25f, 1.0f);
//    ConsoleLog(LogLevel_Info, "Setting console font scale to %0.3f", s_console.font_scale);
//}

CONSOLE_FUNCTION(Logo)
{
    Color logo_color = Mint;

    auto WriteLine = [&logo_color](const char* line) {
        sConsoleLog(LogLevel_Internal, line, nullptr, &logo_color);
    };

    for (u64 i = 0; i < s_logo_string.count; i++)
    {
        WriteLine(s_logo_string[i]);
    }
}

void ConsoleCheckForInit()
{
    if (s_console.initialized)
        return;
    const Vec2I window_size = SysGetWindowSize();
    if (window_size != s_console.window_size)
        Console_OnWindowSize(window_size);

    s_console.window_size = window_size;
    s_console.initialized = true;
    s_console.items.reserve(1000);
    ConsoleAddCommand("help", ShowHelp);
    ConsoleAddCommand("clear", ConsoleClear);
    //TODO(CSH):
    //ConsoleAddCommand("font_scale", ConsoleFontScale);
    ConsoleAddCommand("logo", Logo);

    stb_textedit_initialize_state(&s_console.te_state, true);

    s_console.caret_tween = TweenBegin(TweenStyle_InverseSquare, CARET_BLINK, 0.1f, 1.0f);

    if (0)
    {
        // Silence unused function warnings
        stb_textedit_click(nullptr, nullptr, 0, 0);
        stb_textedit_drag(nullptr, nullptr, 0, 0);
    }

    Logo();

    assert(logStrings.size() == logLevels.size());
    for (int i = 0; i < logStrings.size(); i++)
    {
        sConsoleLog(logLevels[i], logStrings[i]);
        free(logStrings[i]);
    }
    logLevels.clear();
    logStrings.clear();
}

void ConsoleRun()
{
    ConsoleCheckForInit();

    if (s_console.mouse_scrolling)
    {
        float pos = SysGetMousePosition().y;
        SimpleRect scroll = ScrollBackgroundRect();
        SimpleRect handle = ScrollbarRect();
        float height = std::fabsf(handle.Height());

        // Consider a region that doesn't include the handle for the mouse scroll so that it is fixed to the initial click position.
        float top_delete = height * (1.0f - s_console.mouse_scroll_handle_t);
        float bot_delete = height * s_console.mouse_scroll_handle_t;
        scroll.top += top_delete;
        scroll.bot -= bot_delete;

        float t = (pos - scroll.bot) / scroll.Height();
        s_console.scroll_target = gb_clamp01(t) * MaxScroll();
    }

    const float lerp_t = 0.2f;
    if (s_console.scroll_target < 0)
        s_console.scroll_target = Lerp(s_console.scroll_target, 0.0f, lerp_t);
    float max_scroll = MaxScroll();
    if (s_console.scroll_target > max_scroll)
        s_console.scroll_target = Lerp(s_console.scroll_target, max_scroll, lerp_t);
#if 1
    //scroll past the bottom
    s_console.scroll_position = Lerp(s_console.scroll_position, s_console.scroll_target, lerp_t);
#else
    //don't scroll past bottom
    s_console.scroll_position = Clamp(s_console.scroll_position, 0.0f, NumItems());
#endif

    Console* console = &s_console;
    console->visible_height = TweenValue(console->tween);
    if (console->visible_height == 0 && console->delta == 0)
        return;

    //Font* font = AppDefaultFont();
    //FontSprite* font = ConsoleFont();
    SimpleRect empty_scissor = {};

    const SimpleRect log_rect = LogRect();
    DrawRect(log_rect, console_color, empty_scissor);

    // Input rect
    const SimpleRect input_rect = InputRect();
    DrawRect(input_rect, input_color, empty_scissor);

    const char* terminal_prompt = "> ";
    float charWidth = s_console.font_mono_width;// s_font_size.x * s_console.font_scale;
    float prompt_width = static_cast<float>(charWidth * strlen(terminal_prompt)); // font->StringWidth(terminal_prompt); // TODO:
    DrawString(input_rect.BotLeft(), font_color, s_console.font, empty_scissor, "%s%s", terminal_prompt, s_console.input_buf.c_str());

    STB_TexteditState& state = s_console.te_state;

    if (state.select_end == state.select_start)
    {
        float alpha = TweenValue(s_console.caret_tween);
        if (s_console.caret_tween.finished)
        {
            std::swap(s_console.caret_tween.v0, s_console.caret_tween.v1);
            s_console.caret_tween.start_time = SysGetTime();
            s_console.caret_tween.finished = false;
            // Flipping style keeps it bright for most of the time:
            if (s_console.caret_tween.v0 < s_console.caret_tween.v1)
                s_console.caret_tween.style = TweenStyle_InverseSquare;
            else
                s_console.caret_tween.style = TweenStyle_Square;
        }

        // Nothing selected, draw the cursor
        float caret_x = static_cast<float>(state.cursor * charWidth);// TODO: font->StringWidth(s_console.input_buf.c_str(), state.cursor);
        caret_x += prompt_width;
        SimpleRect caret;
        caret.left  = caret_x - 1.0f;
        caret.right = caret_x + 1.0f;

        float center_y = (input_rect.bot + input_rect.top) / 2.0f;
        caret.bot = center_y - ItemHeight() * 0.5f;
        caret.top = center_y + ItemHeight() * 0.5f;
        Color c = caret_color;
        c.a = alpha;
        DrawRect(caret, input_color, empty_scissor);
    }
    else
    {
        // Draw selection rectangle
        int start = Min(state.select_start, state.select_end);
        int end = Max(state.select_start, state.select_end);

        float select_start = static_cast<float>(start * charWidth); // TODO: font->StringWidth(s_console.input_buf.c_str(), start);
        float select_end = static_cast<float>(end * charWidth); // TODO: font->StringWidth(s_console.input_buf.c_str(), end);
        select_start += prompt_width;
        select_end += prompt_width;
        float select_width = select_end - select_start;
        SimpleRect selection;
        selection.left = select_start;
        selection.right = select_end;

        float center_y = (input_rect.bot + input_rect.top) / 2.0f;
        selection.bot = center_y - ItemHeight() * 0.5f;
        selection.top = center_y + ItemHeight() * 0.5f;

        DrawRect(selection, selection_color, empty_scissor);
    }

    // Draw log
    const float text_x_offset = 2.0f;
    Vec2 min = log_rect.BotLeft();
    min.x += text_x_offset;

    // Scissor coordinates need to be framebuffer relative: 0, 0 is the bottom left;
    SimpleRect scissor_rect;
    scissor_rect.left = Min(log_rect.left, log_rect.right);
    scissor_rect.bot = s_console.window_size.y - Max(log_rect.bot, log_rect.top);
    scissor_rect.right = Max(log_rect.left, log_rect.right);
    scissor_rect.top = s_console.window_size.y - Min(log_rect.bot, log_rect.top);

    for (size_t i = 0; i < s_console.items.size(); i++)
    {
        float y_offset = float((NumItems() - 1) - i) * ItemHeight() - s_console.scroll_position * ItemHeight();
        min.y = log_rect.bot - y_offset;
        auto& item = s_console.items[i];

        if (min.y < 0.0f)
            continue;
        else if (min.y > log_rect.bot + ItemHeight())
            continue;

        DrawString(min, item.color, s_console.font, scissor_rect, "%s%s", item.preamble, item.text.c_str());
        min.y += ItemHeight();//font->AdvanceY();
    }

    // Scrollbar
    {
        SimpleRect scroll = ScrollBackgroundRect();
        DrawRect(scroll, scroll_background_color, scissor_rect);

        SimpleRect bar = ScrollbarRect();
        Color color = s_console.mouse_scrolling ? scroll_handle_active_color : scroll_handle_color;
        // The current scissor rect will still clip the y-coord here:
        DrawRect(bar, color, scissor_rect);
    }
}

void ConsoleSetLogLevel(LogLevel level)
{
    s_console.log_level = Clamp(level, LogLevel(0), LogLevel(LogLevel_Count - 1));
}

void AddLogToList(char* buf, size_t size, LogLevel level = LogLevel_Info)
{
    char* copy = (char*)malloc(size);
    memcpy(copy, buf, size);

    logStrings.push_back(copy);
    logLevels.push_back(level);
}

void ConsoleLog(LogLevel level, const char* fmt, ...)
{
    char buf[1024];
    va_list args;
    va_start(args, fmt);
    vsnprintf(buf, arrsize(buf), fmt, args);
    buf[arrsize(buf) - 1] = 0;
    va_end(args);

    if (s_console.initialized)
        sConsoleLog(level, buf);
    else
        AddLogToList(buf, sizeof(buf), level);
    DebugPrint("%s\n", buf);
}

void ConsoleLog(const char* fmt, ...)
{

    char buf[1024];
    va_list args;
    va_start(args, fmt);
    vsnprintf(buf, arrsize(buf), fmt, args);
    buf[arrsize(buf) - 1] = 0;
    va_end(args);

    if (s_console.initialized)
        sConsoleLog(LogLevel_Info, buf);
    else
        AddLogToList(buf, sizeof(buf));
    DebugPrint("%s\n", buf);
}

static ConsoleCommand& AddCommand(const char* name)
{
    ConsoleCheckForInit();
    ConsoleCommand command = {};
    strcpy_s(command.name, name);

    auto it = s_console.commands.begin();
    for (; it != s_console.commands.end(); ++it)
    {
        if (_strcmpi(name, it->name) < 0)
        {
            break;
        }
    }

    auto result = s_console.commands.insert(it, command);
    return *result;
}

void ConsoleAddCommand(const char* name, CommandFunc func)
{
    assert(name && func);
    ConsoleCommand& command = AddCommand(name);
    command.callback = func;
}

void ConsoleAddCommand(const char* name, CommandFuncArgs func)
{
    assert(name && func);
    ConsoleCommand& command = AddCommand(name);
    command.callback_args = func;
}

void ConsoleClose()
{
    Console* console = &s_console;
    console->tween = TweenBegin(TweenStyle_InverseCube, OPEN_TIME, console->visible_height, 0.0f);
    s_console.wants_input = false;
    ConsoleClearInput();
    ConsoleClearAutoComplete();
}

static float TargetHeight(bool large)
{
    const Vec2I size = s_console.window_size;
    const float target = large ? size.y * OPEN_LARGE : size.y * OPEN_STANDARD;
    return target;
}

void ConsoleOpen(bool large)
{
    float target = TargetHeight(large);
    s_console.tween = TweenBegin(TweenStyle_InverseCube, OPEN_TIME, s_console.visible_height, target);
    s_console.wants_input = true;
}

static bool FloatEquals(float a, float b, float epsilon = FLT_EPSILON)
{
    float abs_diff = std::fabsf(b - a);
    return abs_diff < epsilon;
}

void ConsoleToggle(bool shift_pressed)
{
    float cur = s_console.tween.v1;
    bool is_open = cur > 0.0f;
    bool is_large = FloatEquals(cur, TargetHeight(true));

    if (!is_open)
        ConsoleOpen(shift_pressed);
    else if (shift_pressed)
        ConsoleOpen(!is_large);
    else
        ConsoleClose();
}

//
// STB helper functions:
//

// NOTE: Used for mouse input, probably don't need/want it.
static void stbLayoutRow(StbTexteditRow* row, TextEditString* string, Pos pos)
{
    UNUSED(row); UNUSED(string); UNUSED(pos);
    NOT_IMPLEMENTED;
}

static float stbCharWidth(TextEditString* string, int start, int i)
{
    UNUSED(string); UNUSED(start); UNUSED(i);
    // Return the size of the character, takking Kerning into account
    NOT_IMPLEMENTED;
    return 1.0f;
}

static int stbKeyToText(int c)
{
    if (c >= SDLK_SPACE && c <= SDLK_TILDE)
        return c;
    return -1;
}

static void stbDeleteChars(TextEditString* string, int start, int count)
{
    string->erase(size_t(start), size_t(count));
}

static bool stbInsertChars(TextEditString* string, int start, char* characters, int count)
{
    string->insert(size_t(start), characters, size_t(count));
    return true;
}

static void CycleHistory(bool reverse)
{
    Console* console = &s_console;

    if (console->ac_active)
    {
        ConsoleMoveAutoCompleteIndex(reverse);
    }
    else
    {
        const int prev_history_pos = console->history_pos;

        if (reverse)
        {
            if (console->history_pos != -1)
                if (++console->history_pos >= (int)console->history.size())
                    console->history_pos = -1;
        }
        else
        {
            if (console->history_pos == -1)
                console->history_pos = static_cast<int>(console->history.size()) - 1;
            else if (console->history_pos > 0)
                console->history_pos--;
        }

        // A better implementation would preserve the data on the current input line along with cursor position.
        if (prev_history_pos != console->history_pos)
        {
            if (console->history_pos == -1)
                console->input_buf = "";
            else
                console->input_buf = console->history[console->history_pos];
            console->te_state.select_end = console->te_state.select_start = 0;
            console->te_state.cursor = static_cast<int>(console->input_buf.size());
        }
    }
}

static void CopySelection()
{
    STB_TexteditState* state = &s_console.te_state;
    if (state->select_start != state->select_end)
    {
        size_t start = Min<size_t>(state->select_start, state->select_end);
        size_t end = Max<size_t>(state->select_start, state->select_end);
        std::string text = s_console.input_buf.substr(start, end - start);
        SDL_SetClipboardText(text.c_str());
    }
}





//========================
//       LOGGING
//========================

void LogInternal(const std::string& category, const LogLevel level, const std::string& message)
{
    SDL_Time ticks;
    SDL_GetCurrentTime(&ticks);
    SDL_DateTime dt;
    SDL_TimeToDateTime(ticks, &dt, true);

    char time_str[32] = {};
#if 1
    snprintf(time_str, sizeof(time_str), "%02d:%02d:%02d",
             dt.hour, dt.minute, dt.second);
#else
    snprintf(time_str, sizeof(time_str), "%04d-%02d-%02d %02d:%02d:%02d",
             dt.year, dt.month, dt.day, dt.hour, dt.minute, dt.second);
#endif

    //                                  time|frame|verb|cat|message
    const std::string str = std::format("[{}][{:3}][{}]{}: {}",
        time_str, g_frame_index, log_level_str[level], category, message);

    DebugPrint("%s", str.c_str());
    ConsoleLog(level, "%s", str.c_str());
}
void Log(const char* category, const LogLevel level, const char* fmt, ...)
{
    va_list list;
    va_start(list, fmt);
    char format_string[4096] = {};
    SYS_VSNPRINTF(format_string, arrsize(format_string), fmt, list);
    va_end(list);

    LogInternal(category, level, format_string);
}
void Log(const wchar_t* category, const LogLevel level, const wchar_t* fmt, ...)
{
    va_list list;
    va_start(list, fmt);
    wchar_t buffer[4096] = {};
    SYS_VSNWPRINTF(buffer, arrsize(buffer), fmt, list);
    va_end(list);

    std::string cat;
    std::string message;
    SysConvertWideCharToMultiByte(cat, category);
    SysConvertWideCharToMultiByte(message, buffer);
    LogInternal(cat, level, message);
}

//
// Input Handler
//

bool ConsoleWantsInput()
{
    return s_console.wants_input;
}

void Console_OnWindowSize(Vec2I size)
{
    if (size.x * size.y == 0 || size == s_console.window_size)
        return;
    //ConsoleCheckForInit(); // Paranoid check to avoid divide by zero

    // Preserve the old ratio that the tween was targeting.
    float ratio = 0.0f;
    if (s_console.window_size.y != 0.0f)
        ratio = s_console.tween.v1 / s_console.window_size.y;
    s_console.tween.v1 = size.y * ratio;
    s_console.window_size = size;
}

static bool Contains(const SimpleRect r, const Vec2 point)
{
    bool x = point.x > r.left && point.x < r.right;
    bool y = point.y < r.bot && point.y > r.top;
    return x && y;
}

static bool MouseIsOverConsole()
{
    Vec2 pos = SysGetMousePosition();
    SimpleRect console_rect = ConsoleRect();
    return Contains(console_rect, pos);
}

static struct ConsoleInputHandler : InputHandler
{
    virtual InputPriority Priority() override { return InputPriority_Console; }

    virtual bool OnKeyDown(const SDL_KeyboardEvent& event) override
    {

        const SDL_Keymod    mods    = event.mod;
        const SDL_Keycode   key     = event.key;
        const bool          control = FlagIntersects(mods, SDL_KMOD_CTRL);
        const bool          shift   = FlagIntersects(mods, SDL_KMOD_SHIFT);
        const bool          alt     = FlagIntersects(mods, SDL_KMOD_ALT);
        if (key == SDLK_GRAVE)
        {
            ConsoleToggle(shift);
            return true;
        }
        if (!ConsoleWantsInput()) return false;
        if (event.repeat) return true;
        //We do not want to process text keys here but we need to consume them
        if (stbKeyToText(key) >= 0) return true;

        const STB_TEXTEDIT_KEYTYPE key_mod = key | (mods << 16);

        STB_TexteditState* state = &s_console.te_state;
        bool clear_autocomplete = false;

        //STB_TEXTEDIT_KEYTYPE mods = 0;
        //STB_TEXTEDIT_KEYTYPE stb_mods = (mods << 16);

        if (key == SDLK_RETURN || key == SDLK_KP_ENTER)
        {
            ExecCommand(s_console.input_buf.c_str());
            clear_autocomplete = true;
            ConsoleClearInput();
        }
        else if (control && key == SDLK_A)
        {
            state->select_start = 0;
            state->select_end = STB_TEXTEDIT_STRINGLEN(&s_console.input_buf);
            clear_autocomplete = true;
        }
        else if (control && (key == SDLK_BACKSPACE || key == SDLK_W))
        {
            stb_textedit_key(&s_console.input_buf, state, SDLK_LEFT | (SDL_KMOD_SHIFT << 16) | (SDL_KMOD_CTRL << 16));
            stb_textedit_key(&s_console.input_buf, state, SDLK_BACKSPACE);
            clear_autocomplete = true;
        }
        else if (s_console.ac_active && key == SDLK_BACKSPACE)
        {
            s_console.input_buf = s_console.ac_pre_string;
            s_console.te_state.cursor = static_cast<int>(s_console.input_buf.length());
            clear_autocomplete = true;
        }
        else if (control && key == SDLK_DELETE)
        {
            stb_textedit_key(&s_console.input_buf, state, SDLK_RIGHT | (SDL_KMOD_SHIFT << 16) | (SDL_KMOD_CTRL << 16));
            stb_textedit_key(&s_console.input_buf, state, SDLK_DELETE);
            clear_autocomplete = true;
        }
        else if (control && (key == SDLK_C || key == SDLK_X))
        {
            CopySelection();
            if (key == SDLK_X)
                stb_textedit_cut(&s_console.input_buf, state);
        }
        else if (control && key == SDLK_V)
        {
            if (const char* clip_text = SDL_GetClipboardText())
            {
                stb_textedit_paste(&s_console.input_buf, state, clip_text, static_cast<int>(strlen(clip_text)));
            }

            clear_autocomplete = true;
        }
        else if (control && (key == SDLK_HOME || key == SDLK_END))
        {
            if (key == SDLK_HOME)
                s_console.scroll_target = MaxScroll();
            else
                s_console.scroll_target = 0.0f;
            clear_autocomplete = true;
        }
        else if (key == SDLK_UP || key == SDLK_DOWN)
        {
            CycleHistory(key == SDLK_DOWN);
        }
        else if (key == SDLK_PAGEDOWN || key == SDLK_PAGEUP)
        {
            float offset = NumVisibleItems();
            s_console.scroll_target += offset * ((key == SDLK_PAGEDOWN) ? -1.0f : 1.0f);
            s_console.scroll_target = Clamp(s_console.scroll_target, 0.0f, MaxScroll());
        }
        else if (key == SDLK_TAB)
        {
            if (s_console.ac_active)
                ConsoleMoveAutoCompleteIndex(!shift);
            else
                ConsoleBeginAutocomplete();
        }
        else
        {
            stb_textedit_key(&s_console.input_buf, state, key);
        }

        if (clear_autocomplete)
            ConsoleClearAutoComplete();

        return false;
    }
    virtual void OnKeyUp(const SDL_KeyboardEvent& event) override { }
    virtual bool OnTextInput(const SDL_TextInputEvent& event) override
    {
        if (!event.text)            return false;
        if (!ConsoleWantsInput())   return false;

        const u64 len = strlen(event.text);
        for (i32 i = 0; i < len; i++)
        {
            const char c = event.text[i];
            if (c == '`' || c == '~') return true;
            STB_TEXTEDIT_KEYTYPE key = c;
            stb_textedit_key(&s_console.input_buf, &s_console.te_state, key);
        }
        ConsoleClearAutoComplete();
        return true;
    }

    virtual bool OnMouseMotion(const SDL_MouseMotionEvent& event) override { return false; }
    virtual bool OnMouseDown(const SDL_MouseButtonEvent& event) override
    {
        const bool pressed = event.down;
        const u8 button = event.button;
        if (button == SDL_BUTTON_LEFT && !pressed)
            s_console.mouse_scrolling = false;

        if (!ConsoleWantsInput())
            return false;
        if (!MouseIsOverConsole())
            return false;

        if (button == SDL_BUTTON_LEFT && pressed)
        {
            const Vec2 pos = { event.x, event.y };//GetMousePosition();
            const SimpleRect rect = ScrollbarRect();
            if (Contains(rect, pos))
            {
                s_console.mouse_scrolling = true;
                s_console.mouse_scroll_handle_t = (pos.y - rect.bot) / rect.Height();
            }
        }

        return true;
    }
    virtual void OnMouseUp(const SDL_MouseButtonEvent& event) override { }
    virtual bool OnMouseWheel(const SDL_MouseWheelEvent& event) override
    {
        if (!ConsoleWantsInput())
            return false;
        if (!MouseIsOverConsole())
            return false;

        s_console.scroll_target += event.y * SCROLL_SPEED;
        return true;
    }
} s_console_input;


void ConsoleInit(const ArrayView<const char*>& logo)
{
    AddInputHandler(&s_console_input);

    s_logo_string = logo;

    i32 console_font_size = 0;
    u8* jetbrainsmono_data = (u8*)SysGetDataFromResource(&console_font_size, IDR_FONT_JETBRAINSMONO);
    ArrayView<u8> jetbrainsmon_array = CreateArrayView(jetbrainsmono_data, console_font_size);
    s_console.font = CreateFont("JetBrainsMono", jetbrainsmon_array, (u32)(s_console.font_height * s_console.font_scale));
    ConsoleCheckForInit();
}

