#define WIN32_LEAN_AND_MEAN
#define _WIN32_DCOM
#include <Windows.h>
#include <shellapi.h>
#include <combaseapi.h>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <iphlpapi.h>
#include <comdef.h>
#include <Wbemidl.h>

#include "OSWindows.h"
#include "WinInterop_File.h"
#include "Math.h"
#include "String.h"
#include "resource.h"
#define STB_IMAGE_IMPLEMENTATION
#include "stb/stb_image.h"
#include "Json.hpp"
#include "Rendering.h"
#include "Tracy.hpp"
#include "Networking.h"
#include "System.h"

#include <fstream>
#include <filesystem>
#include <cwctype>
#include <format>
#include <iostream>
#include <chrono>
#include <charconv>

std::string SysIP4::ToString() const
{
    std::string r = {};
    if (!IsValid())
        return r;
    //"-1" to remove the length of null terminator
    r.resize(INET_ADDRSTRLEN - 1);
    if (InetNtopA(AF_INET, (IN_ADDR*)(&addr), r.data(), r.size()) == NULL)
    {
        DebugPrint("Error: Failed to convert IPv4 to string: {%i}", addr);
        FAIL;
    }
    return r;
}
std::wstring SysIP4::ToWString() const
{
    std::wstring r = {};
    if (!IsValid())
        return r;
    //"-1" to remove the length of null terminator
    r.resize(INET_ADDRSTRLEN - 1);
    if (InetNtopW(AF_INET, (IN_ADDR*)(&addr), r.data(), r.size()) == NULL)
    {
        DebugPrint("Error: Failed to convert IPv4 to string: {%i}", addr);
        FAIL;
    }
    return r;
}
void SysIP4::FromString(const std::string& in)
{
    if (in.size() == 0)
        return;
    if (InetPtonA(AF_INET, in.c_str(), e) == NULL)
    {
        DebugPrint("Error: Failed to convert string to IPv4: {%s}", in.c_str());
        FAIL;
    }
}

std::string SysIP6::ToString() const
{
    std::string r = {};
    if (!IsValid())
        return r;
    //"-1" to remove the length of null terminator
    r.resize(INET6_ADDRSTRLEN - 1);
    if (InetNtopA(AF_INET6, (IN6_ADDR*)(&addr), r.data(), r.size()) == NULL)
    {
        DebugPrint("Error: Failed to convert IPv6 to string: {%i, %i}", addr[0], addr[1]);
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
    mask = htonl(ip.addr);
}

SysIP6 SysIP6Subnet::ToIP6() const
{
    SysIP6 r;
    r.addr[0] = htonll(mask[0]);
    r.addr[1] = htonll(mask[1]);
    return r;
}


void OSWriteToAttachedConsole(const char* buffer, bool add_new_line)
{
    //If we have a console, print there too
    HANDLE console = GetStdHandle(STD_OUTPUT_HANDLE);
    if (console != NULL && console != INVALID_HANDLE_VALUE)
    {
        DWORD mode;
        if (GetConsoleMode(console, &mode)) // succeeds only if console attached
        {
            DWORD written;
            WriteConsoleA(console, buffer, (DWORD)strlen(buffer), &written, NULL);
            if (add_new_line)
            {
                const char* new_line = "\n";
                WriteConsoleA(console, new_line, (DWORD)strlen(new_line), &written, NULL);
            }
        }
    }
}
void OSWriteToAttachedConsole(const wchar_t* buffer, bool add_new_line)
{
    //If we have a console, print there too
    HANDLE console = GetStdHandle(STD_OUTPUT_HANDLE);
    if (console != NULL && console != INVALID_HANDLE_VALUE)
    {
        DWORD mode;
        if (GetConsoleMode(console, &mode)) // succeeds only if console attached
        {
            DWORD written;
            WriteConsoleW(console, buffer, (DWORD)wcslen(buffer), &written, NULL);
            if (add_new_line)
            {
                const wchar_t* new_line = L"\n";
                WriteConsoleW(console, new_line, (DWORD)wcslen(new_line), &written, NULL);
            }
        }
    }
}

void OSDebugOutput(const char* s)
{
    OutputDebugStringA(s);
}
void OSDebugOutput(const wchar_t* s)
{
    OutputDebugStringW(s);
}

struct CoInitializer
{
    HRESULT result;
    CoInitializer()
    {
        const DWORD param1 = COINIT_MULTITHREADED | COINIT_DISABLE_OLE1DDE;
        const DWORD param2 = COINIT_APARTMENTTHREADED | COINIT_DISABLE_OLE1DDE;
        result = CoInitializeEx(NULL, param1);
        if (result == S_FALSE)
        {
            DebugPrint("Verbose: CoInitializeEx() already ran on this thread with %u", param1);
        }
        else if (result == RPC_E_CHANGED_MODE)
        {
            DebugPrint("Warning: CoInitializeEx() previously ran with different parameters: %u", param1);
            result = CoInitializeEx(NULL, param2);
			ASSERT(result == S_OK || result == S_FALSE);
        }

        HRESULT r = CoInitializeSecurity(NULL, -1, NULL, NULL, RPC_C_AUTHN_LEVEL_DEFAULT, RPC_C_IMP_LEVEL_IMPERSONATE, NULL, EOAC_NONE, NULL);
        if (r == RPC_E_TOO_LATE)
        {
            DebugPrint("Warning: CoInitializeSecurity() already ran");
        }
        else if (r == RPC_E_NO_GOOD_SECURITY_PACKAGES)
        {
            DebugPrint("Error: CoInitializeSecurity() got issues");
            FAIL;
            return;
        }
    }
    ~CoInitializer()
    {
        CoUninitialize();
    }

};

i32 OSRunShellProcess(const wchar_t* path, const wchar_t* args, std::string* output, Mutex* output_lock, RunProcessFlags flags)
{
    //TODO: Allow this to work for ASCII AND Unicode
    SHELLEXECUTEINFO info = {};
    info.cbSize = sizeof(SHELLEXECUTEINFO);
    info.fMask = SEE_MASK_NOASYNC | SEE_MASK_NOCLOSEPROCESS;
    info.hwnd;
    //info.lpVerb = "open";
    info.lpVerb = NULL;
    info.lpFile = path ? path : L"cmd.exe";
    info.lpParameters = args;
    info.lpDirectory = NULL;
    info.nShow = flags & RunProcess_Show ? SW_SHOW : SW_HIDE;
    info.hInstApp = NULL; //out
    info.lpIDList;
    info.lpClass;
    info.hkeyClass;
    info.dwHotKey;
    info.hProcess; //out

    CoInitializer co_initializer;
    Defer {
        CloseHandle(info.hProcess);
        CoUninitialize();
    };
    if (!ShellExecuteEx(&info))
    {
        std::wstring errorBoxTitle = ToString(L"ShellExecuteEx Error: %i", GetLastError());
        std::wstring errorText     = ToString(L"Application Path: %s\n"
                                             "Command Line Params: %s", info.lpFile, args);
        OSShowErrorWindow(errorBoxTitle, errorText);
        ASSERT(false);
        return 2;
    }
    if (!(flags & RunProcess_Async))
    {
        DWORD result = WaitForSingleObject(info.hProcess, INFINITE);
        if (result)
        {
            std::wstring errorBoxTitle = ToString(L"WaitForSingleObject Error: %i", GetLastError());
            std::wstring errorText = ToString(L"Application Path: %s\n"
                "Command Line Params: %s", info.lpFile, args);
            OSShowErrorWindow(errorBoxTitle, errorText);
            ASSERT(false);
            return -1;
        }
        DWORD exitCode = {};
        if (!GetExitCodeProcess(info.hProcess, &exitCode))
        {
            std::wstring errorBoxTitle = ToString(L"GetExitCodeProcess Error: %i", GetLastError());
            std::wstring errorText = ToString(L"Application Path: %s\n"
                "Command Line Params: %s", info.lpFile, args);
            OSShowErrorWindow(errorBoxTitle, errorText);
            return -1;
        }
        if (exitCode)
        {
            std::wstring werrorBoxTitle = ToString(L"Program Exited with Code: %i", exitCode);
            std::wstring werrorText = ToString(L"Application Path: %s\n"
                L"Command Line Params: %s", info.lpFile, args);
            std::string error_box_title;
            std::string error_text;
            SysConvertWideCharToMultiByte(error_box_title, werrorBoxTitle);
            SysConvertWideCharToMultiByte(error_text, werrorText);
            return SysShowCustomErrorWindow(error_box_title, error_text);
        }
    }
    return 0;
}

i32 OSRunProcess(const std::wstring& path, const std::wstring& args, AsyncData<std::string>* output, AsyncData<Path>* output_file, RunProcessFlags flags)
{
    std::string zone_text;
    std::string zone_name;
    GetNameAndTextForJob(zone_text, zone_name, path, args);
    ZoneScoped;
    ZoneName(zone_name.c_str(), zone_name.size());
    ZoneText(zone_text.c_str(), zone_text.size());

    SECURITY_ATTRIBUTES sa = {
        .nLength = sizeof(sa),
        .bInheritHandle = TRUE,
    };

    HANDLE readPipe = NULL;
    HANDLE writePipe = NULL;
    CreatePipe(&readPipe, &writePipe, &sa, 0);
    SetHandleInformation(readPipe, HANDLE_FLAG_INHERIT, 0);

    STARTUPINFOW si{
        .cb = sizeof(si),
        .dwFlags = STARTF_USESTDHANDLES,
        .hStdInput = GetStdHandle(STD_INPUT_HANDLE),
        .hStdOutput = writePipe,
        .hStdError = writePipe,
    };

    std::wstring real_path;
    if (path.size() > 1)
        real_path = path;
    else
        real_path = L"cmd.exe /C";

    std::wstring cmdline = real_path;
    if (args.size())
        cmdline += real_path + L" " + args;

    PROCESS_INFORMATION pi = {};
    BOOL r = CreateProcessW(
        nullptr,
        (LPWSTR)cmdline.c_str(),
        nullptr, nullptr,
        TRUE, // inherit handles
        (flags & RunProcess_Show) ? CREATE_NEW_CONSOLE : CREATE_NO_WINDOW,
        nullptr, nullptr,
        &si, &pi
    );

    if (r == 0)
    {
        std::wstring errorBoxTitle = ToString(L"CreateProcess() Error: %i", GetLastError());
        std::wstring errorText     = ToString(L"Application Path: %s\n"
                                              L"Command Line Params: %s", real_path.c_str(), args.c_str());
        DebugPrint("%s\n", errorBoxTitle.c_str());
        DebugPrint(errorText.c_str());
        DebugPrint("\n");
        SysShowErrorWindow(errorBoxTitle, errorText);
        FAIL;
        return 2;
    }

    CloseHandle(writePipe); // parent reads only

    if (output)
    {
        char buffer[4096];
        DWORD bytesRead;
        while (ReadFile(readPipe, buffer, sizeof(buffer), &bytesRead, nullptr))
        {
            TRACY_LOCK(output->lock);
            output->data.append(buffer, bytesRead);
        }
    }
    if (output_file)
    {
        if (output_file->data.empty())
        {
            std::string p;
            SysConvertWideCharToMultiByte(p, path);
            std::string a;
            SysConvertWideCharToMultiByte(a, args);
            DebugPrint("RunProcess() has output_file specified but no data: \"%s\" \"%s\"", p.c_str(), a.c_str());
        }
        else
        {
            ZoneScopedN("Output File");
            std::fstream file(output_file->data, std::ios_base::out);
            if (!file.good())
            {
                std::string of;
                SysConvertWideCharToMultiByte(of, output_file->data);
                DebugPrint("Failed to open file for write: %s", of.c_str());
                FAIL;
                r = ERROR_TOO_MANY_OPEN_FILES;
            }
            else
            {
                TRACY_LOCK(output_file->lock);
                file << output;
            }
        }
    }

    if (!(flags & RunProcess_Async))
        DWORD result = WaitForSingleObject(pi.hProcess, INFINITE);
    CloseHandle(readPipe);
    CloseHandle(pi.hProcess);
    CloseHandle(pi.hThread);

    return r;
}

#define TRACY_SET_NAME_FOR_JOB(app, args)\
    ZoneScoped;                                                                         \
    const std::wstring cmdlinew = app.size() ? app + L" " + args : args;                \
    std::string cmdline;                                                                \
    SysConvertWideCharToMultiByte(cmdline, cmdlinew);                                      \
    cmdline.find_first_of('')\
    const std::string zone_name = cmdline;                                              \
    ZoneName(zone_name.c_str(), zone_name.size())                                       \
    \
    ZoneText()
    //std::string zone_name = ToString("Process Log To File Job: %s", cmdline.c_str());

void ParsePowershell(PowershellResponse& out, const std::string& in)
{
    const i32 titles_index = 1;
    const i32 lines_index = 2;
    const i32 data_start_index = 3;
    if (data_start_index >= in.size())
        return;
    if (in[0] != '\r' || in[1] != '\n')
        return;
    const std::vector<std::string> strings = TextToStringArray(in.c_str(), "\n");
    if (strings.size() < 3)
        return;
    const std::string& titles = strings[titles_index];
    const std::string& lines = strings[lines_index];
    if (lines[0] != '-')
        return;

    std::vector<i32> column_lengths;
    for (i32 i = 0; i < lines.size(); i++)
    {
        i32 dash_count = i;
        for (; dash_count < lines.size(); dash_count++)
        {
            if (lines[dash_count] != '-')
                break;
        }

        i32 column_width = dash_count;
        for (; column_width < lines.size(); column_width++)
        {
            if (lines[column_width] != ' ')
                break;
        }
        i32 width = column_width - i;
        if (width <= 0)
            break;
        column_lengths.push_back(width);
        i = column_width - 1;
    }

    const i32 max_rows = (i32)strings.size() - 2;
    const size_t max_column = Min(std::tuple_size_v<PowershellResponse::value_type>, column_lengths.size());

    i32 out_index = 0;
    for (i32 strings_index = titles_index; strings_index < max_rows; strings_index++)
    {
        if (strings_index == lines_index)
            continue;
        const std::string& s = strings[strings_index];
        i32 previous_len = 0;
        out.push_back({});
        for (i32 j = 0; j < max_column; j++)
        {
            const i32 len = column_lengths[j];
            std::string& s_out = out[out_index][j];
            s_out = s.substr(previous_len, len);
            StringRemoveLeading(s_out, ' ');
            StringRemoveTrailing(s_out, ' ');
            previous_len += len;
        }
        ++out_index;
    }
}

void* OSGetWindowHandle(SDL_Window* window)
{
    SDL_PropertiesID props = SDL_GetWindowProperties(window);
    HWND hwnd = (HWND)SDL_GetPointerProperty(props, SDL_PROP_WINDOW_WIN32_HWND_POINTER, NULL);
    return hwnd;
}

bool OSInit(SDL_Window* window)
{
    //HMODULE modh = GetModuleHandle(NULL);
    //VALIDATE_V(modh != NULL, false);

    HWND hwnd = (HWND)OSGetWindowHandle(window);
    if (!hwnd)
    {
        DebugPrint("Failed to get HWND: %s", SDL_GetError());
        FAIL;
        return false;
    }

    {
        DWORD name_size = MAX_COMPUTERNAME_LENGTH + 1;
        g_sysinfo.name.resize(name_size);
        if (!GetComputerNameW(g_sysinfo.name.data(), &name_size))
        {
            DebugPrint("Failed to get Computer Name error: %i", GetLastError());
            FAIL;
            return false;
        }
        g_sysinfo.name.resize(name_size);
    }

    {
        SYSTEM_LOGICAL_PROCESSOR_INFORMATION info[1024] = {};
        DWORD buffer_size = sizeof(info);
        if (!GetLogicalProcessorInformation(info, &buffer_size))
        {
            DebugPrint("Failed to get processor information error: %i", GetLastError());
            FAIL;
            return false;
        }

        i32 count = buffer_size / sizeof(_SYSTEM_LOGICAL_PROCESSOR_INFORMATION);
        g_sysinfo.cores = 0;
        g_sysinfo.threads = 0;

        for (i32 i = 0; i < count; ++i)
        {
            if (info[i].Relationship == RelationProcessorCore)
            {
                g_sysinfo.cores++;
                ULONG_PTR mask = info[i].ProcessorMask;
                while (mask)
                {
                    g_sysinfo.threads += (mask & 1);
                    mask >>= 1;
                }
            }
        }

        if (g_sysinfo.cores == 0 || g_sysinfo.threads == 0)
        {
            DebugPrint("Error getting cpu and thread counts: %i %i", g_sysinfo.cores, g_sysinfo.threads);
        }
    }
    return true;
}

void OSDestroy(SDL_Window* window)
{
    SDL_DestroyWindow(window);
}

//#pragma comment(lib, "iphlpapi.lib")
//#pragma comment(lib, "ws2_32.lib")

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
    WSADATA wsa_data;
    if (WSAStartup(MAKEWORD(2, 2), &wsa_data))
    {
        DebugPrint("Error: WSAStartup failed");
        return false;
    }

    ULONG buf_len = Kilobytes(15);
    std::vector<u8> buffer(buf_len);
    PIP_ADAPTER_ADDRESSES adapter_addresses = (PIP_ADAPTER_ADDRESSES)buffer.data();

    ULONG flags = GAA_FLAG_INCLUDE_PREFIX | GAA_FLAG_INCLUDE_WINS_INFO | GAA_FLAG_INCLUDE_GATEWAYS;
    DWORD r = GetAdaptersAddresses(AF_UNSPEC, flags, NULL, adapter_addresses, &buf_len);
    if (r == ERROR_BUFFER_OVERFLOW)
    {
        buffer.resize(buf_len);
        adapter_addresses = (PIP_ADAPTER_ADDRESSES)buffer.data();
        r = GetAdaptersAddresses(AF_UNSPEC, flags, nullptr, adapter_addresses, &buf_len);
    }
    if (r != ERROR_SUCCESS)
    {
        DebugPrint("Error Failed to GetAdaptersAddresses()");
        return false;
    }

    PIP_ADAPTER_ADDRESSES adapter = adapter_addresses;
    while (adapter)
    {
        SysNetworkAdapterInfo ad = {};
        ad.index = adapter->IfIndex;
        ad.name = adapter->AdapterName;
        ad.friendly_name = adapter->FriendlyName;
        ad.description = adapter->Description;
        ad.status = adapter->OperStatus == IfOperStatusUp ? "Up" : "Down";
        ad.ipv4_enabled = adapter->Ipv4Enabled;
        ad.ipv6_enabled = adapter->Ipv6Enabled;
        ad.dhcpv4_enabled = adapter->Dhcpv4Enabled;
        ad.ipv4_metric = adapter->Ipv4Metric;
        ad.ipv6_metric = adapter->Ipv6Metric;
        ad.ddns_enabled = adapter->DdnsEnabled;
        ad.domain_dns_register_enabled = adapter->RegisterAdapterSuffix;
        ad.receive_only = adapter->ReceiveOnly;
        ad.multicast_enabled = !adapter->NoMulticast;

        if (adapter->PhysicalAddressLength != 0)
            ad.mac_address.SetBytes(adapter->PhysicalAddress, adapter->PhysicalAddressLength);

        // Get IP Addresses (Unicast)
        PIP_ADAPTER_UNICAST_ADDRESS unicast = adapter->FirstUnicastAddress;
        while (unicast)
        {
            char ip_str[INET6_ADDRSTRLEN] = {};
            sockaddr* sa = unicast->Address.lpSockaddr;
            const u8 prefix_len = unicast->OnLinkPrefixLength;
            if (sa)
            {
                if (sa->sa_family == AF_INET) //IPv4
                {
                    SysIP4AndSubnet ips;
                    ips.ip = GetIP4FromSockaddr(sa);
                    ips.subnet.FromLength(prefix_len);
                    ad.ipv4_ips.push_back(ips);
                }
                else if (sa->sa_family == AF_INET6) //IPv6
                {
                    SysIP6AndSubnet ips = {};
                    ips.ip = GetIP6FromSockaddr(sa);
                    ips.subnet.FromLength(prefix_len);
                    ad.ipv6_ips.push_back(ips);
                }
            }

            unicast = unicast->Next;
        }

        // Get DNS Servers
        PIP_ADAPTER_DNS_SERVER_ADDRESS dns = adapter->FirstDnsServerAddress;
        while (dns)
        {
            char dns_str[INET6_ADDRSTRLEN] = {};
            sockaddr* sa = dns->Address.lpSockaddr;

            if (sa)
            {
                if (sa->sa_family == AF_INET) //IPv4
                {
                    ad.ipv4_dns.push_back(GetIP4FromSockaddr(sa));
                }
                else if (sa->sa_family == AF_INET6) //IPv6
                {
                    ad.ipv6_dns.push_back(GetIP6FromSockaddr(sa));
                }
            }
            dns = dns->Next;
        }

        // Get DNS Domain/Suffix
        if (wcslen(adapter->DnsSuffix) > 0)
        {
            ad.dns_domain = adapter->DnsSuffix;
        }

        // Get Default Gateways
        PIP_ADAPTER_GATEWAY_ADDRESS gateway = adapter->FirstGatewayAddress;
        while (gateway)
        {
            char g_s[INET6_ADDRSTRLEN] = { };
            sockaddr* sa = gateway->Address.lpSockaddr;
            if (sa)
            {
                if (sa->sa_family == AF_INET)
                {
                    ad.ipv4_gateways.push_back(GetIP4FromSockaddr(sa));
                }
                else if (sa->sa_family == AF_INET6)
                {
                    ad.ipv6_gateways.push_back(GetIP6FromSockaddr(sa));
                }
            }
            gateway = gateway->Next;
        }

        // Get DHCP
        ASSERT(ad.dhcpv4_enabled == !!(adapter->Flags & IP_ADAPTER_DHCP_ENABLED));
        if (adapter->Flags & IP_ADAPTER_DHCP_ENABLED)
        {
            char dhcp_s[INET6_ADDRSTRLEN] = {};
            sockaddr* sa = adapter->Dhcpv4Server.lpSockaddr;
            if (sa)
            {
                if (sa->sa_family == AF_INET)
                {
                    ad.ipv4_dhcp = GetIP4FromSockaddr(sa);
                }
                else if (sa->sa_family == AF_INET6)
                {
                    ad.ipv6_dhcp = GetIP6FromSockaddr(sa);
                }
            }
        }

        adapters.push_back(ad);
        adapter = adapter->Next;
    }

    WSACleanup();
    return true;
}

// Creates a COM SafeArray containing a single string
VARIANT CreateSafeArray(const std::string& str)
{
    VARIANT r;
    r.vt = VT_ARRAY | VT_BSTR;
    r.parray = SafeArrayCreateVector(VT_BSTR, 0, 1);
    long index = 0;
    _bstr_t bstrVal(str.c_str());
    HRESULT result = SafeArrayPutElement(r.parray, &index, bstrVal.GetBSTR());
    if (result == DISP_E_BADINDEX)
    {
        DebugPrint("Error: SafeArrayPutElement() has invalid index: %i", index);
        FAIL;
    }
    else if (result == E_INVALIDARG)
    {
        DebugPrint("Error: SafeArrayPutElement() has an invalid argument");
        FAIL;
    }
    else if (result == E_OUTOFMEMORY)
    {
        DebugPrint("Error: SafeArrayPutElement() ran out of memory");
        FAIL;
    }
    return r;
}

VARIANT CreateSafeArray(const ArrayView<std::string>& strings)
{
    VARIANT var;
    VariantInit(&var);
    var.vt = VT_ARRAY | VT_BSTR; 
    SAFEARRAY* safe_array = SafeArrayCreateVector(VT_BSTR, 0, (ULONG)strings.size());
    for (LONG i = 0; i < strings.size(); i++)
    {
        _bstr_t bstr(strings[i].c_str());
        SafeArrayPutElement(safe_array, &i, (void*)bstr.GetBSTR());
    }
    var.parray = safe_array;
    return var;
}

struct WMIClassObject
{
    IWbemClassObject* obj = nullptr;
    ~WMIClassObject()
    {
        Clear();
    }
    bool IsValid() const { return !!obj; }
    void Clear()
    {
        obj->Release();
        obj = nullptr;
    }
};

struct WMIServices
{
    IWbemServices* services = nullptr;
    IWbemLocator* locator = nullptr;
    WMIServices()
    {
        if (FAILED(CoCreateInstance(CLSID_WbemLocator, NULL, CLSCTX_INPROC_SERVER, IID_IWbemLocator, (LPVOID*)&locator)))
        {
            DebugPrint("Error: Failed to create instance for WbemLocator");
            FAIL;
            return;
        }
        if (FAILED(locator->ConnectServer(_bstr_t(L"ROOT\\CIMV2"), NULL, NULL, NULL, NULL, NULL, NULL, &services)))
        {
            DebugPrint("Error: Failed to ConnectServer() with locator");
            FAIL;
            Clear();
            return;
        }
        if (FAILED(CoSetProxyBlanket(services, RPC_C_AUTHN_WINNT, RPC_C_AUTHZ_NONE, NULL, RPC_C_AUTHN_LEVEL_CALL, RPC_C_IMP_LEVEL_IMPERSONATE, NULL, EOAC_NONE)))
        {
            DebugPrint("Error: Failed to CoSetProxyBlanket()");
            FAIL;
            Clear();
            return;
        }
    }

    void Clear()
    {
        locator->Release();
        services->Release();
        services = nullptr;
    }
    bool IsValid() const
    {
        return !!locator && !!services;
    }
    ~WMIServices()
    {
        Clear();
    }

    i32 GetAdapterIndexByGuid(const std::string& adapter_guid)
    {
        const std::string query = ToString("SELECT Index FROM Win32_NetworkAdapterConfiguration WHERE SettingID = '%hs'", adapter_guid.c_str());
        IEnumWbemClassObject* enumerator = nullptr;
        if (FAILED(services->ExecQuery(_bstr_t(L"WQL"), _bstr_t(query.c_str()), WBEM_FLAG_FORWARD_ONLY | WBEM_FLAG_RETURN_IMMEDIATELY, NULL, &enumerator)))
        {
            DebugPrint("Error: Failed to find adapter for guid: %hs", adapter_guid.c_str());
            FAIL;
            return -1;
        }
        Defer{ enumerator->Release(); };
        IWbemClassObject* adapter = NULL;
        ULONG return_val = 0;
        i32 index = -1;
        if (FAILED(enumerator->Next(WBEM_INFINITE, 1, &adapter, &return_val)) || return_val == 0)
        {
            DebugPrint("Error: enumerator->Next(WBEM_INFINITE, 1, &adapter, &return_val)) failed: return: %i", return_val);
            return -1;
            FAIL;
        }
        VARIANT var;
        adapter->Get(L"Index", 0, &var, 0, 0);
        index = var.intVal;
        VariantClear(&var);
        adapter->Release();
        return index;
    }

    WMIClassObject GetClassObject(const _bstr_t* class_object_path_bstr, const char* class_object_path_str)
    {
        WMIClassObject class_object = {};
        if (FAILED(services->GetObjectW(*class_object_path_bstr, 0, NULL, &class_object.obj, NULL)))
        {
            DebugPrint("Error: GetObjectW() failed to get %s: %i", class_object_path_str, GetLastError());
            ASSERT(!class_object.IsValid());
            FAIL;
        }
        return class_object;
    }
    WMIClassObject GetClassObject(const char* object_str)
    {
        _bstr_t object_bstr(object_str);
        return GetClassObject(&object_bstr, object_str);
    }
};

template <typename T>
void SafeRelease(T*& unknown)
{
    if (unknown)
    {
        unknown->Release();
        unknown = nullptr;
    }
}

struct WinFunc
{
    WMIServices* services;
    std::string  func_name;
    std::wstring func_namew;
    std::string inst_object_path;
    _bstr_t     inst_object_path_bstr;
    std::string class_object_path;
    _bstr_t     class_object_path_bstr;
    IWbemClassObject* args_in = nullptr;
    IWbemClassObject* args_out = nullptr;
    IWbemClassObject* args_inst_in = nullptr;
    std::vector<VARIANT> input_params;
    bool is_valid = false;

    WinFunc(WMIServices* in_services, const std::wstring& w_str_inst_object_path, const std::wstring& w_str_class_object_path, const wchar_t* function_name) :
        inst_object_path_bstr(w_str_inst_object_path.c_str()),
        class_object_path_bstr(w_str_class_object_path.c_str()),
        services(in_services),
        func_namew(function_name)
    {
        OSConvertWideCharToMultiByte(inst_object_path,  w_str_inst_object_path);
        OSConvertWideCharToMultiByte(class_object_path, w_str_class_object_path);
        OSConvertWideCharToMultiByte(func_name, func_namew);

        WMIClassObject class_object = services->GetClassObject(&class_object_path_bstr, class_object_path.c_str());
        if (!class_object.IsValid())
        {
            FAIL;
            return;
        }

        if (FAILED(class_object.obj->GetMethod(func_namew.c_str(), 0, &args_in, &args_out)))
        {
            DebugPrint("Error: GetMethod(%s) failed: %i", func_name.c_str(), GetLastError());
            FAIL;
            return;
        }

        if (args_in)
        {
            if (FAILED(args_in->SpawnInstance(0, &args_inst_in)))
            {
                DebugPrint("Error: SpawnInstance() for in args for %s() failed: %i", func_name.c_str(), GetLastError());
                FAIL;
                return;
            }
        }
        is_valid = true;
    }

    bool AddInputParam(const wchar_t* var_name, const char* var_val)
    {
        VALIDATE_V(args_inst_in, false);
        VALIDATE_V(is_valid, false);
        VALIDATE_V(var_name, false);
        VALIDATE_V(var_val, false);

        VARIANT v = CreateSafeArray(var_val);
        return AddInputParam(var_name, &v);
    }
    bool AddInputParam(const wchar_t* var_name, VARIANT* var_val)
    {
        VALIDATE_V(args_inst_in, false);
        VALIDATE_V(is_valid, false);
        VALIDATE_V(var_name, false);
        VALIDATE_V(var_val, false);

        input_params.push_back(*var_val);
        VARIANT& v = input_params.back();
        if (FAILED(args_inst_in->Put(var_name, 0, &v, 0)))
        {
            std::string s;
            OSConvertWideCharToMultiByte(s, var_name);
            DebugPrint("Error: Put(%s) failed: %i", s.c_str(), GetLastError());
            FAIL;
            return false;
        }
        return true;
    }
    

    bool Execute(i32* return_val = nullptr)
    {
        const HRESULT r = services->services->ExecMethod(inst_object_path_bstr, _bstr_t(func_namew.c_str()), 0, NULL, args_inst_in, &args_out, NULL);
        if (FAILED(r))
        {
            DebugPrint("Error: ExecMethod(%s) failed: %i, HRESULT: 0x%X", func_name.c_str(), GetLastError(), r);
            FAIL;
            return false;
        }

        if (args_out)
        {
            VARIANT ret_val;
            VariantInit(&ret_val);
            if (SUCCEEDED(args_out->Get(L"ReturnValue", 0, &ret_val, NULL, 0)))
            {
                switch (ret_val.vt)
                {
                case VT_I4:
                {
                    i32 status = ret_val.intVal;
                    DebugPrint("%s() Result Code: %i", func_name.c_str(), status);
                    if (return_val)
                        *return_val = status;
                    break;
                }
                default:
                {
                    DebugPrint("Warning: Unsupported return value type (%u) for function: %s()", ret_val.vt, func_name.c_str());
                    FAIL;
                    break;
                }
                }
            }
            VariantClear(&ret_val);
        }
        return true;
    }

    ~WinFunc()
    {
        SafeRelease(args_in);
        SafeRelease(args_out);
        SafeRelease(args_inst_in);
        for (i32 i = 0; i < input_params.size(); i++)
        {
            VariantClear(&input_params[i]);
        }
        is_valid = false;
    }
};

bool OSSetNetAdapterIP(const std::string& adapter_guid, const SysNetAdapterConfig& adapter, const SysNetAdapterConfig& src_adapter)
{
    VALIDATE_V((adapter.ip.ip.IsValid() && adapter.ip.subnet.IsValid()) || adapter.dhcp_enabled, false);
    VALIDATE_V(adapter_guid.size(), false);

    CoInitializer co_initializer;
    WMIServices service;
    if (!service.IsValid())
        return false;

    i32 adapter_index = service.GetAdapterIndexByGuid(adapter_guid);
    if (adapter_index == -1)
        return false;

    const wchar_t* wmi_class_str = L"Win32_NetworkAdapterConfiguration";
    const std::wstring class_inst_name = ToString(L"%s.Index=%i", wmi_class_str, adapter_index);
    if (adapter.dhcp_enabled) 
    {
        WinFunc enable_dhcp_func(&service, class_inst_name, wmi_class_str, L"EnableDHCP");
        if (!enable_dhcp_func.Execute())
            return false;
    }
    else 
    {
        WinFunc enable_static_func(&service, class_inst_name, wmi_class_str, L"EnableStatic");
        if (!enable_static_func.AddInputParam(L"IPAddress", adapter.ip.ip.ToString().c_str()))
            return false;
        if (!enable_static_func.AddInputParam(L"SubnetMask", adapter.ip.subnet.ToIP4().ToString().c_str()))
            return false;
        i32 result_code = 0;
        if (!enable_static_func.Execute(&result_code))
            return false;

        //0 = Success, 1 = Success (Reboot required)
        if (result_code != 0 && result_code != 1)
        {
            DebugPrint("Error: WMI rejected the IP change. (See MS documentation for code %i)", result_code);
            FAIL;
        }

        if (adapter.gateway.IsValid()) 
        {
            WinFunc set_gatway_fun(&service, class_inst_name, wmi_class_str, L"SetGateways");
            if (!set_gatway_fun.is_valid)
                return false;
            if (!set_gatway_fun.AddInputParam(L"DefaultIPGateway", adapter.gateway.ToString().c_str()))
                return false;
            if (!set_gatway_fun.Execute())
                return false;
        }
    }
    return true;
}

bool OSSetNetAdapterDNS(const std::string& adapter_guid, const SysNetAdapterConfig& adapter, const SysNetAdapterConfig& src_adapter)
{
    VALIDATE_V((adapter.dns[0].IsValid() || adapter.dns[1].IsValid()) || adapter.dhcp_enabled, false);
    if (adapter.ddns_enabled && !adapter.dhcp_enabled)
    {
        DebugPrint("Warning: Attempting to set Dynamic DNS with static IP");
        return false;
    }

    CoInitializer co_initializer;
    WMIServices service;
    if (!service.IsValid())
        return false;

    i32 adapter_index = service.GetAdapterIndexByGuid(adapter_guid);
    if (adapter_index == -1)
        return false;

    const wchar_t* wmi_class_str = L"Win32_NetworkAdapterConfiguration";
    const std::wstring class_inst_name = ToString(L"%s.Index=%i", wmi_class_str, adapter_index);

    WinFunc Set_dns_func(&service, class_inst_name, wmi_class_str, L"SetDNSServerSearchOrder");
    VARIANT dns_variant;
    if (adapter.ddns_enabled)
    {
        dns_variant.vt = VT_NULL;
    }
    else
    {
        std::string dns_strings[SYS_NET_CONFIG_MAX_DNS];
        for (i32 i = 0; i < SYS_NET_CONFIG_MAX_DNS; i++)
        {
            dns_strings[i] = adapter.dns[i].ToString();
        }
        dns_variant = CreateSafeArray(CreateArrayView(dns_strings));
    }
    if (!Set_dns_func.AddInputParam(L"DNSServerSearchOrder", &dns_variant))
        return false;

    i32 return_value = 0;
    if (!Set_dns_func.Execute(&return_value))
        return false;

    switch (return_value)
    {
    case 0: break;
    case 1: DebugPrint("SetDNSServerSearchOrder suceeded but system needst to reboot"); break;
    case 84: DebugPrint("Error: SetDNSServerSearchOrder failed with %i: 'IP not enabled on adapter.'", return_value); return false;
    default:
    {
        DebugPrint("Error: WMI rejected the DNS change. (See MS documentation for code %i)", return_value);
        FAIL;
        return false;
    }
    }
    return true;
}

bool OSHasAdminPrivledge()
{
    bool is_admin = false;
    HANDLE handle = NULL;
    if (OpenProcessToken(GetCurrentProcess(), TOKEN_QUERY, &handle))
    {
        TOKEN_ELEVATION elevation;
        DWORD size = sizeof(TOKEN_ELEVATION);
        if (GetTokenInformation(handle, TokenElevation, &elevation, sizeof(elevation), &size))
        {
            is_admin = (elevation.TokenIsElevated != 0);
        }
        CloseHandle(handle);
    }
    return is_admin;
}

bool OSIsConsoleAttached()
{
    return AttachConsole(ATTACH_PARENT_PROCESS);
}
bool OSIsDebuggerAttached()
{
    return IsDebuggerPresent();
}
void OSHideConsole()
{
    ::ShowWindow(::GetConsoleWindow(), SW_HIDE);
}
void OSShowConsole()
{
    ::ShowWindow(::GetConsoleWindow(), SW_SHOW);
}
bool OSIsConsoleVisible()
{
    return ::IsWindowVisible(::GetConsoleWindow()) != FALSE;
}

void OSShowErrorWindow(const std::wstring& title, const std::wstring& text)
{
#if 1
    int msgboxID = MessageBox(
        NULL,
        text.c_str(),
        title.c_str(),
        MB_ABORTRETRYIGNORE | MB_ICONSTOP | MB_DEFBUTTON1 | MB_APPLMODAL
    );

    switch (msgboxID)
    {
    case IDABORT:
        g_running = false;
        break;
    case IDRETRY:
        break;
    case IDIGNORE:
        break;
    }
#else
    ImGuiWindowFlags flags =
        ImGuiWindowFlags_NoCollapse |
        ImGuiWindowFlags_NoSavedSettings;
    const ImVec2 min = { 260, 100 };
    const ImVec2 windowSize = ImGui::GetMainViewport()->WorkSize;
    const ImVec2 max = {windowSize.x - 200, windowSize.y - 200};
    ImGui::SetNextWindowSizeConstraints(min, max);
    ImGui::SetNextWindowPos(ImVec2(windowSize.x / 2, windowSize.y / 2), ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));
    ImGui::OpenPopup(title.c_str());
    if (ImGui::BeginPopupModal(title.c_str(), NULL, flags))
    {
        ImGui::TextWrapped(text.c_str());
        if (ImGui::Button("Continue"))
            ImGui::CloseCurrentPopup();
        ImGui::SameLine();
        if (ImGui::Button("Copy to Clipboard"))
            SDL_SetClipboardText(text.c_str());
        ImGui::SameLine();
        if (ImGui::Button("Exit"))
        {
            SDL_Event e;
            e.type = SDL_QUIT;
            e.quit.timestamp = 0;
            SDL_PushEvent(&e);
        }
        ImGui::EndPopup();
    }
#endif
}
void OSFlashWindow(SDL_Window* window)
{
    FLASHWINFO info = {};
    info.hwnd = (HWND)OSGetWindowHandle((SDL_Window*)window);
    info.dwFlags = FLASHW_TRAY | FLASHW_TIMERNOFG;
    info.uCount;
    info.dwTimeout;
    info.cbSize = sizeof(info);

    FlashWindowEx(&info);
}

//TODO(CSH): Create filepath helper functions
void _ScanDirectoryForFileNames(const std::wstring& root, const std::wstring& dir, ScannedFiles& out, ScanDirectoryFlags flags)
{
    std::wstring d = root;
    if (d.size() < 2)
    {
        d = L"*";
    }
    else
    {

        size_t end_index = d.find_first_of(L'\0');
        if (end_index == std::wstring::npos)
            end_index = d.size();
        if (d[end_index - 1] != L'*')
        {
            if (d[end_index - 1] != L'/')
            {
                d.insert(end_index, L"/*");
            }
            else
            {
                d.insert(end_index, L"*");
            }
        }
    }

    WIN32_FIND_DATAW find_data;
    HANDLE handle = FindFirstFileW(d.c_str(), &find_data);
    if (handle == INVALID_HANDLE_VALUE)
    {
        DWORD error = GetLastError();
        std::string mb;
        SysConvertWideCharToMultiByte(mb, d);
        DebugPrint(ToString("Error finding files: %s", mb.c_str()).c_str());
        return;
    }
    while (handle != INVALID_HANDLE_VALUE)
    {
        if (find_data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY && find_data.cFileName[0] != '.')
        {
            if (flags & ScanDirectoryFlags_IncludeDirs)
            {
                if (dir.size())
                    out.push_back({ dir + L"/" + find_data.cFileName, true });
                else
                    out.push_back({ find_data.cFileName, true });
            }

            if (flags & ScanDirectoryFlags_Recursive)
            {
                std::wstring new_root = d;
                new_root.pop_back();
                new_root += find_data.cFileName;
                std::wstring new_dir = dir;
                if (dir.size())
                    new_dir = new_dir + L"/" + find_data.cFileName;
                else
                    new_dir += find_data.cFileName;
                _ScanDirectoryForFileNames(new_root, new_dir, out, flags);
            }
        }
        else if (!(find_data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY))
        {
                if (dir.size())
                    out.push_back({ dir + L"/" + find_data.cFileName, false });
                else
                    out.push_back({ find_data.cFileName, false });
        }
        if (FindNextFileW(handle, &find_data) == 0)
        {
            //if (GetLastError() == ERROR_NO_MORE_FILES)
            break;
        }
    }
}

void OSScanDirectoryForFileNames(const std::wstring& dir, ScannedFiles& out, ScanDirectoryFlags flags)
{
    out.clear();
    _ScanDirectoryForFileNames(dir, L"", out, flags);
}

#include "shlobj_core.h"

static int CALLBACK BrowseFolderCallback(HWND hwnd, UINT uMsg, LPARAM lParam, LPARAM lpData)
{
    if (uMsg == BFFM_INITIALIZED) {
        LPCTSTR path = LPCTSTR(lpData);
        ::SendMessage(hwnd, BFFM_SETSELECTION, true, (LPARAM)path);
    }
    return 0;
}

bool OSGetDirectoryFromUser(const std::wstring& currentDir, std::wstring& dir)
{
    std::wstring baseDir = currentDir;
    if (currentDir.size() == 0)
    {
        TCHAR buf[MAX_PATH] = { 0 };
        GetModuleFileName(NULL, buf, MAX_PATH);
        std::wstring::size_type pos = std::wstring(buf).find_last_of(L"\\/");
        baseDir = std::wstring(buf).substr(0, pos);
    }
    dir.clear();
    dir.resize(MAX_PATH);
    int imageIndex = 0;
    BROWSEINFO info = {
        .hwndOwner = (HWND)OSGetWindowHandle(gfx.window),
        .pidlRoot = NULL,
        .pszDisplayName = NULL,//dir.data(),
        .lpszTitle = L"Select Config Directory",
        .ulFlags =  BIF_USENEWUI, //BIF_EDITBOX | BIF_NEWDIALOGSTYLE,
        .lpfn = BrowseFolderCallback,//NULL,
        .lParam = (LPARAM)baseDir.c_str(), //NULL,
        .iImage = imageIndex,
    };
    CoInitializer co_initializer;
    PIDLIST_ABSOLUTE pidl = SHBrowseForFolder(&info);
    if (pidl == NULL)
        return false;
    BOOL result = SHGetPathFromIDList(pidl, dir.data());

    auto pos = dir.find_first_of(L'\0');
    if (pos != std::wstring::npos)
        dir.resize(pos);

    if (result)
    {
        std::wstring_view dir1 = dir;
        std::wstring_view dir2 = baseDir;
        if (dir1.find(dir2) != std::wstring::npos && dir2.find(dir1) != std::wstring::npos)
        {
            dir.clear();
        }
        return true;
    }
    return false;
}

void OSConvertMultibyteToWideChar(std::wstring& out, const std::string& in)
{
    //WideCharToMultiByte
    i32 wide_char_count = MultiByteToWideChar(
        CP_UTF8,                //[in]            UINT                              CodePage,
        MB_ERR_INVALID_CHARS,   //[in]            DWORD                             dwFlags,
        in.c_str(),             //[in]            _In_NLS_string_(cbMultiByte)LPCCH lpMultiByteStr,
        -1,                     //[in]            int                               cbMultiByte,
        nullptr,                //[out, optional] LPWSTR                            lpWideCharStr,
        0                       //[in]            int                               cchWideChar
    );
    ASSERT(wide_char_count > 0);
    out.clear();
    out.resize(wide_char_count);
    i32 wide_char_actual = MultiByteToWideChar(
        CP_UTF8,                //[in]            UINT                              CodePage,
        MB_ERR_INVALID_CHARS,   //[in]            DWORD                             dwFlags,
        in.c_str(),             //[in]            _In_NLS_string_(cbMultiByte)LPCCH lpMultiByteStr,
        -1,                     //[in]            int                               cbMultiByte,
        out.data(),             //[out, optional] LPWSTR                            lpWideCharStr,
        wide_char_count         //[in]            int                               cchWideChar
    );
    ASSERT(wide_char_actual > 0);
    ASSERT(wide_char_actual == wide_char_count);
}

void OSConvertWideCharToMultiByte(std::string& out, const std::wstring& in)
{
    //WideCharToMultiByte
    BOOL invalid_string;

    i32 multibyte_char_count = WideCharToMultiByte(
        CP_UTF8,                //[in]            UINT                               CodePage,
        0,//MB_ERR_INVALID_CHARS,   //[in]            DWORD                              dwFlags,
        in.c_str(),             //[in]            _In_NLS_string_(cchWideChar)LPCWCH lpWideCharStr,
        -1,                     //[in]            int                                cchWideChar,
        nullptr,                //[out, optional] LPSTR                              lpMultiByteStr,
        0,                      //[in]            int                                cbMultiByte,
        "#",                    //[in, optional]  LPCCH                              lpDefaultChar,
        &invalid_string         //[out, optional] LPBOOL                             lpUsedDefaultChar
    );
    ASSERT(multibyte_char_count > 0);
    out.clear();
    out.resize(multibyte_char_count);
    i32 multibyte_char_actual = WideCharToMultiByte(
        CP_UTF8,                //[in]            UINT                               CodePage,
        0,//MB_ERR_INVALID_CHARS,   //[in]            DWORD                              dwFlags,
        in.c_str(),             //[in]            _In_NLS_string_(cchWideChar)LPCWCH lpWideCharStr,
        -1,                     //[in]            int                                cchWideChar,
        out.data(),             //[out, optional] LPSTR                              lpMultiByteStr,
        (i32)out.size(),        //[in]            int                                cbMultiByte,
        "#",                    //[in, optional]  LPCCH                              lpDefaultChar,
        &invalid_string         //[out, optional] LPBOOL                             lpUsedDefaultChar
    );
    ASSERT(multibyte_char_actual > 0);
    ASSERT(multibyte_char_actual == multibyte_char_count);
    if (out.size() && out.back() == '\0')
        out.pop_back();
}

void OSExpandEnvironemntVariable(std::wstring& out, const std::wstring& in)
{
    DWORD size = ExpandEnvironmentStringsW(in.c_str(), nullptr, 0);
    if (size == 0)
    {
        std::string var;
        SysConvertWideCharToMultiByte(var, in);
        DebugPrint("Failed to expand string: \"%s\" error: %i", var.c_str(), GetLastError());
        return;
    }

    out.resize(size);
    ExpandEnvironmentStringsW(in.c_str(), out.data(), size);
    out.resize(size - 1); //Remove trailing null inserted by API
}

#if FEATURE_CUSTOM_ASSERT
#pragma comment(lib, "Comctl32.lib")
#include <commctrl.h>
#include <signal.h> // raise

#if WINVER <= _WIN32_WINNT_WINXP
#define SRW_LOCK_INIT(_lock)
#define SRW_LOCK_ACQUIRE(_lock) _lock.lock()
#define SRW_LOCK_RELASE(_lock) _lock.unlock()
#define SRW_LOCK std::mutex
#define ASSERT_DIALOG(_message, _info, _button) _button = MessageBoxW(NULL,\
                                                                   _info,\
                                                                   _message,\
                                                                   MB_CANCELTRYCONTINUE | MB_ICONWARNING | MB_DEFBUTTON2)

#else
#define SRW_LOCK_INIT(_lock) InitializeSRWLock(_lock)
#define SRW_LOCK_ACQUIRE(_lock) AcquireSRWLockExclusive(&_lock)
#define SRW_LOCK_RELASE(_lock) ReleaseSRWLockExclusive(&_lock)
#define SRW_LOCK SRWLOCK
#define ASSERT_DIALOG(_message, _info, _button) TaskDialog(NULL, NULL, \
                                                           L"Assertion Failed",\
                                                           _message,\
                                                           _info,\
                                                           TDCBF_YES_BUTTON | TDCBF_NO_BUTTON | TDCBF_RETRY_BUTTON | TDCBF_CLOSE_BUTTON, TD_WARNING_ICON,\
                                                           &_button)
#endif

struct AssertRecord
{
    // Key
    const char* file; // Points to what is retrieved from the __FILE__ macro, so it should be stable.
    int         line;

    int         hit_counter;
    bool        ignored;
};


struct SRWLock
{
    SRWLock() { SRW_LOCK_INIT(&lock); }
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
            };
            s_assert_records.push_back(new_record);
            record = &s_assert_records.back();
            record->file = file;
            record->line = line;
        }

        record->hit_counter++;
        if (record->ignored)
        {
            return;
        }


        WCHAR wmessage[1024];
        ArrayView<WCHAR> wmessage_view = CreateArrayView(wmessage);
        wmessage_view.Last() = 0;
        if (MultiByteToWideChar(CP_UTF8, 0, message, -1, wmessage_view.data, (int)wmessage_view.Bytes()) == 0)
        {
            wcscpy_s(wmessage, (size_t)wmessage_view.Bytes(), L"Error");
        }


        const char* s = record->hit_counter == 1 ? "" : "s";
        char info_buffer[1024];
        ArrayView<char> info_buffer_view = CreateArrayView(info_buffer);
#if WINVER == 0x0501
        sprintf_s(info_buffer_view.data, (size_t)info_buffer_view.Bytes(), "%s(%d)\n\n"
                                                     "This has been hit %d time%s.\n\n"
                                                     "Cancel    : Break into debugger\n"
                                                     "Try Again : Continue execution\n"
                                                     "Continue  : Ignore this assert in the future",
                                                     file, line, record->hit_counter, s);
#else
        sprintf_s(info_buffer_view.data, (size_t)info_buffer_view.Bytes(), "%s(%d)\n\n"
                                                     "This has been hit %d time%s.\n\n"
                                                     "Yes   : Break into debugger\n"
                                                     "No    : Continue execution\n"
                                                     "Retry : Ignore this assert in the future\n"
                                                     "Close : Abort the program",
                                                     file, line, record->hit_counter, s);
#endif
        WCHAR winfo[1024];
        auto winfo_view = CreateArrayView(winfo);
        winfo_view.Last() = 0;
        if (MultiByteToWideChar(CP_UTF8, 0, info_buffer, -1, winfo_view.data, (int)winfo_view.Bytes()) == 0)
        {
            wcscpy_s(winfo, (size_t)winfo_view.Bytes(), L"Error");
        }


        int button = 0;
        ASSERT_DIALOG(wmessage, winfo, button);


        switch(button)
        {
        default:
        case IDTRYAGAIN: [[fallthrough]];
        case IDNO: break;
#if WINVER == 0x0501
        case IDCANCEL: [[fallthrough]];
#else
        case IDCANCEL: {
        } break;
#endif
        case IDYES: {
            __debugbreak();
        } break;

        case IDCONTINUE: [[fallthrough]];
        case IDRETRY: {
            if (record)
            {
                record->ignored = true;
            }
        } break;

        case IDCLOSE: {
            // NOTE: This is how the CRT assert works when the abort button is pressed:
            raise(SIGABRT);
            _exit(3);
        } break;
        }
    }
}
#else
#include <assert.h>
void os_assert(bool expr, const char*, const char*, int)
{
    ASSERT(expr);
}
#endif

struct OS {
    HMODULE hmod;
    HWND hwnd;
    Vec2I screen_size;
};
static OS s_os;


int main(int argc, char** argv)
{
    return SysMain(argc, argv);
}
int WINAPI WinMain(HINSTANCE instance, HINSTANCE prev_instance, LPSTR str, int val)
{
    return SysMain(-1, &str);
}

void* OSGetDataFromResource(i32* out_size, const i32 resource_id)
{
    HRSRC handle = FindResource(nullptr, MAKEINTRESOURCE(resource_id), RT_RCDATA);
    DWORD error = GetLastError();
    VALIDATE_V(handle, nullptr);
    HGLOBAL res = LoadResource(nullptr, handle);
    VALIDATE_V(res, nullptr);
    DWORD size = SizeofResource(nullptr, handle);
    if (out_size)
        *out_size = (i32)size;
    void* data = LockResource(res);
    return data;
}

static_assert(sizeof(GUID) == sizeof(Guid));
Guid OSNewGuid()
{
    Guid id;
    if (FAILED(CoCreateGuid((GUID*)&id)))
    {
        const DWORD error = GetLastError();
        FAIL;
        return {};
    }
    return id;
}