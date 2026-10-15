#pragma once
#include "CashMath.h"
#include "CashArray.h"
#include "CashArrayView.h"

#include <string>
#include <vector>






//========================
// File Holding Functions
//========================

enum FileMode {
    FileMode_Invalid,
    FileMode_Read,              //Open for read
    FileMode_WriteCreate,       //Open for Write.           Create if doesn't exist
    FileMode_ReadWriteCreate,   //Open for Read and Write.  Create if doesn't exist
    FileMode_AppendCreate,      //Open for Read and Append. Create if doesn't exist
    FileMode_Count,
};
ENUMOPS(FileMode);

DATAID_TYPE(FileID);
[[nodiscard]]   FileID  FileOpen(const char* filename, FileMode mode);
                void    FileClose(FileID ID);
//
bool FileRead(FileID ID, std::vector<u8> out_data);
//
size_t FileWrite(FileID id, const void* data, const size_t bytes);
template <typename T>
inline size_t FileWrite(FileID ID, const ArrayView<T> data) { return FileWrite(ID, (void*)data.data, data.Bytes()); };
inline size_t FileWrite(FileID ID, const std::string& data) { return FileWrite(ID, (void*)data.c_str(), data.size()); };
//
[[nodiscard]] u64 FileGetTime(FileID ID);
//
bool FileDelete(FileID ID);





//========================
//   One shot Functions
//========================

//Returns true on success
bool FileRead(const char* path, std::vector<u8>& out_data);
bool FileRead(const char* path, std::string& out_data);
//Will create if the file doesn't exist
bool FileWrite(const char* path, const void* data, const size_t bytes);
inline bool FileWrite(const char* path, const std::string& data) { return FileWrite(path, data.c_str(), data.size()); };
template <typename T>
inline bool FileWrite(const char* path, const ArrayView<T> data) { return FileWrite(path, (void*)data.data, data.Bytes()); };
//Returns time since epoch or something
[[nodiscard]] u64 FileGetCreateTime(const char* path);
[[nodiscard]] u64 FileGetModifyTime(const char* path);
[[nodiscard]] u64 FileGetAccessTime(const char* path);
[[nodiscard]] u64 FileGetSize(const char* path);
//Returns true on valid delete
bool FileDelete(const char* path);
//
[[nodiscard]] bool FileExists(const char* path);
[[nodiscard]] bool FileIsDir(const char* path);
