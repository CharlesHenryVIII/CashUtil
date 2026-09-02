#pragma once
#include "CashThreading.h"
#include "imgui.h"
#include "CashMath.h"
#include "CashArrayView.h"
#include "CashString.h"
#include "CashSystem.h"

struct SDL_Window;

void OSWriteToAttachedConsole(const char* buffer, bool add_new_line);
void OSWriteToAttachedConsole(const wchar_t* buffer, bool add_new_line);
void OSDebugOutput(const char* s);
void OSDebugOutput(const wchar_t* s);

bool OSInit();
void OSDestroy();
void* OSGetWindowHandle(SDL_Window* window);


struct sg_environment;
struct sg_swapchain;
bool OSRenderInit(const SysRenderInitDesc* desc);
void OSRenderDestroy();
void OSRenderPresent();
void OSGetRenderEnvironment(sg_environment* env);
void OSGetRenderSwapchain(sg_swapchain* sc);


bool OSHasAdminPrivledge();
bool OSGetNetworkAdapters(std::vector<SysNetworkAdapterInfo>& out_adapters);
bool OSSetNetAdapterIP(const std::string& adapter_guid, const SysNetAdapterConfig& adapter, const SysNetAdapterConfig& src_adapter);
bool OSSetNetAdapterDNS(const std::string& adapter_guid, const SysNetAdapterConfig& adapter, const SysNetAdapterConfig& src_adapter);

bool OSIsConsoleAttached();
bool OSIsDebuggerAttached();
void OSHideConsole();
void OSShowConsole();
bool OSIsConsoleVisible();

void OSShowErrorWindow(const std::string& title, const std::string& text);
void OSFlashWindow(SDL_Window* window);
void OSScanDirectoryForFileNames(const Path& dir, ScannedFiles& out, ScanDirectoryFlags flags);
bool OSGetDirectoryFromUser(const Path& currentDir, std::wstring& dir);

void OSConvertMultibyteToWideChar(std::wstring& out, const std::string& in, StringEncoding encoding = StringEncoding_UTF8);
void OSConvertWideCharToMultiByte(std::string& out, const std::wstring& in, StringEncoding encoding = StringEncoding_UTF8);
void OSExpandEnvironemntVariable(std::wstring& out, const std::wstring& in);
void* OSGetDataFromResource(i32* out_size, const i32 resource_id);

void* OSReserveMemory(u64 bytes);
void OSCommitMemory(void* p, u64 bytes);
bool OSFreeMemory(void* p, u64 bytes);

Guid OSNewGuid();