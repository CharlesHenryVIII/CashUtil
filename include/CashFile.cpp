#include "CashFile.h"
#include "CashSystem.h"
#include "CashConsole.h"





//========================
// File Holding Functions
//========================

static const char* s_op[FileMode_Count] = { "", "rb", "wb", "w+b", "a+" };

struct File {
    FileID data_id;
    SDL_IOStream* io;
    SDL_PathInfo info = {};
    std::string path;
    FileMode mode;
};
static StaticIdArray<File, FileID, 256> s_files;

[[nodiscard]] FileID FileOpen(const char* path, FileMode mode)
{
    VALIDATE_MV(mode != FileMode_Invalid && mode != FileMode_Count, {}, LogLevel_Error, "Invalid FileMode for %s", path);

    SDL_IOStream* io = SDL_IOFromFile(path, s_op[mode]);
    VALIDATE_MV(io, {}, LogLevel_Warning, "Failed to open file: %s", path);

    File* f = s_files.CreateNew();
    f->io = io;
    f->path = path;
    f->mode = mode;
    return f->data_id;
}
void FileClose(FileID id)
{
    File* f = s_files.TryGet(id);
    VALIDATE_M(f, LogLevel_Error, "Could not retrieve File from handle: %i:%i", id.index, id.generation);
    SDL_CloseIO(f->io);
    s_files.Erase(f->data_id);
}
bool FileRead(FileID id, std::vector<u8> out_data)
{
    File* f = s_files.TryGet(id);
    VALIDATE_MV(f, false, LogLevel_Error, "Could not retrieve File from handle: %i:%i", id.index, id.generation);
    const i64 size = SDL_GetIOSize(f->io);
    VALIDATE_MV(size > 0, false, LogLevel_Warning, "Failed to get file size: %s : %s", f->path.c_str(), SDL_GetError());
    out_data.resize(size);
    const size_t read = SDL_ReadIO(f->io, out_data.data(), out_data.size());
    return read == size;
}
size_t FileWrite(FileID id, const void* data, const size_t bytes)
{
    File* f = s_files.TryGet(id);
    VALIDATE_MV(f, 0, LogLevel_Error, "Could not retrieve File from handle: %i:%i", id.index, id.generation);
    VALIDATE_MV(data, 0, LogLevel_Error, "Invalid data passed to FileWrite(): %s", f->path.c_str());
    VALIDATE_MV(bytes, 0, LogLevel_Error, "Invalid bytes passed to FileWrite(): %i : %s", bytes, f->path.c_str());
    const size_t r = SDL_WriteIO(f->io, data, bytes);
    if (r < bytes)
    {
        SDL_IOStatus s = SDL_GetIOStatus(f->io);
        LOG(LogLevel_Warning, "SDL io failure: %i", s);
    }
    return r;
}
[[nodiscard]] u64 FileGetTime(FileID id)
{
    File* f = s_files.TryGet(id);
    VALIDATE_MV(f, 0, LogLevel_Error, "Could not retrieve File from handle: %i:%i", id.index, id.generation);
    SDL_GetPathInfo(f->path.c_str(), &f->info);
    FAIL; // what even is this time.  "Time since epoch"
    return (u64)f->info.modify_time;
}
bool FileDelete(FileID id)
{
    File* f = s_files.TryGet(id);
    VALIDATE_MV(f, false, LogLevel_Error, "Could not retrieve File from handle: %i:%i", id.index, id.generation);
    return FileDelete(f->path.c_str());
}





//========================
//   One shot Functions
//========================

[[nodiscard]] bool FileRead(const char* path, std::vector<u8>& out)
{
    if (out.size())
    {
        out.clear();
    }
    size_t size = 0;
    void* data = SDL_LoadFile(path, &size);   // allocates, adds a null terminator
    if (!data)
        return false;
    out.resize(size + 1);
    memmove(out.data(), data, size + 1);
    SDL_free(data);
    return true;
}
[[nodiscard]] bool FileRead(const char* path, std::string& out)
{
    if (out.size())
    {
        out.clear();
    }
    size_t size = 0;
    void* data = SDL_LoadFile(path, &size);   // allocates, adds a null terminator
    if (!data)
        return false;
    out.resize(size + 1);
    memmove(out.data(), data, size + 1);
    SDL_free(data);
    return true;
}

bool FileWrite(const char* path, ArrayView<u8> data)
{
    return SDL_SaveFile(path, data.data, data.Bytes());
}
bool FileWrite(const char* path, const std::string& data)
{
    return SDL_SaveFile(path, data.c_str(), data.size());
}

[[nodiscard]] u64 FileGetCreateTime(const char* path)
{
    SDL_PathInfo info;
    SDL_GetPathInfo(path, &info);
    FAIL; // what even is this time.  "Time since epoch"
    return (u64)info.create_time;
}
[[nodiscard]] u64 FileGetModifyTime(const char* path)
{
    SDL_PathInfo info;
    SDL_GetPathInfo(path, &info);
    FAIL; // what even is this time.  "Time since epoch"
    return (u64)info.modify_time;
}
[[nodiscard]] u64 FileGetAccessTime(const char* path)
{
    SDL_PathInfo info;
    SDL_GetPathInfo(path, &info);
    FAIL; // what even is this time.  "Time since epoch"
    return (u64)info.access_time;
}
[[nodiscard]] u64 FileGetSize(const char* path)
{
    SDL_PathInfo info;
    SDL_GetPathInfo(path, &info);
    return info.size;
}
bool FileDelete(const char* path)
{
    return remove(path) == 0;
}

[[nodiscard]] bool FileExists(const char* path)
{
    return SDL_GetPathInfo(path, nullptr);
}

[[nodiscard]] bool FileIsDir(const char* path)
{
    SDL_PathInfo info;
    SDL_GetPathInfo(path, &info);
    return info.type == SDL_PATHTYPE_DIRECTORY;
}
