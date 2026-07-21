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
i32 OSRunShellProcess(const wchar_t* path, const wchar_t* args, std::string* output = nullptr, Mutex* output_lock = nullptr, RunProcessFlags flags = RunProcess_None);
i32 OSRunProcess(const std::wstring& path, const std::wstring& args, AsyncData<std::string>* output = nullptr, AsyncData<Path>* output_file = nullptr, RunProcessFlags flags = RunProcess_None);

bool OSInit(SDL_Window* window);
void OSDestroy(SDL_Window* window);
void* OSGetWindowHandle(SDL_Window* window);

bool OSHasAdminPrivledge();
bool OSGetNetworkAdapters(std::vector<SysNetworkAdapterInfo>& out_adapters);
bool OSSetNetAdapterIP(const std::string& adapter_guid, const SysNetAdapterConfig& adapter, const SysNetAdapterConfig& src_adapter);
bool OSSetNetAdapterDNS(const std::string& adapter_guid, const SysNetAdapterConfig& adapter, const SysNetAdapterConfig& src_adapter);

bool OSIsConsoleAttached();
bool OSIsDebuggerAttached();
void OSHideConsole();
void OSShowConsole();
bool OSIsConsoleVisible();

void OSShowErrorWindow(const std::wstring& title, const std::wstring& text);
void OSFlashWindow(SDL_Window* window);
void OSScanDirectoryForFileNames(const std::wstring& dir, ScannedFiles& out, ScanDirectoryFlags flags);
bool OSGetDirectoryFromUser(const std::wstring& currentDir, std::wstring& dir);

void OSConvertMultibyteToWideChar(std::wstring& out, const std::string& in);
void OSConvertWideCharToMultiByte(std::string& out, const std::wstring& in);
void OSExpandEnvironemntVariable(std::wstring& out, const std::wstring& in);
void* OSGetDataFromResource(i32* out_size, const i32 resource_id);

Guid OSNewGuid();