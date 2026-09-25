#include "CashConsole.h"

#include "CashMath.h"
#include "CashRendering.h"
#include "CashSystem.h"

#include "SDL3/SDL.h"
#include <cstdint>

#include "ft2build.h"
#include FT_FREETYPE_H

typedef uint32_t Pos;
#define NOT_IMPLEMENTED FAIL

// STB TextEdit implementation defines
#define STB_TEXTEDIT_CHARTYPE char
#define STB_TEXTEDIT_POSITIONTYPE int
#define STB_TEXTEDIT_UNDOSTATECOUNT 32
#define STB_TEXTEDIT_UNDOCHARCOUNT 1024


//characters per row * rows * verts per character
#define VERTS_PER_CHARACTER 6
#define CHAR_PER_ROW 256
#define MAX_ROWS 128
#define MAX_VERTS CHAR_PER_ROW * MAX_ROWS * VERTS_PER_CHARACTER

// Include in header mode to get struct definitions
#include "stb/stb_textedit.h"
#define STB_RECT_PACK_IMPLEMENTATION
#include "stb/stb_rect_pack.h"

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


#define STB_TEXTEDIT_IMPLEMENTATION
#include "stb/stb_textedit.h"
#define STB_TRUETYPE_IMPLEMENTATION
#include "stb/stb_truetype.h"
//#define STB_RECT_PACK_IMPLEMENTATION
//#include "stb/stb_rect_pack.h"

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
    Vec2                        window_size;

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

    // Rendering
    Pipeline*                   pipeline = nullptr;
    StaticArray<Vertex_2D, MAX_VERTS> vertices = {};
    GpuBuffer*                  vertex_buffer = nullptr;
    //i32                         vertex_length = 0;
    UniformID                   uniform = 0;
    Texture*                    font_texture = nullptr;
};
static Console s_console;

static ArrayView<const char*> s_logo_string;
//static Vec2 s_font_size;

#define FONT_BITMAP_SIZE_X  512
#define FONT_BITMAP_SIZE_Y  512
#define FONT_BITMAP_SIZE_BYTES (FONT_BITMAP_SIZE_X * FONT_BITMAP_SIZE_Y * sizeof(u32))
#define FONT_CHAR_START 32
#define FONT_CHAR_FINAL_INDEX 1586
#define FONT_CHAR_COUNT (FONT_CHAR_FINAL_INDEX - FONT_CHAR_START)

static float s_font_ascender = 0;
static float s_font_descender = 0;

#if 1
static struct Glyph {
    SimpleRect uvs;
    Vec2 size;
    Vec2 offset;
    float advance_x;
} _s_char_data[FONT_CHAR_COUNT] = {};
Glyph& GetGlyph(u32 utf8_index)
{
    if (utf8_index < FONT_CHAR_START || utf8_index > FONT_CHAR_FINAL_INDEX)
    {
        FAIL;
        Log("CashConsole", LogLevel_Error, "Trying to get glyph that doesn't exist");
        return _s_char_data[0];
    }
    return _s_char_data[utf8_index - FONT_CHAR_START];
}
#else
static stbtt_packedchar s_char_data[FONT_CHAR_COUNT] = {};
#endif



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
    const Vec2 window_size = s_console.window_size;// GetWindowSize();

    SimpleRect console_rect;
    console_rect.left = 0.0f;
    console_rect.right = window_size.x;
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
    s_console.window_size = SysGetWindowSize();
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

void DrawRect(SimpleRect rect, Color color, const SimpleRect& scissor)
{
    const SimpleRect uv = {
        .left = 0,
        .bot = 0,
        .right = 1,
        .top = 1,
    };
    // 6 verts in a quad
    const Vertex_2D top_left  = { rect.TopLeft(),  color, uv.TopLeft()  }; //0 Top Left
    const Vertex_2D bot_left  = { rect.BotLeft(),  color, uv.BotLeft()  }; //1 Bot Left
    const Vertex_2D top_right = { rect.TopRight(), color, uv.TopRight() }; //2 Top Right
    const Vertex_2D bot_right = { rect.BotRight(), color, uv.BotRight() }; //3 Bot Right

    Vertex_2D verts[6] = {
        // First part of Quad
        top_left,
        bot_left,
        top_right,

        //Second part of quad
        top_right,
        bot_left,
        bot_right,
    };
    const i32 start_index = (i32)s_console.vertices.used;
    s_console.vertices.Add(CreateArrayView(verts));
    const i32 end_index = (i32)s_console.vertices.used;

    DrawCallParams draw = {};
    draw.pipeline = s_console.pipeline;
    draw.bindings = {};
    draw.bindings.vertex_buffer = s_console.vertex_buffer;
    draw.bindings.read_textures.Add(gfx.plain_texture);
    draw.bindings.samplers.Add(gfx.common_sampler);

    draw.vertex_index = start_index;
    draw.vertex_length = end_index - start_index + 1;
    draw.scissor = scissor;

    draw.color_actions[0] = { };
    draw.depth_action = { };
    draw.stencil_action = { };

    //ShaderConstants_Blit2D uniform = {
    //    .orthographic = gb_mat4_identity(),
    //};
    //UpdateUniform(s_console.uniform, CreateArrayView((u8*)&uniform, sizeof(uniform)), 0);
    draw.uniforms.Add(s_console.uniform);

    draw.color_targets[0] = { gfx.hdr_target };
    draw.depth_stencil_target = {};
    draw.draw_to_backbuffer = false;

    CreateDrawCall("Console Draw Text", draw);
}

void DrawText(const char* string, Vec2 bot_left_p, Color color, const SimpleRect& scissor)
{
    const i32 start_index = (i32)s_console.vertices.used;
    //Vec2 bot_left_inv = { bot_left_p.x, s_console.window_size.y - bot_left_p.y };
    size_t len = strlen(string);
    for (size_t i = 0; i < len; i++)
    {
        const char& c = string[i];
        SimpleRect uv;
        SimpleRect vert;
#if 1
        const Glyph& g = GetGlyph(c);

        // Calculate vertex positions
        // Note: Y is subtracted because FreeType's bearing_y goes UP from the baseline
        vert.left   = bot_left_p.x + g.offset.x;
        vert.right  = vert.left + g.size.x;
        vert.bot    = bot_left_p.y + (g.size.y - g.offset.y) - 2;
        vert.top    = vert.bot - g.size.y;

        // Advance the cursor for the next character
        bot_left_p.x += g.advance_x;

        uv = g.uvs;
        ASSERT(uv.Width() * FONT_BITMAP_SIZE_X == vert.Width());
        ASSERT(uv.Height() * FONT_BITMAP_SIZE_Y == vert.Height());
        //uv.left = g.uv0.x;
        //uv.right = g.uv1.x;
        //uv.top = g.uv0.y;
        //uv.bot = g.uv1.y;
#else
        stbtt_aligned_quad q;
        stbtt_GetPackedQuad(s_char_data, FONT_BITMAP_SIZE_X, FONT_BITMAP_SIZE_Y, c, &bot_left_inv.x, &bot_left_inv.y, &q, 1);
        uv.left = q.s0;
        uv.right = q.s1;
        uv.top = q.t0;
        uv.bot = q.t1;
        vert.left = q.x0;
        vert.right = q.x1;
        vert.bot = s_console.window_size.y - q.y1;
        vert.top = s_console.window_size.y - q.y0;
#endif

        //inverting hight so we match y+ is up
        //vert.top = s_console.window_size.y - q.y0;
        //vert.bot = q.y1;
        //vert.top = q.y1 + (q.y1 - q.y0);

        const Vertex_2D top_left  = { Round(vert.TopLeft()),  color, uv.TopLeft() }; //0 Top Left
        const Vertex_2D bot_left  = { Round(vert.BotLeft()),  color, uv.BotLeft() }; //1 Bot Left
        const Vertex_2D top_right = { Round(vert.TopRight()), color, uv.TopRight() }; //2 Top Right
        const Vertex_2D bot_right = { Round(vert.BotRight()), color, uv.BotRight() }; //3 Bot Right

        Vertex_2D verts[] = {
            // First part of Quad
            top_left,
            bot_left,
            top_right,

            //Second part of quad
            top_right,
            bot_left,
            bot_right,
        };
        s_console.vertices.Add(CreateArrayView(verts));
    }
    const i32 end_index = (i32)s_console.vertices.used;

    DrawCallParams draw = {};
    draw.pipeline = s_console.pipeline;
    draw.bindings = {};
    draw.bindings.vertex_buffer = s_console.vertex_buffer;
    draw.bindings.read_textures.Add(s_console.font_texture);
    draw.bindings.samplers.Add(gfx.common_sampler);

    draw.vertex_index = start_index;
    draw.vertex_length = end_index - start_index;
    draw.scissor = scissor;

    draw.color_actions[0] = { };
    draw.depth_action = { };
    draw.stencil_action = { };

    //ShaderConstants_Blit2D uniform = {
    //    .orthographic = gb_mat4_identity(),
    //};
    //UpdateUniform(s_console.uniform, CreateArrayView((u8*)&uniform, sizeof(uniform)), 0);
    draw.uniforms.Add(s_console.uniform);


    draw.color_targets[0] = { gfx.hdr_target };
    draw.depth_stencil_target = {};
    draw.draw_to_backbuffer = false;

    CreateDrawCall("Console Draw Text", draw);
}

void DrawString(Vec2 location, Color color, const SimpleRect& scissor, const char* text, ...)
{
    va_list count_args, write_args;
    va_start(count_args, text);
    va_copy(write_args, count_args);
    auto count = vsnprintf(nullptr, 0, text, count_args);
    va_end(count_args);

    if (count)
    {
        std::string buffer;
        buffer.resize(count);
        vsnprintf(&buffer[0], buffer.size() + 1, text, write_args);
        assert(*(buffer.data() + buffer.size()) == 0);
        DrawText(buffer.c_str(), location, color, scissor);
    }
}

void ConsoleRun()
{
    ConsoleCheckForInit();
    DrawString({ 0, s_console.font_height }, White, {}, "test");

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
    s_console.scroll_position = Lerp(s_console.scroll_position, s_console.scroll_target, lerp_t);
    //s_console.scroll_position = Clamp(s_console.scroll_position, 0.0f, NumItems());

    Console* console = &s_console;
    console->visible_height = TweenValue(console->tween);
    if (console->visible_height == 0 && console->delta == 0)
        return;

    //Font* font = AppDefaultFont();
    //FontSprite* font = ConsoleFont();
    SimpleRect empty_scissor = {};

    const SimpleRect log_rect = LogRect();
    //AddRectToRender(RenderType::DebugFill, log_rect, console_color, RenderPrio::Console, CoordinateSpace::UI);
    DrawRect(log_rect, console_color, empty_scissor);

    // Input rect
    const SimpleRect input_rect = InputRect();
    //AddRectToRender(RenderType::DebugFill, input_rect, input_color, RenderPrio::Console, CoordinateSpace::UI);
    DrawRect(input_rect, input_color, empty_scissor);

    const char* terminal_prompt = "> ";
    float charWidth = s_console.font_mono_width;// s_font_size.x * s_console.font_scale;
    float prompt_width = static_cast<float>(charWidth * strlen(terminal_prompt)); // font->StringWidth(terminal_prompt); // TODO:
    DrawString(input_rect.BotLeft(), font_color, empty_scissor, "%s%s", terminal_prompt, s_console.input_buf.c_str());

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

        DrawString(min, item.color, scissor_rect, "%s%s", item.preamble, item.text.c_str());
        min.y += ItemHeight();//font->AdvanceY();
    }

    // Scrollbar
    {
        SimpleRect scroll = ScrollBackgroundRect();
        //AddRectToRender(RenderType::DebugFill, scroll, scroll_background_color, RenderPrio::Console, CoordinateSpace::UI);
        DrawRect(scroll, scroll_background_color, scissor_rect);

        SimpleRect bar = ScrollbarRect();
        Color color = s_console.mouse_scrolling ? scroll_handle_active_color : scroll_handle_color;
        // The current scissor rect will still clip the y-coord here:
        //AddRectToRender(RenderType::DebugFill, bar, color, RenderPrio::Console, CoordinateSpace::UI);
        DrawRect(bar, color, scissor_rect);
    }

    //Upload data to gpu
    s_console.vertex_buffer->Upload(s_console.vertices);
    {
        ZoneScopedN("Clearing console vertices");
        s_console.vertices.Clear();
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
    Vec2 size = s_console.window_size;
    float target = large ? size.y * OPEN_LARGE : size.y * OPEN_STANDARD;
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

void Console_OnWindowSize(i32 width, i32 height)
{
    if (width * height == 0)
        return;
    ConsoleCheckForInit(); // Paranoid check to avoid divide by zero

    // Preserve the old ratio that the tween was targeting.
    float ratio = s_console.tween.v1 / s_console.window_size.y;
    s_console.tween.v1 = height * ratio;
    s_console.window_size = {
        static_cast<float>(width),
        static_cast<float>(height),
    };
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

bool Console_OnMouseWheel(float scroll)
{
    if (!ConsoleWantsInput())
        return false;
    if (!MouseIsOverConsole())
        return false;

    s_console.scroll_target += scroll * SCROLL_SPEED;
    return true;
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


void ConsoleInit(const ArrayView<const char*>& logo, ArrayView<const u8> console_font_data)
{
    AddInputHandler(&s_console_input);

    s_logo_string = logo;
    const Vec2 window_size = SysGetWindowSize();
    const Vec2 screen_size = SysGetScreenSize();
    //u8 temp_font_bitmap[FONT_BITMAP_SIZE_X][FONT_BITMAP_SIZE_Y] = {};
    ColorI* temp_font_bitmap = (ColorI*)malloc(FONT_BITMAP_SIZE_BYTES);
    memset(temp_font_bitmap, 0, FONT_BITMAP_SIZE_BYTES);
    Defer{ free(temp_font_bitmap); };


#if 1
    stbrp_context pack_ctx;
    stbrp_node pack_nodes[FONT_CHAR_COUNT];
    stbrp_init_target(&pack_ctx, FONT_BITMAP_SIZE_X, FONT_BITMAP_SIZE_Y, pack_nodes, FONT_CHAR_COUNT);

    FT_Library ft;
    VALIDATE_M(!FT_Init_FreeType(&ft), "ConsoleInit", LogLevel_Error, "%s", "Failed to create FreeType Library");
    FT_Face face;
    VALIDATE_M(!FT_New_Memory_Face(ft, console_font_data.data, (FT_Long)console_font_data.Bytes(), 0, &face), "ConsoleInit", LogLevel_Error, "%s", "Failed to create FreeType Memory Face");
    VALIDATE_M(!FT_Set_Pixel_Sizes(face, 0, (u32)s_console.font_height * s_console.font_scale), "ConsoleInit", LogLevel_Error, "%s", "Failed to set Pixel Sizes");

    s_font_ascender = (float)(face->size->metrics.ascender >> 6);
    s_font_descender = (float)(face->size->metrics.descender >> 6);
    //const float = (float)(face->size->metrics.height >> 6);

    stbrp_rect rects[FONT_CHAR_COUNT] = {};
    for (i32 i = 0; i < FONT_CHAR_COUNT; i++)
    {
        const i32 codepoint = FONT_CHAR_START + i;
        VALIDATE_M(!FT_Load_Char(face, codepoint, FT_LOAD_DEFAULT), "ConsoleInit", LogLevel_Error, "Failed to load char %i", codepoint);

        rects[i].id = codepoint;
        rects[i].w = face->glyph->bitmap.width;
        rects[i].h = face->glyph->bitmap.rows;
    }
    stbrp_pack_rects(&pack_ctx, rects, FONT_CHAR_COUNT);

    for (i32 i = 0; i < FONT_CHAR_COUNT; i++)
    {
        const i32 codepoint = FONT_CHAR_START + i;
        const stbrp_rect& rect = rects[i];
        if (!rect.was_packed)
            continue;

        FT_Int32 flags = FT_LOAD_RENDER | FT_LOAD_FORCE_AUTOHINT | FT_LOAD_TARGET_LCD;
#if _DEBUG 
        flags |= FT_LOAD_PEDANTIC;
#endif
        VALIDATE_M(!FT_Load_Char(face, codepoint, flags), "ConsoleInit", LogLevel_Error, "Failed to load char %i", codepoint);
        if (face->glyph->format != FT_GLYPH_FORMAT_BITMAP)
            FT_Render_Glyph(face->glyph, FT_RENDER_MODE_NORMAL);
        FT_Bitmap* bmp = &face->glyph->bitmap;

        //const i32 r = stbi_write_png("test.png", char_pix_width, char_pix_height, FONT_BITMAP_BYTE_PER_PIX, bmp->buffer, bmp->pitch);
        //ASSERT(r);

        const u32 dest_x = rect.x;
        const u32 dest_y = rect.y;
        const u32 src_x = 0;
        const u32 src_y = 0;
        const u32 char_pix_width  = rect.w;
        const u32 char_pix_height = rect.h;
        for (u32 row = 0; row < char_pix_height; ++row)
        {
            for (u32 col = 0; col < char_pix_width; ++col)
            {
                const u32 dest_pix_index    = ((dest_y + row) * FONT_BITMAP_SIZE_X) + (dest_x + col);
                //const i32 atlas_index = (start_y + row) + (start_x + col);
                //const i32 buffer_index = row * bmp->pitch + col;
                const u32 src_pix_index     = ((src_y + row) * (bmp->pitch))      + ((src_x + col) * 3);
                const u32 src_byte_index    = src_pix_index * 3;

                //ASSERT(atlas_index < FONT_BITMAP_SIZE_X * FONT_BITMAP_SIZE_Y);
                //ASSERT(buffer_index < bmp->rows * bmp->pitch);
                ColorI& dest = temp_font_bitmap[dest_pix_index];
                dest.r = bmp->buffer[src_pix_index + 0];
                dest.g = bmp->buffer[src_pix_index + 1];
                dest.b = bmp->buffer[src_pix_index + 2];
                dest.a = ((dest.r + dest.g + dest.b) / 3);
            }
        }

        Glyph& g = GetGlyph(codepoint);
        g.size.x = (float)rect.w;
        g.size.y = (float)rect.h;
        //g.size.x = (float)bitmap->width;
        //g.size.y = (float)bitmap->rows;
        g.offset.x = (float)face->glyph->bitmap_left;
        g.offset.y = (float)face->glyph->bitmap_top;
        // Advance is stored in 1/64th of a pixel, so bitshift right by 6 to get true pixels
        g.advance_x = (float)(face->glyph->advance.x >> 6);
        const i32 height = face->glyph->metrics.height >> 6;
        const i32 b_y = face->glyph->bitmap_top;

        g.uvs.left  = (float)dest_x / FONT_BITMAP_SIZE_X;
        g.uvs.top   = (float)dest_y / FONT_BITMAP_SIZE_Y;
        g.uvs.right = (float)(dest_x + char_pix_width) / FONT_BITMAP_SIZE_X;
        g.uvs.bot   = (float)(dest_y + char_pix_height) / FONT_BITMAP_SIZE_Y;
        //ASSERT(g.uvs.Width() && g.uvs.Height());
    }

    FT_Done_Face(face);
    FT_Done_FreeType(ft);
#else
    //STBTT_DEF int stbtt_PackBegin(stbtt_pack_context *spc, unsigned char *pixels, int pw, int ph, int stride_in_bytes, int padding, void *alloc_context)
//    stbtt_PackBegin();
//stbtt_PackSetOversampling()          -- for improved quality on small fonts
//stbtt_PackFontRanges()               -- pack and renders
//stbtt_PackEnd()
//stbtt_GetPackedQuad()

    stbtt_pack_context pc;
    if (!stbtt_PackBegin(&pc, (u8*)temp_font_bitmap, FONT_BITMAP_SIZE_X, FONT_BITMAP_SIZE_Y, 0, 1, nullptr))
    {
        DebugPrint("Font packing failed");
        FAIL;
        return;
    }

    stbtt_PackSetOversampling(&pc, 4, 4);
    if (!stbtt_PackFontRange(&pc,
        console_font_data.data,
        0,
        s_console.font_height,
        FONT_CHAR_START,
        FONT_CHAR_COUNT,
        s_char_data))
    {
        DebugPrint("Failed to build font");
        FAIL;
        return;
    }
    stbtt_PackEnd(&pc);
    const i32 char_index = 'M' - FONT_CHAR_START;
    const float monospace_width = s_char_data[char_index].xadvance;
    s_console.font_mono_width = monospace_width;
#endif

#if 1
    TextureParams tp = {
        .size = { FONT_BITMAP_SIZE_X, FONT_BITMAP_SIZE_Y, 0 },
        .msaa_samples = 1,
        .mip_count = 1,

        .dimension = TextureDimension_2D,
        .format = TextureFormat_RGBA8_UNORM_SRGB,
        .type = TextureType_Texture,
        .update = TextureUpdateType_Immutable,
    };
    ArrayView<u8> font_bitmap_byte_view = CreateArrayView((u8*)temp_font_bitmap, FONT_BITMAP_SIZE_BYTES);
    //const u64 size = font_bitmap_byte_view.Bytes() * 4;
    //ColorI* font_rgba8_data = (ColorI*)malloc(size);
    //for (i32 i = 0; i < font_bitmap_byte_view.Bytes(); i += FONT_BITMAP_BYTE_PER_PIX)
    //{
    //    font_rgba8_data[i].r = font_bitmap_byte_view[i + 0];
    //    font_rgba8_data[i].g = font_bitmap_byte_view[i + 1];
    //    font_rgba8_data[i].b = font_bitmap_byte_view[i + 2];
    //    font_rgba8_data[i].a = font_bitmap_byte_view[i + 0];
    //}
    //ArrayView<u8> font_array_view = CreateArrayView((u8*)font_rgba8_data, size);
    ////CreateTextureAndUpload(&s_console.font_texture, "Console Font", tp, font_array_view);
    CreateTextureAndUpload(&s_console.font_texture, "Console Font", tp, font_bitmap_byte_view);
#else
    TextureParams tp = {
        .size = { FONT_BITMAP_SIZE_X, FONT_BITMAP_SIZE_Y, 0 },
        .msaa_samples = 1,
        .mip_count = 1,

        .dimension = TextureDimension_2D,
        .format = TextureFormat_R8_SNORM,
        .type = TextureType_Texture,
        .update = TextureUpdateType_Immutable,
    };
    ArrayView<u8> font_bitmap_view = CreateArrayView((u8*)temp_font_bitmap, FONT_BITMAP_SIZE_X * FONT_BITMAP_SIZE_Y);
    CreateTextureAndUpload(&s_console.font_texture, "Console Font", tp, font_bitmap_view);
#endif


    {
        PipelineParams params = {
            .shader = gfx.blit2d_shader,
            .primitive_type = PrimitiveType_Triangles,
            .cull_mode = RenderCullMode_None,
            .msaa_sample_count = 1,
            .has_index_buffer = false,
            .front_ccw_winding_order = true,
            .alpha_to_coverage_enabled = false,

            //.depth,
            //.depth_compare_func,
            //.depth_bias = 0.0f,
            //.depth_bias_slope_scale = 0.0f,
            //.depth_bias_clamp = 0.0f,

            .stencil = gfx.stencil_2d,
        };

        params.targets[0] = {
            .texture = gfx.hdr_target,
            .blend = gfx.blend_normal,
        };
        //params.targets[0].blend.enabled = true;
        //params.targets[0].blend.src_factor_rgb = BlendFactor_SrcAlpha;
        //params.targets[0].blend.dst_factor_rgb = BlendFactor_OneMinusSrcAlpha;
        //params.targets[0].blend.op_rgb      = BlendOp_Add;
        //params.targets[0].blend.src_factor_alpha = BlendFactor_One;
        //params.targets[0].blend.dst_factor_alpha = BlendFactor_Zero;
        //params.targets[0].blend.op_alpha    = BlendOp_Add;
        params.targets[0].mask = ColorMask_RGB;

        CreatePipeline(&s_console.pipeline, "Console Pipeline", params);
    }
    CreateGpuBuffer(&s_console.vertex_buffer, "Console Vertex Buffer", GpuBufferType_Vertex, GpuBufferFlag_StreamUpdate, MAX_VERTS * sizeof(Vertex_2D));

    const float display_pos_x = 0;
    const float display_pos_y = 0;
    const float L = display_pos_x;
    const float R = display_pos_x + gfx.window_size.x;
    const float T = display_pos_y;
    const float B = display_pos_y + gfx.window_size.y;

    const Vec4 full = { 1024, 600, 0, 1 };
    const Vec4 half = {  512, 300, 0, 1 };
    const Vec4 zero = {    0,   0, 0, 1 };
    const Vec4 t    = {    7,   9, 0, 1 };

    ShaderConstants_Blit2D uniform = {
        .orthographic = {
             2.0f/(R-L),   0.0f,           0.0f,       0.0f,
             0.0f,         2.0f/(T-B),     0.0f,       0.0f,
             0.0f,         0.0f,           0.5f,       0.0f,
             (R+L)/(L-R),  (T+B)/(B-T),    0.5f,       1.0f,
    },
    };

    gb_mat4_transpose(uniform.orthographic);

    //TODO(CSH): Delete
    const Vec4 full_r = uniform.orthographic * full;
    const Vec4 half_r = uniform.orthographic * half;
    const Vec4 zero_r = uniform.orthographic * zero;
    const Vec4 t_r    = uniform.orthographic * t;

    s_console.uniform = CreateUniform(CreateArrayView((u8*)&uniform, sizeof(uniform)), 0);

    ConsoleCheckForInit();
}

