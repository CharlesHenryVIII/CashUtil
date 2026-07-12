#include "System.h"
#include "Rendering.h"
#include <array>


#if WIN32
#include "OSWindows.h"
#endif

#include "ImGui/backends/imgui_impl_sdl3.h"
#include "ImGui/backends/imgui_impl_sdlrenderer3.h"

SysInfo g_sysinfo;

bool SysInit(SDL_Window* window)
{
    return OSInit(window);
}
void SysDestroy(SDL_Window* window)
{
    OSDestroy(window);
}

void* SysGetWindowHandle(SDL_Window* window)
{
    return OSGetWindowHandle(window);
}


void SysDebugPrintDirect(const char* fmt, ...)
{
    va_list list;
    va_start(list, fmt);
    char buffer[4096];
    vsnprintf(buffer, sizeof(buffer), fmt, list);
    OSDebugOutput(buffer);
    OSDebugOutput("\n");
    va_end(list);

    OSWriteToAttachedConsole(buffer, false);
}

void DebugPrint(const char* fmt, ...)
{
    va_list list;
    va_start(list, fmt);
    char buffer[4096] = {};
    vsnprintf_s(buffer, arrsize(buffer), _TRUNCATE, fmt, list);
    OSDebugOutput(buffer);
    OSDebugOutput("\n");
    va_end(list);

    OSWriteToAttachedConsole(buffer, true);
}
void DebugPrint(const wchar_t* fmt, ...)
{
    va_list list;
    va_start(list, fmt);
    wchar_t buffer[4096] = {};
    _vsnwprintf_s(buffer, arrsize(buffer), _TRUNCATE, fmt, list);
    OSDebugOutput(buffer);
    va_end(list);

    OSWriteToAttachedConsole(buffer, true);
}

bool SysIsConsoleAttached()
{
    return OSIsConsoleAttached();
}
bool SysIsDebuggerAttached()
{
    return OSIsDebuggerAttached();
}
void SysHideConsole()
{
    OSHideConsole();
}
void SysShowConsole()
{
    OSShowConsole();
}
bool SysIsConsoleVisible()
{
    return OSIsConsoleVisible();
}

i32 SysRunShellProcess(const wchar_t* path, const wchar_t* args, std::string* output, Mutex* output_lock, RunProcessFlags flags)
{
    return OSRunShellProcess(path, args, output, output_lock, flags);
}
i32 SysRunProcess(const char* path, const char* args, AsyncData<std::string>* output, AsyncData<Path>* output_file, RunProcessFlags flags)
{
    const std::string p = path ? path : "";
    const std::string a = args ? args : "";
    return SysRunProcess(p, a, output, output_file, flags);
}
i32 SysRunProcess(const wchar_t* path, const wchar_t* args, AsyncData<std::string>* output, AsyncData<Path>* output_file, RunProcessFlags flags)
{
    const std::wstring pathw = path ? path : L"";
    const std::wstring argsw = args ? args : L"";
    return SysRunProcess(pathw, argsw, output, output_file, flags);
}
i32 SysRunProcess(const std::string& path, const std::string& args, AsyncData<std::string>* output, AsyncData<Path>* output_file, RunProcessFlags flags)
{
    std::wstring wpath;
    std::wstring wargs;
    SysConvertMultibyteToWideChar(wpath, path);
    SysConvertMultibyteToWideChar(wargs, args);
    return SysRunProcess(wpath, wargs, output, output_file, flags);
}
i32 SysRunProcess(const std::wstring& path, const std::wstring& args, AsyncData<std::string>* output, AsyncData<Path>* output_file, RunProcessFlags flags)
{
    return OSRunProcess(path, args, output, output_file, flags);
}



bool SysGetNetworkAdapters(std::vector<SysNetworkAdapterInfo>& out_adapters)
{
    ZoneScoped;
    return OSGetNetworkAdapters(out_adapters);
}
bool SysHasAdminPrivledge()
{
    ZoneScoped;
    return OSHasAdminPrivledge();
}

bool SysSetNetAdapterIP(const std::string& adapter_guid, const SysNetAdapterConfig& adapter, const SysNetAdapterConfig& src_adapter)
{
    ZoneScoped;
    return OSSetNetAdapterIP(adapter_guid, adapter, src_adapter);
}
bool SysSetNetAdapterDNS(const std::string& adapter_guid, const SysNetAdapterConfig& adapter, const SysNetAdapterConfig& src_adapter)
{
    ZoneScoped;
    return OSSetNetAdapterDNS(adapter_guid, adapter, src_adapter);
}

void SysAssert(bool expr, const char* message, const char* file, int line)
{
    OsAssert(expr, message, file, line);
}
void SysSleep(u64 _ms)
{
    ZoneScoped;
    std::this_thread::sleep_for(std::chrono::milliseconds(_ms));
}


double SysGetTime()
{
    const static double freq = double(SDL_GetPerformanceFrequency()); //HZ
    double time = SDL_GetPerformanceCounter() / freq;
    return time;
}
float SysMonitorScale()
{
    ZoneScoped;
    const static float scale = SDL_GetDisplayContentScale(SDL_GetPrimaryDisplay());
    return scale;
}

void ParseCSV(PowershellResponse& out, const std::string& in, bool using_quotes)
{
    ZoneScoped;
    if (!in.size())
    {
        FAIL;
        return;
    }
    const std::vector<std::string> rows = TextToStringArray(in.c_str(), "\n");
    if (!rows.size())
    {
        FAIL;
        return;
    }

    for (i32 row_i = 0; row_i < rows.size(); row_i++)
    {
        const std::string& row = rows[row_i];
        std::vector<std::string> strings;
        if (using_quotes)
            strings = TextCsvToStringArray(row.c_str());
        else
            strings = TextToStringArray(row.c_str(), ",");
        if (!strings.size())
            continue;
        out.emplace_back();
        if (strings.size() >= PWSH_MAX_COLUMNS)
        {
            FAIL;
            continue;
        }
        for (i32 i = 0; i < strings.size(); i++)
        {
            if (strings[i].size())
            {
                std::string& s = out[out.size() - 1][i];
                s = strings[i];
                if (using_quotes)
                    continue;
#if 0
                //This seems to be marginally faster?
                if (i == strings.size() - 1)
                    s = strings[i].substr(0, strings[i].size() - 2);
                else
                    s = strings[i].substr(0, strings[i].size() - 1);
#else
                s = strings[i];
                //TextRemoval(s, "\"");
                TextRemoval(s, ",");
                TextRemoval(s, "\r");
                TextRemoval(s, "\n");
                StringRemoveTrailing(s, ' ');
                StringRemoveLeading(s, ' ');
#endif
            }
        }
    }
}

void SysShowErrorWindow(const std::wstring& title, const std::wstring& text)
{
    OSShowErrorWindow(title, text);
}

void SysFlashWindow(SDL_Window* window)
{
    OSFlashWindow(window);
}

i32 SysShowCustomErrorWindow(const std::string& title, const std::string& text)
{
    FAIL;
    //const SDL_MessageBoxButtonData buttons[] = {
    //    { 0,                                        MessageBoxResponse_Quit, "Quit Program" },
    //    { SDL_MESSAGEBOX_BUTTON_ESCAPEKEY_DEFAULT,  MessageBoxResponse_Continue, "Continue" },
    //    { SDL_MESSAGEBOX_BUTTON_RETURNKEY_DEFAULT,  MessageBoxResponse_OpenLog, "Open Log" },
    //};
    //const SDL_MessageBoxColorScheme colorScheme = {
    //    { /* .colors (.r, .g, .b) */
    //        /* [SDL_MESSAGEBOX_COLOR_BACKGROUND] */
    //        { 255,   0,   0 },
    //        /* [SDL_MESSAGEBOX_COLOR_TEXT] */
    //        {   0, 255,   0 },
    //        /* [SDL_MESSAGEBOX_COLOR_BUTTON_BORDER] */
    //        { 255, 255,   0 },
    //        /* [SDL_MESSAGEBOX_COLOR_BUTTON_BACKGROUND] */
    //        {   0,   0, 255 },
    //        /* [SDL_MESSAGEBOX_COLOR_BUTTON_SELECTED] */
    //        { 255,   0, 255 }
    //    }
    //};
    //const SDL_MessageBoxData messageboxdata = {
    //    //SDL_MESSAGEBOX_INFORMATION, /* .flags */
    //    //SDL_MESSAGEBOX_ERROR,
    //    SDL_MESSAGEBOX_WARNING,
    //    NULL, /* .window */
    //    title.c_str(), /* .title */
    //    text.c_str(), /* .message */
    //    SDL_arraysize(buttons), /* .numbuttons */
    //    buttons, /* .buttons */
    //    &colorScheme /* .colorScheme */
    //};
    //i32 buttonID = -1;
    //if (SDL_ShowMessageBox(&messageboxdata, &buttonID) < 0) {
    //    SDL_Log("error displaying message box");
    //    //Quit Program
    //    SDL_Event e;
    //    e.type = SDL_QUIT;
    //    e.quit.timestamp = 0;
    //    SDL_PushEvent(&e);
    //    return 0;
    //}
    ////TODO: Add better error handling for this?
    //ASSERT(buttonID >= 0);

    //if (buttonID == MessageBoxResponse_Quit)
    //{
    //    SDL_Event e;
    //    e.type = SDL_QUIT;
    //    e.quit.timestamp = 0;
    //    SDL_PushEvent(&e);
    //}
    //return buttonID;
    return 1;
}

void SysScanDirectoryForFileNames(const std::wstring& dir, ScannedFiles& out, ScanDirectoryFlags flags)
{
    OSScanDirectoryForFileNames(dir, out, flags);
}

bool SysGetDirectoryFromUser(const std::wstring& currentDir, std::wstring& dir)
{
    return OSGetDirectoryFromUser(currentDir, dir);
}

void SysConvertMultibyteToWideChar(std::wstring& out, const std::string& in)
{
    OSConvertMultibyteToWideChar(out, in);
}
void SysConvertWideCharToMultiByte(std::string& out, const std::wstring& in)
{
    OSConvertWideCharToMultiByte(out, in);
}

void SysExpandEnvironemntVariable(std::wstring& out, const std::wstring& in)
{
    OSExpandEnvironemntVariable(out, in);
}

void* SysGetDataFromResource(i32* out_size, const i32 resource_id)
{
    return OSGetDataFromResource(out_size, resource_id);
}

ImFont* SysLoadFontForImgui(int resource_id, float fontSize)
{
    i32 size;
    void* data = OSGetDataFromResource(&size, resource_id);
    if (!data || size == 0)
        return nullptr;

    ImFontConfig cfg;
    cfg.FontDataOwnedByAtlas = false;
    ImFont* font = ImGui::GetIO().Fonts->AddFontFromMemoryTTF(
        data,
        size,
        fontSize,
        &cfg
    );
    if (!font)
        return nullptr;
    return font;
}



std::string Guid::ToString() const
{
    return ::ToString("%08X-%04X-%04X-%04X-%04X%08X", a, b >> 16, b & 0XFFFF, c >> 16, c & 0XFFFF, d);
}

Guid GuidFromString(const char* s)
{
    Guid r = {};
    const size_t char_len = strnlen_s(s, 38);
    if (char_len != 36)
    {
        FAIL;
        return r;
    }

    char b[8] = { s[ 9], s[10], s[11], s[12], s[14], s[15], s[16], s[17] };
    char c[8] = { s[19], s[20], s[21], s[22], s[24], s[25], s[26], s[27] };
    r.a = (u32)strtoll(s,      nullptr, 16);
    r.b = (u32)strtoll(b,      nullptr, 16);
    r.c = (u32)strtoll(c,      nullptr, 16);
    r.d = (u32)strtoll(&s[28], nullptr, 16);

    return r;
}

Guid SysNewGuid()
{
    return OSNewGuid();
}


void SysProcessEvents()
{
    // Poll and handle events (inputs, window resize, etc.)
    // You can read the io.WantCaptureMouse, io.WantCaptureKeyboard flags to tell if dear imgui wants to use your inputs.
    // - When io.WantCaptureMouse is true, do not dispatch mouse input data to your main application, or clear/overwrite your copy of the mouse data.
    // - When io.WantCaptureKeyboard is true, do not dispatch keyboard input data to your main application, or clear/overwrite your copy of the keyboard data.
    // Generally you may always pass all inputs to dear imgui, and hide them from your application based on those two flags.
    ZoneScopedN("Poll Events");
    SDL_Event event;
    while (SDL_PollEvent(&event))
    {
        ImGui_ImplSDL3_ProcessEvent(&event);
        //DebugPrint("Event: %i", event.type);
        if (event.type == SDL_EVENT_WINDOW_CLOSE_REQUESTED && event.window.windowID == SDL_GetWindowID(gfx.window))
            g_running = true;

        switch (event.type)
        {
        case SDL_EVENT_QUIT:
            g_running = false;
            break;
        case SDL_EVENT_WINDOW_CLOSE_REQUESTED:
            g_running = !(event.window.windowID == SDL_GetWindowID(gfx.window));
            break;
        case SDL_EVENT_KEY_DOWN:
        case SDL_EVENT_KEY_UP:
            g_sysinfo.keys[event.key.key].down = (event.type == SDL_EVENT_KEY_DOWN);
            break;
        case SDL_EVENT_MOUSE_BUTTON_DOWN:
        case SDL_EVENT_MOUSE_BUTTON_UP:
            g_sysinfo.keys[event.button.button].down = event.button.down;
            break;
        case SDL_EVENT_MOUSE_MOTION:
        {
            ZoneScopedN("SDL_MOUSEMOTION");
            Vec2 delta;
            delta.x = ((event.motion.x) - g_sysinfo.mouse.p.x);
            delta.y = ((event.motion.y) - g_sysinfo.mouse.p.y);

            g_sysinfo.mouse.delta_p += delta;
            g_sysinfo.mouse.p.x = event.motion.x;
            g_sysinfo.mouse.p.y = event.motion.y;
            break;
        }
        case SDL_EVENT_MOUSE_WHEEL:
        {
            g_sysinfo.mouse.wheel_instant.x = g_sysinfo.mouse.wheel.x = event.wheel.x;
            g_sysinfo.mouse.wheel_instant.y = g_sysinfo.mouse.wheel.y = event.wheel.y;
            break;
        }
        case SDL_EVENT_WINDOW_RESIZED:
        {
            gfx.window_size.x = event.window.data1;
            gfx.window_size.y = event.window.data2;
            break;
        }
        case SDL_EVENT_WINDOW_FOCUS_GAINED:
        {
            g_sysinfo.has_attention = true;
            //g_sysinfo.mouse.delta_p = {};
            //SDL_GetMouseState(&g_sysinfo.mouse.p.x, &g_sysinfo.mouse.p.y);
            //g_sysinfo.mouse.p.y = g_settings.graphics.resolution.y - g_sysinfo.mouse.p.y;
            //SetFocus(g_renderer.SDL_Context);
            break;
        }
        case SDL_EVENT_WINDOW_FOCUS_LOST:
        {
            g_sysinfo.has_attention = false;
            break;
        }
        case SDL_EVENT_DROP_BEGIN:
            g_sysinfo.drop_active = true;
            break;
        case SDL_EVENT_DROP_COMPLETE:
            g_sysinfo.drop_active = false;
            break;
        case SDL_EVENT_DROP_FILE:
            if (event.drop.data)
            {
                g_sysinfo.drop_file.push_back(event.drop.data);
            }
            break;
        //case SDL_EVENT_DROP_TEXT:
        //case SDL_EVENT_DROP_BEGIN:
        //case SDL_EVENT_DROP_COMPLETE:
        //case SDL_EVENT_DROP_POSITION:
        //{
        //    if (event.drop.file)
        //    {
        //    }
        //    break;
        //}
        }
    }

    for (auto& key : g_sysinfo.keys)
    {
        if (key.second.down)
        {
            key.second.upThisFrame = false;
            if (key.second.downPrevFrame)
            {
                key.second.downThisFrame = false;
            }
            else
            {
                key.second.downThisFrame = true;
            }
        }
        else
        {
            key.second.downThisFrame = false;
            if (key.second.downPrevFrame)
            {
                key.second.upThisFrame = true;
            }
            else
            {
                key.second.upThisFrame = false;
            }
        }
        key.second.downPrevFrame = key.second.down;
    }

    if (g_sysinfo.mouse.wheel_modified_last_frame)
    {
        g_sysinfo.mouse.wheel_instant.y = 0;
        g_sysinfo.mouse.wheel_modified_last_frame = false;
    }
    else if (g_sysinfo.mouse.wheel_instant.y)
    {
        g_sysinfo.mouse.wheel_modified_last_frame = true;
    }
}

void RunProcessJob::RunJob()
{
    std::string zone_text;
    std::string zone_name;
    GetNameAndTextForJob(zone_text, zone_name, path, args);
    ZoneScoped;
    ZoneName(zone_name.c_str(), zone_name.size());
    ZoneText(zone_text.c_str(), zone_text.size());
    const wchar_t* wpath = path.size() ? path.c_str() : nullptr;
    const wchar_t* wargs = args.size() ? args.c_str() : nullptr;
    i32 result = SysRunProcess(wpath, wargs, output);
    //if (result)
    //{
    //    Threading::GetInstance().RunAndClearJobs();
    //}
}

void RunProcessLogToFileJob::RunJob()
{
    ZoneScopedN("RunProcessLogToFileJob");
    bool r = SysRunProcess(path.c_str(), args.c_str(), output, &output_file);
    if (run_and_clear && r)
    {
        ZoneScopedN("Run and Clear");
        Threading::GetInstance().RunAndClearJobs();
    }

    if (completed)
    {
        ASSERT(*completed == false);
        (*completed) = true;
    }
}
