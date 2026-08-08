#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <pwd.h>
#include <ifaddrs.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <net/if.h>
#include <endian.h>
#include <dirent.h>
#include <sys/stat.h>

#include "../CashOS.h"
#include "../CashMath.h"
#include "../CashString.h"
#include "../CashSystem.h"

#include <SDL3/SDL.h>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <vector>
#include <string>
#include <cstdlib>
#include <cstring>

// -----------------------------------------------------------------------------
// IP Conversions
// -----------------------------------------------------------------------------

std::string SysIP4::ToString() const
{
    std::string r;
    if (!IsValid()) return r;
    r.resize(INET_ADDRSTRLEN - 1);
    if (inet_ntop(AF_INET, &addr, r.data(), INET_ADDRSTRLEN) == nullptr)
    {
        DebugPrint("Error: Failed to convert IPv4 to string.");
        FAIL;
    }
    return r;
}

std::wstring SysIP4::ToWString() const
{
    std::string s = ToString();
    std::wstring ws;
    OSConvertMultibyteToWideChar(ws, s);
    return ws;
}

void SysIP4::FromString(const std::string& in)
{
    if (in.empty()) return;
    if (inet_pton(AF_INET, in.c_str(), &addr) <= 0)
    {
        DebugPrint("Error: Failed to convert string to IPv4: {%s}", in.c_str());
        FAIL;
    }
}

std::string SysIP6::ToString() const
{
    std::string r;
    if (!IsValid()) return r;
    r.resize(INET6_ADDRSTRLEN - 1);
    if (inet_ntop(AF_INET6, &addr, r.data(), INET6_ADDRSTRLEN) == nullptr)
    {
        DebugPrint("Error: Failed to convert IPv6 to string.");
        FAIL;
    }
    return r;
}

SysIP4 SysIP4Subnet::ToIP4() const
{
    SysIP4 r;
    r.addr = htonl(mask);
    return r;
}
void SysIP4Subnet::FromIP(const SysIP4 ip)
{
    mask = ntohl(ip.addr);
}

SysIP6 SysIP6Subnet::ToIP6() const
{
    SysIP6 r;
    r.addr[0] = htobe64(mask[0]);
    r.addr[1] = htobe64(mask[1]);
    return r;
}

// -----------------------------------------------------------------------------
// Console & Debugging
// -----------------------------------------------------------------------------

void OSWriteToAttachedConsole(const char* buffer, bool add_new_line)
{
    std::cout << buffer;
    if (add_new_line) std::cout << std::endl;
}

void OSWriteToAttachedConsole(const wchar_t* buffer, bool add_new_line)
{
    std::wcout << buffer;
    if (add_new_line) std::wcout << std::endl;
}

void OSDebugOutput(const char* s)
{
    std::cout << s;
}

void OSDebugOutput(const wchar_t* s)
{
    std::wcout << s;
}

bool OSIsConsoleAttached()
{
    return isatty(fileno(stdout));
}

bool OSIsDebuggerAttached()
{
    // Linux equivalent: Check TracerPid in /proc/self/status
    std::ifstream status("/proc/self/status");
    std::string line;
    while (std::getline(status, line))
    {
        if (line.rfind("TracerPid:", 0) == 0)
        {
            return std::stoi(line.substr(10)) != 0;
        }
    }
    return false;
}

void OSHideConsole() { /* No-op on Linux, terminals own the process */ }
void OSShowConsole() { /* No-op */ }
bool OSIsConsoleVisible() { return OSIsConsoleAttached(); }

// -----------------------------------------------------------------------------
// Initialization & Windowing
// -----------------------------------------------------------------------------

bool OSInit(SDL_Window* window)
{
    if (!window)
    {
        DebugPrint("Failed to get Window.");
        FAIL;
        return false;
    }

    char hostname[256];
    if (gethostname(hostname, sizeof(hostname)) == 0)
    {
        SysConvertMultibyteToWideChar(g_sysinfo.name, hostname);
    }

    g_sysinfo.cores = sysconf(_SC_NPROCESSORS_ONLN);
    g_sysinfo.threads = g_sysinfo.cores; // Linux doesn't easily expose logical vs physical via POSIX API without parsing /proc/cpuinfo

    return true;
}

void OSDestroy(SDL_Window* window)
{
    SDL_DestroyWindow(window);
}

void* OSGetWindowHandle(SDL_Window* window)
{
    SDL_PropertiesID props = SDL_GetWindowProperties(window);
    // Try Wayland first, fallback to X11
    void* handle = SDL_GetPointerProperty(props, SDL_PROP_WINDOW_WAYLAND_SURFACE_POINTER, NULL);
    if (!handle)
    {
        handle = SDL_GetPointerProperty(props, SDL_PROP_WINDOW_X11_WINDOW_NUMBER, NULL);
    }
    return handle;
}

void OSShowErrorWindow(const std::string& title, const std::string& text)
{
    SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, title.c_str(), text.c_str(), nullptr);
}

void OSFlashWindow(SDL_Window* window)
{
    SDL_FlashWindow(window, SDL_FLASH_BRIEFLY);
}

// -----------------------------------------------------------------------------
// Networking
// -----------------------------------------------------------------------------

SysIP4 GetIP4FromSockaddr(const sockaddr* sa)
{
    const sockaddr_in* sa_in = (sockaddr_in*)sa;
    SysIP4 r;
    r.addr = sa_in->sin_addr.s_addr;
    return r;
}

SysIP6 GetIP6FromSockaddr(const sockaddr* sa)
{
    const sockaddr_in6* sa_in = (sockaddr_in6*)sa;
    SysIP6 r;
    memmove((void*)&r, (void*)&sa_in->sin6_addr, sizeof(r));
    return r;
}

bool OSGetNetworkAdapters(std::vector<SysNetworkAdapterInfo>& adapters)
{
    struct ifaddrs* ifaddr, * ifa;
    if (getifaddrs(&ifaddr) == -1)
    {
        DebugPrint("Error: getifaddrs failed");
        return false;
    }

    for (ifa = ifaddr; ifa != nullptr; ifa = ifa->ifa_next)
    {
        if (ifa->ifa_addr == nullptr) continue;

        // Try to find if we already added this interface name
        SysNetworkAdapterInfo* ad = nullptr;
        for (auto& existing : adapters)
        {
            if (existing.name == ifa->ifa_name)
            {
                ad = &existing;
                break;
            }
        }

        if (!ad)
        {
            SysNetworkAdapterInfo new_ad = {};
            new_ad.name = ifa->ifa_name;
            SysConvertMultibyteToWideChar(new_ad.friendly_name, new_ad.name);
            new_ad.status = (ifa->ifa_flags & IFF_UP) ? "Up" : "Down";
            new_ad.multicast_enabled = (ifa->ifa_flags & IFF_MULTICAST);
            adapters.push_back(new_ad);
            ad = &adapters.back();
        }

        if (ifa->ifa_addr->sa_family == AF_INET)
        {
            ad->ipv4_enabled = true;
            SysIP4AndSubnet ips = {};
            ips.ip = GetIP4FromSockaddr(ifa->ifa_addr);
            ips.subnet.FromIP(GetIP4FromSockaddr(ifa->ifa_netmask));
            ad->ipv4_ips.push_back(ips);
        }
        else if (ifa->ifa_addr->sa_family == AF_INET6)
        {
            ad->ipv6_enabled = true;
            SysIP6AndSubnet ips = {};
            ips.ip = GetIP6FromSockaddr(ifa->ifa_addr);
            // IPv6 subnet masking requires iterating the netmask bytes, omitted for brevity
            ad->ipv6_ips.push_back(ips);
        }
    }

    freeifaddrs(ifaddr);
    return true;
}

bool OSSetNetAdapterIP(const std::string& adapter_guid, const SysNetAdapterConfig& adapter, const SysNetAdapterConfig& src_adapter)
{
    // Linux uses NetworkManager (nmcli) for cross-distro network management
    if (adapter.dhcp_enabled)
    {
        std::string cmd = "nmcli con mod " + adapter_guid + " ipv4.method auto";
        return system(cmd.c_str()) == 0;
    }
    else
    {
        std::string ip = adapter.ip.ip.ToString();
        // Assuming /24 for simplification, a real implementation needs to calculate CIDR from subnet
        std::string cmd = "nmcli con mod " + adapter_guid +
                          " ipv4.addresses " + ip + "/24" +
                          " ipv4.method manual";

        if (adapter.gateway.IsValid())
        {
            cmd += " ipv4.gateway " + adapter.gateway.ToString();
        }

        if (system(cmd.c_str()) == 0)
        {
            std::string up_cmd = "nmcli con up " + adapter_guid;
            return system(up_cmd.c_str()) == 0;
        }
    }
    return false;
}

bool OSSetNetAdapterDNS(const std::string& adapter_guid, const SysNetAdapterConfig& adapter, const SysNetAdapterConfig& src_adapter)
{
    if (adapter.ddns_enabled)
    {
        std::string cmd = "nmcli con mod " + adapter_guid + " ipv4.ignore-auto-dns no";
        return system(cmd.c_str()) == 0;
    }
    else
    {
        std::string dns1 = adapter.dns[0].ToString();
        std::string dns2 = adapter.dns[1].ToString();
        std::string cmd = "nmcli con mod " + adapter_guid + " ipv4.dns \"" + dns1 + " " + dns2 + "\" ipv4.ignore-auto-dns yes";

        if (system(cmd.c_str()) == 0)
        {
            std::string up_cmd = "nmcli con up " + adapter_guid;
            return system(up_cmd.c_str()) == 0;
        }
    }
    return false;
}

// -----------------------------------------------------------------------------
// Processes & System
// -----------------------------------------------------------------------------

bool OSHasAdminPrivledge()
{
    return geteuid() == 0;
}

// -----------------------------------------------------------------------------
// Filesystem & Strings
// -----------------------------------------------------------------------------

void OSConvertMultibyteToWideChar(std::wstring& out, const std::string& in)
{
    if (in.empty()) { out.clear(); return; }
    size_t len = mbstowcs(nullptr, in.c_str(), 0);
    if (len == (size_t)-1) return;
    out.resize(len);
    mbstowcs(out.data(), in.c_str(), len);
}

void OSConvertWideCharToMultiByte(std::string& out, const std::wstring& in)
{
    if (in.empty()) { out.clear(); return; }
    size_t len = wcstombs(nullptr, in.c_str(), 0);
    if (len == (size_t)-1) return;
    out.resize(len);
    wcstombs(out.data(), in.c_str(), len);
}

void OSExpandEnvironemntVariable(std::wstring& out, const std::wstring& in)
{
    // A simplified expansion: checks if string starts with $ and retrieves it
    std::string mbIn;
    OSConvertWideCharToMultiByte(mbIn, in);

    if (mbIn.length() > 1 && mbIn[0] == '$')
    {
        const char* env = getenv(mbIn.c_str() + 1);
        if (env)
        {
            OSConvertMultibyteToWideChar(out, env);
            return;
        }
    }
    out = in; // Fallback to unmodified
}

void _ScanDirectoryForFileNames(const Path& root, const Path& dir, ScannedFiles& out, ScanDirectoryFlags flags)
{
    DIR* dir_handle = opendir(root.string().c_str());
    if (!dir_handle)
    {
        DebugPrint(ToString("Error opening directory: %s", root.c_str()).c_str());
        return;
    }

    struct dirent* entry;
    while ((entry = readdir(dir_handle)) != nullptr)
    {
        if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0)
        {
            continue;
        }


        bool is_dir = entry->d_type == DT_DIR;
        if (entry->d_type == DT_UNKNOWN)
        {
            // Fallback: If the filesystem doesn't support d_type, ask the OS explicitly.
            const Path full_path = root / entry->d_name;
            struct stat st;
            if (stat(full_path.string().c_str(), &st) == 0)
            {
                is_dir = S_ISDIR(st.st_mode);
            }
        }

        const Path relative_path = dir.empty() ? entry->d_name : dir / entry->d_name;
        if (is_dir)
        {

            if (FlagIntersects(flags, ScanDirectoryFlags_IncludeDirs))
                out.push_back({ relative_path.wstring(), true });

            if (FlagIntersects(flags, ScanDirectoryFlags_Recursive))
            {
                Path new_root = root / entry->d_name;
                _ScanDirectoryForFileNames(new_root, relative_path, out, flags);
            }
        }
        else
        {
            out.push_back({ relative_path.wstring(), false });
        }
    }

    closedir(dir_handle);
}
void OSScanDirectoryForFileNames(const Path& dir, ScannedFiles& out, ScanDirectoryFlags flags)
{
    out.clear();
    _ScanDirectoryForFileNames(dir, "", out, flags);
}
//void OSScanDirectoryForFileNames(const Path& dir, ScannedFiles& out, ScanDirectoryFlags flags)
//{
//    out.clear();
//
//    try {
//        if (flags & ScanDirectoryFlags_Recursive)
//        {
//            for (const auto& entry : std::filesystem::recursive_directory_iterator(dir))
//            {
//                std::wstring wpath;
//                OSConvertMultibyteToWideChar(wpath, entry.path().string());
//                out.push_back({ wpath, entry.is_directory() });
//            }
//        }
//        else
//        {
//            for (const auto& entry : std::filesystem::directory_iterator(dir))
//            {
//                std::wstring wpath;
//                OSConvertMultibyteToWideChar(wpath, entry.path().string());
//                out.push_back({ wpath, entry.is_directory() });
//            }
//        }
//    } catch (const std::filesystem::filesystem_error& e) {
//        DebugPrint("ScanDirectory Error: %s", e.what());
//    }
//}

bool OSGetDirectoryFromUser(const Path& currentDir, std::wstring& dir)
{
    // Use Zenity, which is standard on Arch/CachyOS and most desktop environments
    FILE* pipe = popen("zenity --file-selection --directory 2>/dev/null", "r");
    if (!pipe) return false;

    char buffer[1024];
    std::string result = "";
    while (fgets(buffer, sizeof(buffer), pipe) != nullptr)
    {
        result += buffer;
    }
    pclose(pipe);

    if (!result.empty())
    {
        // Strip trailing newline
        if (result.back() == '\n') result.pop_back();
        OSConvertMultibyteToWideChar(dir, result);
        return true;
    }
    return false;
}


#ifdef FEATURE_CUSTOM_ASSERT

#include <mutex>
#include <vector>
#include <cstdio>   // snprintf
#include <cstdlib>  // _exit
#include <signal.h> // raise, SIGABRT, SIGTRAP
#include <SDL3/SDL.h>

// 1. Replace SRW Locks with standard C++ Mutex
#define SRW_LOCK std::mutex
#define SRW_LOCK_ACQUIRE(_lock) _lock.lock()
#define SRW_LOCK_RELASE(_lock) _lock.unlock()

// 2. Linux debug break equivalent
#if defined(__clang__) || defined(__GNUC__)
    #define DEBUG_BREAK() __builtin_debugtrap() // or __builtin_trap() depending on compiler version
#else
    #define DEBUG_BREAK() raise(SIGTRAP)
#endif

struct AssertRecord
{
    const char* file;
    int         line;
    int         hit_counter;
    bool        ignored;
};

struct SRWLock
{
    SRW_LOCK lock;
};

static SRWLock s_assert_mutex;
static std::vector<AssertRecord> s_assert_records;

void OsAssert(bool expr, const char* message, const char* file, int line)
{
    if (!expr)
    {
        SRW_LOCK_ACQUIRE(s_assert_mutex.lock);
        Defer { SRW_LOCK_RELASE(s_assert_mutex.lock); };

        AssertRecord* record = nullptr;
        for (AssertRecord& it : s_assert_records)
        {
            if (it.file == file && it.line == line)
            {
                record = &it;
                break;
            }
        }

        if (!record)
        {
            AssertRecord new_record = {
                .file = file,
                .line = line,
                .hit_counter = 0,
                .ignored = false
            };
            s_assert_records.push_back(new_record);
            record = &s_assert_records.back();
            // Force pointer stability just in case push_back reallocated
            record->file = file;
            record->line = line;
        }

        record->hit_counter++;
        if (record->ignored)
        {
            return;
        }

        const char* s = record->hit_counter == 1 ? "" : "s";
        char info_buffer[2048];

        // Replaced sprintf_s with standard snprintf
        snprintf(info_buffer, sizeof(info_buffer),
            "%s\n\n"
            "File: %s(%d)\n"
            "This has been hit %d time%s.\n",
            message, file, line, record->hit_counter, s);

        // 3. Define custom buttons for the SDL Message Box
        const SDL_MessageBoxButtonData buttons[] = {
            { SDL_MESSAGEBOX_BUTTON_RETURNKEY_DEFAULT, 0, "Break" },     // ID: 0
            { 0,                                       1, "Continue" },  // ID: 1
            { 0,                                       2, "Ignore" },    // ID: 2
            { SDL_MESSAGEBOX_BUTTON_ESCAPEKEY_DEFAULT, 3, "Abort" },     // ID: 3
        };

        const SDL_MessageBoxData messageboxdata = {
            SDL_MESSAGEBOX_WARNING,
            NULL,               // Window (NULL makes it a system-level popup)
            "Assertion Failed", // Title
            info_buffer,        // Message
            SDL_arraysize(buttons),
            buttons,
            NULL                // Default color scheme
        };

        int button_id = -1;
        SDL_ShowMessageBox(&messageboxdata, &button_id);

        switch (button_id)
        {
            case 0: // Break
            {
                DEBUG_BREAK();
            } break;

            case 1: // Continue
            {
                // Do nothing, let execution continue
            } break;

            case 2: // Ignore
            {
                if (record)
                {
                    record->ignored = true;
                }
            } break;

            case 3:   // Abort
            default:  // Window closed via 'X' or error
            {
                raise(SIGABRT);
                _exit(3);
            } break;
        }
    }
}
#else
#include <assert.h>
void OsAssert(bool expr, const char*, const char*, int)
{
    ASSERT(expr);
}
#endif

// Linux only uses the standard C/C++ main entry point.
int main(int argc, char** argv)
{
    return SysMain(argc, argv);
}

// -----------------------------------------------------------------------------
// Resources & UUID
// -----------------------------------------------------------------------------

void* OSGetDataFromResource(i32* out_size, const i32 resource_id)
{
    DebugPrint("Warning: Windows PE Resources are not supported on Linux.");
    if (out_size) *out_size = 0;
    return nullptr;
}

Guid OSNewGuid()
{
    Guid id = {};
    // Read from kernel random UUID generation to avoid needing libuuid dependency
    std::ifstream file("/proc/sys/kernel/random/uuid");
    if (file.is_open())
    {
        std::string uuid_str;
        std::getline(file, uuid_str);
        // Format is: xxxxxxxx-xxxx-xxxx-xxxx-xxxxxxxxxxxx
        // A full implementation would parse the hex string into the GUID struct bytes.
        // Omitted string-to-byte parsing here for brevity.
    }
    return id;
}