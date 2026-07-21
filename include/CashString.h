#pragma once

#include "CashMath.h"
#include "CashDebug.h"
#include "CashArrayView.h"

#include <string>
#include <filesystem>
#include <fstream>
#include <cwctype>

namespace fs = std::filesystem;
using Path = fs::path;

enum StringCase {
    StringCase_Sensitive,
    StringCase_Insensitive,
    StringCase_Count,
};
ENUMOPS(StringCase);

template <u64 sizeT>
struct InlineString
{
    u64 max_len = sizeT;
    char s[sizeT] = {};

    template <typename T = char>
    ArrayView<T> ToArrayView() { return CreateArrayView(s, max_len); };
    StringView   ToStringView()  { return ToArrayView(); };
};

std::vector<i32> TextToIntArray(const char* text, const char lineEnd);
std::vector<i32> TextToIntArray(const char* text);
std::vector<i32> FileToIntArray(const char* fileName, const char lineEnd);
std::vector<i32> FileToIntArray(const char* fileName);
bool TextDetection(const std::string& string, const std::string& check);
void TextRemoval(std::string& string, const std::string& removalRef);
void TextRemoveAfter(std::string& string, const std::string& removalRef);
void TextAddition(std::string& string, const std::string& additionalText, const std::string& after);
std::vector<std::string> TextToStringArray(const char* text);
std::vector<std::string> TextToStringArray(const char* text, const char* lineEnd);
std::vector<std::string> TextCsvToStringArray(const char* text);
std::vector<std::string> FileToStringArray(const char* fileName);
std::vector<std::string> FileToStringArray(const char* fileName, const char* lineEnd);
i32 NumberLengthInString(const std::string& string, i32& i);
std::string ToString(const char* fmt, ...);
std::wstring ToString(const wchar_t* fmt, ...);
void ToLower(std::wstring& s);
void ToLower(std::string& s);
i32 StringToInt(const std::string& string, i32 i, i32 length);
i32 StringToInt(const std::string& string, i32 i);
void StringGetReadableByteSize(std::string& out, const u64 v);
bool StringRemoveLeading (std::string& string, const char remove);
bool StringRemoveTrailing(std::string& string, const char remove);
bool ContainsString(const std::wstring& source, const std::wstring& find, StringCase case_insensitive);
bool StringCompare(StringCase case_sensitivity, const std::string& a, const std::string& b);
bool StringCompare(StringCase case_sensitivity, const char* a, const char* b);
bool CopyFile(const Path& source, const Path& dest);
bool CopyFileRelative(const Path& source, const Path& dest, const Path& relative);
bool CopyFolderRelative(const Path& source, const Path& dest, const Path& relative);
void CopyString(char** dest, const char* source, const u64 max_length);
template <const u64 len>
void CopyString(char (*dest)[len], const char* source, const u64 max_length)
{
    ASSERT(len == max_length);
    memmove(*dest, source, Min<size_t>(max_length, strnlen_s(source, max_length)));
}
template<u64 SizeDest, u64 SizeSource>
void CopyString(InlineString<SizeDest>& dest, const InlineString<SizeSource>& source)
{
    memmove(dest.s, source.s, Min<size_t>(dest.max_len, source.max_len));
}
template<u64 SizeDest>
void CopyString(InlineString<SizeDest>& dest, const std::string& source)
{
    memmove(dest.s, source.c_str(), Min<size_t>(dest.max_len, source.size()));
}

//File path helpers
std::wstring PathGetFilename(const std::wstring& path);
std::wstring PathGetFilenameWithExtension(const std::wstring& path);
std::wstring PathGetExtension(const std::wstring& path);
void PathCleanSlashs(std::wstring& s);
void PathRemoveExtension(std::wstring& path);
std::wstring PathAddEndSlash(const std::wstring& s);
std::wstring PathConcat(const std::wstring& a, const std::wstring& b);
std::string PathConcat(const std::string& a, const std::string& b);
void CreateParentDirectories(const Path& path);

u64 FileReadAll(std::string& out, const Path& filepath);
u64 FileReadAll(std::wstring& out, const Path& filepath);

void TuiProgressBar(u64 count, u64 max);