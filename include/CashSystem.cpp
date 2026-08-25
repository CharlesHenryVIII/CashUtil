#include "CashSystem.h"
#include "CashRendering.h"
#include "CashArrayView.h"
#include "CashOS.h"

#include <array>

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

    //OSWriteToAttachedConsole(buffer, false);
}

void DebugPrint(const char* fmt, ...)
{
    va_list list;
    va_start(list, fmt);
    char buffer[4096] = {};
    SYS_VSNPRINTF(buffer, arrsize(buffer), fmt, list);
    OSDebugOutput(buffer);
    OSDebugOutput("\n");
    va_end(list);

    //OSWriteToAttachedConsole(buffer, true);
}
void DebugPrint(const wchar_t* fmt, ...)
{
    va_list list;
    va_start(list, fmt);
    wchar_t buffer[4096] = {};
    SYS_VSNWPRINTF(buffer, arrsize(buffer), fmt, list);
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

struct WaitForProcessJob : Job
{
    SDL_Process* process = nullptr;
    virtual void RunJob()
    {
        VALIDATE(process);
        SDL_WaitProcess(process, true, nullptr);
        SDL_DestroyProcess(process);
    };
    std::string zone_text;
    std::string zone_name;
};

i32 SysRunProcess(const char* args, AsyncData<std::string>* output, AsyncData<Path>* output_file, RunProcessFlags flags)
{
    const std::string a = args ? args : "";
    return SysRunProcess(a, output, output_file, flags);
}
i32 SysRunProcess(const wchar_t* argswt, AsyncData<std::string>* output, AsyncData<Path>* output_file, RunProcessFlags flags)
{
    const std::wstring argsw = argswt ? argswt : L"";
    return SysRunProcess(argsw, output, output_file, flags);
}
i32 SysRunProcess(const std::wstring& argsw, AsyncData<std::string>* output, AsyncData<Path>* output_file, RunProcessFlags flags)
{
    std::string args;
    SysConvertWideCharToMultiByte(args, argsw);
    return SysRunProcess(args, output, output_file, flags);
}
i32 SysRunProcess(const std::string& args, AsyncData<std::string>* output, AsyncData<Path>* output_file, RunProcessFlags flags)
{
    std::vector<std::string> arg_strings;
    bool in_quotes = false;
    std::string current_arg;
    for (const char c : args)
    {
        if (c == '\"')
        {
            in_quotes = !in_quotes;
        }
        else if (c == ' ' && !in_quotes)
        {
            if (!current_arg.empty())
            {
                arg_strings.push_back(current_arg);
                current_arg.clear();
            }
        }
        else
        {
            current_arg += c;
        }
    }
    if (!current_arg.empty())
        arg_strings.push_back(current_arg);

    // SDL requires a null-terminated array of char pointers
    std::vector<const char*> process_args;
    for (const auto& str : arg_strings)
        process_args.push_back(str.c_str());
    return SysRunProcess(CreateArrayView(process_args), output, output_file, flags);
}
i32 SysRunProcess(ArrayView<const char*> args, AsyncData<std::string>* output, AsyncData<Path>* output_file, RunProcessFlags flags)
{
    VALIDATE_V(args.size(), 0);
    std::string zone_name = args[0];
    std::string zone_text;
    for (i32 i = 0; i < args.size() && args[i]; i++)
    {
        zone_text += std::format("\"{}\"", args[i]);
        if (i < args.size() - 1 && args[i + 1])
            zone_text += " ";
    }
    ZoneScoped;
    ZoneName(zone_name.c_str(), zone_name.size());
    ZoneText(zone_text.c_str(), zone_text.size());

    //Checking for NULL final element
    std::vector<const char*> arg_array;
    for (const auto& arg : args)
        arg_array.push_back(arg);
    if (args.Last() != nullptr)
        arg_array.push_back(nullptr);
    ArrayView<const char*> arg_array_view = CreateArrayView(arg_array);

    SDL_PropertiesID props = SDL_CreateProperties();
    SDL_SetBooleanProperty(props, SDL_PROP_PROCESS_CREATE_BACKGROUND_BOOLEAN, true);
    SDL_SetPointerProperty(props, SDL_PROP_PROCESS_CREATE_ARGS_POINTER, arg_array_view.data);
    const bool pipe_output = (output || output_file);
    if (pipe_output)
    {
      SDL_SetNumberProperty(props, SDL_PROP_PROCESS_CREATE_STDOUT_NUMBER, SDL_PROCESS_STDIO_APP);
      SDL_SetBooleanProperty( props, SDL_PROP_PROCESS_CREATE_STDERR_TO_STDOUT_BOOLEAN, true);
    }
    SDL_Process* process = SDL_CreateProcessWithProperties(props);
    SDL_DestroyProperties(props);

    if (!process)
    {
        const std::string errorBoxTitle = ToString("SDL_CreateProcess Error: %s", SDL_GetError());
        const std::string errorText = ToString("Command Line Params: %s", zone_text.c_str());
        DebugPrint("%s\n", errorBoxTitle.c_str());
        DebugPrint(errorText.c_str());
        DebugPrint("\n");
        SysShowErrorWindow(errorBoxTitle, errorText);
        FAIL;
        return 2;
    }

    std::string local_file_buffer;
    if (pipe_output)
    {
        SDL_IOStream* stream = SDL_GetProcessOutput(process);
        if (stream)
        {
            if (output)
                output->state = AsyncStatus_Fetching;

            char buffer[4096];
            size_t bytesRead;

            while (true)
            {
                SDL_IOStatus stat = SDL_GetIOStatus(stream);
                if (((bytesRead = SDL_ReadIO(stream, buffer, sizeof(buffer))) == 0) && (stat != SDL_IO_STATUS_READY && stat != SDL_IO_STATUS_NOT_READY))
                    break;
                if (output)
                {
                    TRACY_LOCK(output->lock);
                    output->data.append(buffer, bytesRead);
                }
                if (output_file)
                    local_file_buffer.append(buffer, bytesRead);
            }
            if (output)
                output->state = AsyncStatus_FetchedSuccess;
        }
    }

    if (output_file)
    {
        if (output_file->data.empty())
        {
            DebugPrint("RunProcess() has output_file specified but no data: \"%s\"", zone_text.c_str());
        }
        else
        {
            ZoneScopedN("Output File");
            std::fstream file(output_file->data, std::ios_base::out);
            if (!file.good())
            {
                DebugPrint("Failed to open file for write: %s", ToString(output_file->data).c_str());
                FAIL;
            }
            else
            {
                TRACY_LOCK(output_file->lock);
                file << (output ? output->data : local_file_buffer);
            }
        }
    }

    int result_code = 0;
    if (!(flags & RunProcess_Async))
    {
        SDL_WaitProcess(process, true, &result_code);
        SDL_DestroyProcess(process);
    }
    else
    {
        WaitForProcessJob* job = new WaitForProcessJob();
        job->process = process;
    }
    return result_code;
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

void SysSleep(u64 _ms)
{
    ZoneScoped;
    std::this_thread::sleep_for(std::chrono::milliseconds(_ms));
}


double SysGetTime()
{
#if 1
    const static double freq = double(SDL_GetPerformanceFrequency()); //HZ
    const double time = SDL_GetPerformanceCounter() / freq;
#else
    static auto base_time = std::chrono::high_resolution_clock::now();
    auto now = std::chrono::high_resolution_clock::now();
    const double time = (now - base_time).count();
#endif
    return time;
}
float SysMonitorScale()
{
    ZoneScoped;
    const static float scale = SDL_GetDisplayContentScale(SDL_GetPrimaryDisplay());
    return scale;
}

Vec2 SysGetMousePosition()
{
    Vec2 result;
    SDL_GetMouseState(&result.x, &result.y);
    return result;
}
Vec2 SysGetWindowSize()
{
    Vec2I resulti;
    SDL_GetWindowSize(gfx.window, &resulti.x, &resulti.y);
    Vec2 result = ToVec2(resulti);
    return result;
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

void SysShowErrorWindow(const std::string& title, const std::string& text)
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

void SysScanDirectoryForFileNames(const Path& dir, ScannedFiles& out, ScanDirectoryFlags flags)
{
    OSScanDirectoryForFileNames(dir, out, flags);
}

bool SysGetDirectoryFromUser(const Path& currentDir, std::wstring& dir)
{
    return OSGetDirectoryFromUser(currentDir, dir);
}

//WARNING(CSH): Data here is a bit wacky and using new and delete be warned
struct OpenSystemNavUserData {
    Path* path;
    SDL_DialogFileFilter filter[2];
    std::function<void(void)> on_complete;
};
void SDLCALL OnFolderSelectedCallback(void* userdata, const char* const* filelist, int filter)
{
    VALIDATE(userdata);
    OpenSystemNavUserData* user_data = static_cast<OpenSystemNavUserData*>(userdata);
    if (filelist && filelist[0])
    {
        *user_data->path = filelist[0];
    }
    if (user_data->on_complete)
        user_data->on_complete();
    if (user_data->filter[0].pattern)
        delete user_data->filter[0].pattern;
    delete user_data;
}
void SysOpenSystemNavigation(Path* out_folder_path, const Path* starting_path, ArrayView<std::string> filters, std::function<void(void)> on_complete, SysFileNavigationFlags flags)
{
    if (!out_folder_path)
        return;

    std::string utf8_start_path = "";
    if (starting_path && !starting_path->empty())
        utf8_start_path = starting_path->string();

    OpenSystemNavUserData* user_data = new OpenSystemNavUserData();
    user_data->path = out_folder_path;
    user_data->on_complete = on_complete;

    SDL_PropertiesID props = SDL_CreateProperties();
    if (!utf8_start_path.empty())
        VERIFY(SDL_SetStringProperty(props, SDL_PROP_FILE_DIALOG_LOCATION_STRING, utf8_start_path.c_str()));
    if (filters.size())
    {
        std::string filter_list = "";
        for (i32 i = 0; i < filters.size(); i++)
        {
            filter_list = filter_list + filters[i] + ";";
        }
        filter_list.pop_back();
        char* filter_data = new char[filter_list.size() + 1];
        memset(filter_data, 0, filter_list.size() + 1);
        ArrayView<char> filter_list_array = CreateArrayView(filter_list);
        ArrayView<char> filter_data_array = CreateArrayView(filter_data, filter_list.size());
        CopyArrayView(filter_list_array, filter_data_array);

        user_data->filter[0] = {
            .name = "EXE Filter",
            .pattern = filter_data,
        };
        user_data->filter[1] = {
            .name = "All Files",
            .pattern = "*",
        };

        VERIFY(SDL_SetPointerProperty(props, SDL_PROP_FILE_DIALOG_FILTERS_POINTER, user_data->filter));
        VERIFY(SDL_SetNumberProperty(props, SDL_PROP_FILE_DIALOG_NFILTERS_NUMBER, arrsize(user_data->filter)));
    }

    //SDL_FILEDIALOG_SAVEFILE,
    SDL_FileDialogType type = SDL_FILEDIALOG_OPENFILE;
    if (FlagIntersects(flags, SysFileNavigationFlags_OnlyFolders))
        type = SDL_FILEDIALOG_OPENFOLDER;
    else if (FlagIntersects(flags, SysFileNavigationFlags_OnlyFiles))
        type = SDL_FILEDIALOG_OPENFILE;
    VERIFY(SDL_SetBooleanProperty(props, SDL_PROP_FILE_DIALOG_MANY_BOOLEAN, !(FlagIntersects(flags, SysFileNavigationFlags_MultiSelect))));

    SDL_ShowFileDialogWithProperties(type, OnFolderSelectedCallback, user_data, props);
    SDL_DestroyProperties(props);
}
bool SysGetExecutablePath(Path& out_path, const std::string& name)
{
    const char* path_env_var = std::getenv("PATH");
    if (!path_env_var)
    {
        return false;
    }

    // 2. Split the PATH by the Linux delimiter ':'
    std::stringstream ss(path_env_var);
    std::string dir;

    while (std::getline(ss, dir, ':'))
    {
        // 3. Combine the directory with the executable name
        out_path = Path(dir) / name;

        // 4. Check if the file actually exists and isn't a folder
        if (fs::exists(out_path) && fs::is_regular_file(out_path))
        {
            return true;
        }
    }

    // Not found in any PATH directory
    return false;
}

void SysConvertMultibyteToWideChar(std::wstring& out, const std::string& in, StringEncoding encoding)
{
    OSConvertMultibyteToWideChar(out, in, encoding);
}
void SysConvertWideCharToMultiByte(std::string& out, const std::wstring& in, StringEncoding encoding)
{
    OSConvertWideCharToMultiByte(out, in, encoding);
}

void SysExpandEnvironemntVariable(std::wstring& out, const std::wstring& in)
{
    OSExpandEnvironemntVariable(out, in);
}

void* SysGetDataFromResource(i32* out_size, const i32 resource_id)
{
    return OSGetDataFromResource(out_size, resource_id);
}

ImFont* SysCreateImguiFont(const ArrayView<const u8> font_data, float font_size)
{
    ImFontConfig cfg;
    cfg.FontDataOwnedByAtlas = false;
    ImFont* font = ImGui::GetIO().Fonts->AddFontFromMemoryTTF(
        const_cast<void*>((const void*)font_data.data),
        (i32)font_data.Bytes(),
        font_size,
        &cfg
    );
    if (!font)
        return nullptr;
    return font;
}

ImFont* SysLoadFontForImgui(int resource_id, float font_size)
{
    i32 size;
    void* data = OSGetDataFromResource(&size, resource_id);
    if (!data || size == 0)
        return nullptr;

    return SysCreateImguiFont(CreateArrayView((const u8*)data, size), font_size);
}

std::string Guid::ToString() const
{
    return ::ToString("%08X-%04X-%04X-%04X-%04X%08X", a, b >> 16, b & 0XFFFF, c >> 16, c & 0XFFFF, d);
}

Guid GuidFromString(const char* s)
{
    Guid r = {};
    const size_t char_len = SYS_STRNLEN(s, 38);
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

bool SysRenderInit(const SysRenderInitDesc* desc)
{
    return OSRenderInit(desc);
}
bool SysRenderDestroy()
{
    return OSRenderDestroy();
}
void SysRenderPresent()
{
    OSRenderPresent();
}
void SysGetRenderEnvironment(sg_environment* env)
{
    OSGetRenderEnvironment(env);
}

void RunProcessJob::RunJob()
{
    VALIDATE(!m_args_string.empty() || m_args_array.size());

    const bool use_array = m_args_array.size();

    std::string zone_text;
    std::string zone_name;
    if (use_array)
    {
        zone_name = m_args_array[0];
        for (i32 i = 0; i < m_args_array.size(); i++)
        {
            if (!m_args_array[i])
                continue;
            zone_text += std::format("\"{}\"", m_args_array[i]);
            if (i < m_args_array.size() - 1 && m_args_array[i + 1])
                zone_text += " ";
        }
        ZoneScoped;
    }
    else
    {
        const size_t p = m_args_string.find_first_of(' ', 1);
        zone_name = m_args_string.substr(0, p);
        zone_text = m_args_string;
    }
    ZoneScoped;
    ZoneName(zone_name.c_str(), zone_name.size());
    ZoneText(zone_text.c_str(), zone_text.size());

    i32 result = 0;
    if (use_array)
        result = SysRunProcess(m_args_array, m_output, m_output_file);
    else
        result = SysRunProcess(m_args_string, m_output, m_output_file);

    if (result && m_run_all_jobs)
    {
        Threading::GetInstance().RunAndClearJobs();
    }

    if (m_completed)
    {
        ASSERT(*m_completed == false);
        (*m_completed) = true;
    }
}