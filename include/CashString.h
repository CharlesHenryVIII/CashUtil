#pragma once

#include "CashMath.h"
#include "CashDebug.h"
#include "CashArrayView.h"
#include "CashMemoryArena.h"
#include "CashStaticArray.h"

//#include <string>
#include <charconv>
#include <filesystem>
#include <cwctype>

namespace fs = std::filesystem;
using Path = fs::path;

enum StringCase {
    StringCase_Sensitive,
    StringCase_Insensitive,
    StringCase_Count,
};
ENUMOPS(StringCase);

struct String {
    u64 len = 0;
    char* s = nullptr;
};

std::vector<i32> TextToIntArray(const char* text, const char lineEnd);
std::vector<i32> TextToIntArray(const char* text);
std::vector<i32> FileToIntArray(const char* fileName, const char lineEnd);
std::vector<i32> FileToIntArray(const char* fileName);
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
std::string ToString(const Path& path);
void ToLower(std::string& s);
void ToLower(std::wstring& s);
[[nodiscard]] i32 StringToInt(const std::string& string, i32 i, i32 length);
[[nodiscard]] i32 StringToInt(const std::string& string, i32 i);
void StringGetReadableByteSize(std::string& out, const u64 v);

bool StringRemoveLeading (std::string& string, const char remove);
bool StringRemoveTrailing(std::string& string, const char remove);
[[nodiscard]] const char* StringRemoveLeading (Arena* arena, const char* s, const char remove);
[[nodiscard]] const char* StringRemoveTrailing(Arena* arena, const char* s, const char remove);

[[nodiscard]] const char* StringEatWhitespace(const char* input);
[[nodiscard]] const char* StringEatWord(const char* word);
[[nodiscard]] const char* StringEatWordAndWhitespace(const char* input);
[[nodiscard]] bool StringGetToken(char* buf, size_t size, const char** input);
template <size_t size>
[[nodiscard]] bool StringGetToken(char(&buf)[size], const char** input) { return StringGetToken(buf, size, input); }
[[nodiscard]] constexpr bool StringIsWhitespace(char c) { return c == ' '; }

[[nodiscard]] bool StringFindFirst(const std::string& source,  const std::string& find,    StringCase case_insensitive);
[[nodiscard]] bool StringFindFirst(const std::wstring& source, const std::wstring& find,   StringCase case_insensitive);

[[nodiscard]] const char* StringFindFirst(const char* source, const char  find);
[[nodiscard]] const char* StringFindFirst(const char* source, const char* find);
[[nodiscard]]       char* StringFindFirst(char* source, const char find);
[[nodiscard]]       char* StringFindFirst(char* source, const char* find);

[[nodiscard]] const char* StringFindLast(const char* source, const char  find);
[[nodiscard]] const char* StringFindLast(const char* source, const char* find);
[[nodiscard]]       char* StringFindLast(char* source, const char find);
[[nodiscard]]       char* StringFindLast(char* source, const char* find);


[[nodiscard]] bool StringCompare(StringCase case_sensitivity, const char* a,          const char* b,          const i32 num_chars );
[[nodiscard]] bool StringCompare(StringCase case_sensitivity, const std::string& a,   const std::string& b,   const i32 num_chars );
[[nodiscard]] bool StringCompare(StringCase case_sensitivity, const char* a,          const char* b                               );
[[nodiscard]] bool StringCompare(StringCase case_sensitivity, const std::string& a,   const std::string& b                        );

u64 StringCopy(char* dest, const char* source, const u64 buffer_length);
template <size_t buf_size>
size_t StringCopy(char(&dest)[buf_size], const char* source) { return StringCopy(dest, source, buf_size); }

template<u64 SizeDest, u64 SizeSource>
void StringCopy(InlineString<SizeDest>& dest, const InlineString<SizeSource>& source)
{
    memmove(dest.s, source.s, Min<size_t>(dest.max_len, source.max_len));
}
template<u64 SizeDest>
void StringCopy(InlineString<SizeDest>& dest, const std::string& source)
{
    memmove(dest.s, source.c_str(), Min<size_t>(dest.max_len, source.size()));
}

//File path helpers
bool CopyFile(const Path& source, const Path& dest);
bool CopyFileRelative(const Path& source, const Path& dest, const Path& relative);
bool CopyFolderRelative(const Path& source, const Path& dest, const Path& relative);





//========================
//    Path std::string
//========================

[[nodiscard]] std::string PathGetFilename               (const std::string& s);
[[nodiscard]] std::string PathGetFilenameWithExtension  (const std::string& s);
[[nodiscard]] std::string PathGetExtension              (const std::string& s);
[[nodiscard]] std::string PathCleanSlashs               (const std::string& s);
[[nodiscard]] std::string PathRemoveExtension           (const std::string& s);
[[nodiscard]] std::string PathAddEndSlash               (const std::string& s);
[[nodiscard]] std::string PathConcat                    (const std::string& s1, const std::string& s2);





//========================
//   Path Arena + char*
//========================

[[nodiscard]] const char* PathGetFilename               (Arena* arena, const char* s);
[[nodiscard]] const char* PathGetFilenameWithExtension  (Arena* arena, const char* s);
[[nodiscard]] const char* PathGetExtension              (Arena* arena, const char* s);
[[nodiscard]] const char* PathCleanSlashes              (Arena* arena, const char* s);
[[nodiscard]] const char* PathRemoveExtension           (Arena* arena, const char* s);
[[nodiscard]] const char* PathAddEndSlash               (Arena* arena, const char* s);
[[nodiscard]] const char* PathConcat                    (Arena* arena, const char* a, const char* b);
[[nodiscard]]       bool  PathHasEndSlash               (const char* s);
[[nodiscard]]       bool  PathIsSlash                   (const char c);

void CreateParentDirectories(const Path& path);

void TuiProgressBar(u64 count, u64 max);
