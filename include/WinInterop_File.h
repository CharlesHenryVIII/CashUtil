#pragma once
#include "Math.h"

#include <string>
#include <vector>

enum FileMode {
    FileMode_Read,  //Read:   Open file for read access.
    FileMode_Write, //Write:  Open and empty file for output.
    //Append,//Append: Open file for output.
};
ENUMOPS(FileMode);

struct File {

    bool    m_handleIsValid     = false;
    bool    m_textIsValid       = false;
    bool    m_timeIsValid       = false;
    bool    m_binaryDataIsValid = false;
    u64     m_time              = {};
    std::wstring    m_filename;
    std::string     m_dataString;
    std::vector<u8> m_dataBinary;

    File();
    File(char const* filename,          FileMode fileMode, bool createIfNotFound);
    File(const std::string& filename,   FileMode fileMode, bool createIfNotFound);
    File(wchar_t const* filename,       FileMode fileMode, bool createIfNotFound);
    File(const std::wstring& filename,  FileMode fileMode, bool createIfNotFound);
    ~File();

    bool Write(const std::string& text);
    bool Write(void* data, size_t sizeInBytes);
    bool Write(const void* data, size_t sizeInBytes);
    void GetData();
    void GetText();
    void GetTime();
    bool Delete();

private:

    void* m_handle;
    u32  m_accessType;
    u32  m_shareType;
    u32  m_openType;

    void GetHandle();
    void Init(const std::wstring& filename, FileMode fileMode, bool createIfNotFound);
    bool FileDestructor();
};
