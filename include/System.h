#pragma once
#include "Threading.h"
#include "Math.h"
#include "ArrayView.h"
#include "String.h"

#include "SDL3/SDL.h"
//#include "SDL3/SDL_events.h"
#include "Imgui.h"

#include <string>
#include <unordered_map>

#define PWSH_MAX_COLUMNS 16
typedef std::vector<std::array<std::string, PWSH_MAX_COLUMNS>> PowershellResponse;


enum RunProcessFlags : u32 {
    RunProcess_None = 0,
    RunProcess_Async = BIT(0),
    RunProcess_Show = BIT(1),
};

struct Key {
    bool down;
    bool downPrevFrame;
    bool downThisFrame;
    bool upThisFrame;
};

struct Mouse {
    Vec2 p = {}; //origin is the bottom left of the window
    Vec2 delta_p = {};
    Vec2 wheel = {}; //Y for vertical rotations, X for Horizontal rotations/movement
    Vec2 wheel_instant = {};
    bool wheel_modified_last_frame = false;
    //SDL_Cursor* cursors[ImGuiMouseCursor_COUNT] = {};
};

struct SysInfo {
    std::wstring name;
    i32 cores;
    i32 threads;
    std::unordered_map<u32, Key> keys;
    Mouse mouse = {};
    bool has_attention;
    bool drop_active = false;
    std::vector<Path> drop_file;
};
extern SysInfo g_sysinfo;

union SysIP4 {
    struct { u8 a,b,c,d; };
    struct { u16 ab, cd; };
    u32 addr;
    u8 e[4];
    SysIP4() = default;
    SysIP4(u8 v) : a(v), b(v), c(v), d(v) {};
    SysIP4(u8 _a, u8 _b, u8 _c, u8 _d) : a(_a), b(_b), c(_c), d(_d) {};
    std::string ToString() const;
    std::wstring ToWString() const;
    void FromString(const std::string& in);
    bool IsValid() const
    {
        return (addr != 0);
    }
};

union SysIP6 {
    struct { u16 a, b, c, d, e, f, g, h; };
    u8  byte[16];
    u16 word[8];
    u64 addr[2];
    SysIP6() = default;
    SysIP6(u16 v) : a(v), b(v), c(v), d(v), e(v), f(v), g(v), h(v) {};
    SysIP6(u8 _a, u8 _b, u8 _c, u8 _d, u8 _e, u8 _f, u8 _g, u8 _h) : a(_a), b(_b), c(_c), d(_d), e(_e), f(_f), g(_g), h(_h) {};
    std::string ToString() const;
    bool IsValid() const
    {
        return (addr[0] != 0 || addr[1] != 0);
    }
};

struct SysIP4Subnet
{
    u32 mask;
    SysIP4 ToIP4() const;
    //Create the mask like 255.255.255.0 from the prefix length
    void FromLength(i32 length)
    {
        mask = (length == 0) ? 0 : (~0UL << (32 - length));
    }
    std::string ToString() const
    {
        std::string r;
        //"-1" to remove null terminator
        const SysIP4 ip = ToIP4();
        const std::string ips = ip.ToString();
        r = ::ToString("%s (/%i)", ips.c_str(), GetLength());
        return r;
    }
    void FromIP(const SysIP4 ip);
    void FromString(const std::string& in)
    {
        const size_t begin = in.find_last_of('/') + 1;
        const size_t end = in.find_last_of(')');
        VALIDATE(begin != std::string::npos);
        VALIDATE(end != std::string::npos);
        const std::string num = in.substr(begin, end - begin);
        i32 val;
        std::from_chars_result r = std::from_chars(&num.front(), num.c_str() + num.size(), val, 10);
        VALIDATE(r.ec == std::errc());
        FromLength(val);
    }
    i32 GetLength() const
    {
        return std::countl_one(mask);
    }
    bool IsValid() const
    {
        return mask != 0;
    }
};

struct SysIP6Subnet
{
    u64 mask[2];
    SysIP6 ToIP6() const;
    //Create the mask like 255.255.255.0 from the prefix length
    void FromLength(i32 length)
    {
        if (length >= 64)
        {
            mask[0] = -1;
            mask[1] = (length == 64) ? 0 : (((u64)(-1)) << (128 - length));
        }
        else
        {
            mask[0] = (length == 0) ? 0 : (~0ULL << (64 - length));
            mask[1] = 0;
        }
        i32 test = 1;
    }
    std::string ToString() const
    {
        std::string r;
        //"-1" to remove null terminator
        SysIP6 ip = ToIP6();
        const std::string ips = ip.ToString();
        r = ::ToString("%s (/%i)", ips.c_str(), GetLength());
        return r;
    }
    i32 GetLength() const
    {
        i32 zerobits = 0;
        u64 m = mask[0];
        for (i32 i = 0; i < sizeof(u64) * 8 && ((m & 0x1) == 0); i++)
        {
            m = m >> 1;
            zerobits++;
        }
        m = mask[1];
        for (i32 i = 0; i < sizeof(u64) * 8 && ((m & 0x1) == 0); i++)
        {
            m = m >> 1;
            zerobits++;
        }
        return ((sizeof(mask) * 8) - zerobits);
    }
};


struct SysIP4AndSubnet {
    SysIP4 ip;
    SysIP4Subnet subnet;
};

struct SysIP6AndSubnet {
    SysIP6 ip;
    SysIP6Subnet subnet;
};

struct SysMacAddress {
    i32 count = 0;
    u8 bytes[8] = {};
    void ToString(std::string& out) const
    {
        out.clear();
        const i32 tot = Min<i32>(arrsize(bytes), count);
        for (i32 i = 0; i < tot; i++)
        {
            char s[4] = {};
            sprintf(s, "%.2X%s", bytes[i], (i == (tot - 1)) ? "" : "-");
            out = out + s;
        }
    }
    void SetBytes(const u8* in, const i32 amount)
    {
        memmove(bytes, in, Min<size_t>(amount, arrsize(bytes)));
        count = amount;
    }
};

#define SYS_NET_CONFIG_MAX_DNS 2
struct SysNetAdapterConfig {
    std::string name;
    SysIP4AndSubnet ip;
    SysIP4 gateway;
    SysIP4 dns[SYS_NET_CONFIG_MAX_DNS];
    bool dhcp_enabled; //DHCP Enabled
    bool ddns_enabled; //Dynamic DNS Enabled
};

struct SysNetworkAdapterInfo
{
    std::string name;
    std::string status;
    std::wstring friendly_name;
    std::wstring description;
    std::wstring dns_domain;
    SysMacAddress mac_address;
    SysIP4        ipv4_dhcp;
    SysIP6        ipv6_dhcp;
    std::vector<SysIP4AndSubnet> ipv4_ips;
    std::vector<SysIP6AndSubnet> ipv6_ips;
    std::vector<SysIP4>          ipv4_dns;
    std::vector<SysIP6>          ipv6_dns;
    std::vector<SysIP4>          ipv4_gateways;
    std::vector<SysIP6>          ipv6_gateways;

    i32 index;
    u32 ipv4_metric;
    u32 ipv6_metric;
    bool ipv4_enabled;
    bool ipv6_enabled;
    bool dhcpv4_enabled;
    bool ddns_enabled;
    bool domain_dns_register_enabled;
    bool receive_only;
    bool multicast_enabled;
};


bool SysInit(SDL_Window* window);
void SysDestroy(SDL_Window* window);
void* SysGetWindowHandle(SDL_Window* window);
i32 SysMain(i32 argc, char** argv);

void DebugPrint(const char* fmt, ...);
void DebugPrint(const wchar_t* fmt, ...);
//No new line
void SysDebugPrintDirect(const char* fmt, ...);
bool SysIsConsoleAttached();
bool SysIsDebuggerAttached();
void SysHideConsole();
void SysShowConsole();
bool SysIsConsoleVisible();

i32 SysRunShellProcess(const wchar_t* path, const wchar_t* args, std::string* output = nullptr, Mutex* output_lock = nullptr, RunProcessFlags flags = RunProcess_None);
i32 SysRunProcess(const char*         path, const char*         args, AsyncData<std::string>* output = nullptr, AsyncData<Path>* output_file = nullptr, RunProcessFlags flags = RunProcess_None);
i32 SysRunProcess(const wchar_t*      path, const wchar_t*      args, AsyncData<std::string>* output = nullptr, AsyncData<Path>* output_file = nullptr, RunProcessFlags flags = RunProcess_None);
i32 SysRunProcess(const std::string&  path, const std::string&  args, AsyncData<std::string>* output = nullptr, AsyncData<Path>* output_file = nullptr, RunProcessFlags flags = RunProcess_None);
i32 SysRunProcess(const std::wstring& path, const std::wstring& args, AsyncData<std::string>* output = nullptr, AsyncData<Path>* output_file = nullptr, RunProcessFlags flags = RunProcess_None);
bool SysGetNetworkAdapters(std::vector<SysNetworkAdapterInfo>& out_adapters);
bool SysHasAdminPrivledge();
bool SysSetNetAdapterIP(const std::string& adapter_guid, const SysNetAdapterConfig& adapter, const SysNetAdapterConfig& src_adapter);
bool SysSetNetAdapterDNS(const std::string& adapter_guid, const SysNetAdapterConfig& adapter, const SysNetAdapterConfig& src_adapter);

void SysAssert(bool expr, const char* message, const char* file, int line);
void SysSleep(u64 ms);
double SysGetTime();
float SysMonitorScale();

void ParseCSV(PowershellResponse& out, const std::string& in, bool using_quotes);

void SysProcessEvents();


void SysShowErrorWindow(const std::wstring& title, const std::wstring& text);
i32 SysShowCustomErrorWindow(const std::string& title, const std::string& text);
void SysFlashWindow(SDL_Window* window);
enum ScanDirectoryFlags : u32 {
    ScanDirectoryFlags_None = 0,
    ScanDirectoryFlags_Recursive = BIT(0),
    ScanDirectoryFlags_IncludeDirs = BIT(1),
    ScanDirectoryFlags_All = ScanDirectoryFlags_Recursive | ScanDirectoryFlags_IncludeDirs,
};
struct ScannedFile {
    std::wstring name;
    bool dir;
};
using ScannedFiles = std::vector<ScannedFile>;
void SysScanDirectoryForFileNames(const std::wstring& dir, ScannedFiles& out, ScanDirectoryFlags flags);
bool SysGetDirectoryFromUser(const std::wstring& currentDir, std::wstring& dir);
enum MessageBoxResponse : i32 {
    MessageBoxResponse_Invalid,
    MessageBoxResponse_OpenLog,
    MessageBoxResponse_Continue,
    MessageBoxResponse_Quit,
    MessageBoxResponse_Count,
};

void SysConvertMultibyteToWideChar(std::wstring& out, const std::string& in);
void SysConvertWideCharToMultiByte(std::string& out, const std::wstring& in);
void SysExpandEnvironemntVariable(std::wstring& out, const std::wstring& in);
ImFont* SysLoadFontForImgui(int resource_id, float fontSize);
void* SysGetDataFromResource(i32* out_size, const i32 resource_id);

union Guid {
    uint64_t e[2];
    //struct {
    //    uint32_t data1;
    //    uint16_t data2;
    //    uint16_t data3;
    //    uint8_t  data4[8];
    //};
    struct {
        uint32_t a;
        uint32_t b;
        uint32_t c;
        uint32_t d;
    };
    std::string ToString() const;
};
[[nodiscard]] Guid GuidFromString(const char* s);
template<typename T> inline [[nodiscard]] T GuidFromString(const char* s) { Guid id = GuidFromString(s); return *(T*)&id; };
inline constexpr [[nodiscard]] bool operator==(const Guid& a, const Guid& b) { return (a.e[0] == b.e[0]) && (a.e[1] == b.e[1]); }
inline constexpr [[nodiscard]] bool operator<(const Guid& a, const Guid& b) { return (a.e[1] < b.e[1]) || ((a.e[1] == b.e[1]) && (a.e[0] < b.e[0])); }
Guid SysNewGuid();

#define STRONG_GUID_DEF(name)                                                                                     \
union name {                                                                                                      \
    uint64_t e[2];                                                                                                \
    struct {                                                                                                      \
        uint32_t a;                                                                                               \
        uint32_t b;                                                                                               \
        uint32_t c;                                                                                               \
        uint32_t d;                                                                                               \
    };                                                                                                            \
    std::string ToString() const { Guid guid = *(Guid*)this; return guid.ToString(); };                           \
    void FromString(const char* s) { *(Guid*)this = GuidFromString(s); };                                         \
    void New() { *(Guid*)this = NewGuid(); };                                                                     \
    static inline    [[nodiscard]] name GetNew() { name r; *(Guid*)&r = NewGuid(); return r; };                   \
    inline constexpr [[nodiscard]] bool IsValid() const { return e[0] && e[1]; };                                 \
};                                                                                                                \
inline constexpr [[nodiscard]] bool operator==(const name& a, const name& b) { return *(Guid*)&a == *(Guid*)&b; };\
inline constexpr [[nodiscard]] bool operator< (const name& a, const name& b) { return *(Guid*)&a <  *(Guid*)&b; };\
enum {}

struct RunProcessJob : Job
{
    std::wstring path;
    std::wstring args;
    AsyncData<std::string>* output;
    virtual void RunJob() override;
};

struct RunProcessLogToFileJob : Job
{
    std::wstring path;
    std::wstring args;
    AsyncData<std::string>* output;
    AsyncData<Path> output_file;
    Atomic<bool>* completed;
    bool run_and_clear = false;
    virtual void RunJob() override;
};