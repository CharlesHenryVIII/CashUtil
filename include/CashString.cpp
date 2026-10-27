#include "CashString.h"
#include "CashSystem.h"
//#include "CashStaticArray.h"
#include "CashLog.h"

#define STB_SPRINTF_IMPLEMENTATION
#include "stb/stb_sprintf.h"



const char* ReadEntireFileAsString(const char* fileName)
{
    FILE* file;
    if ((file = fopen(fileName, "rb")))
    {
        fseek(file, 0, SEEK_END);
        const long fileLength = ftell(file);
        fseek(file, 0, SEEK_SET);
        char* buffer = new char[fileLength + 1];
        fread(buffer, 1, fileLength, file);
        fclose(file);
        buffer[fileLength] = 0;
        return buffer;
    }
    return nullptr;
}

std::vector<i32> TextToIntArray(const char* text, const char lineEnd)
{
    std::vector<i32> workingNums;
    std::vector<i32> nums;
    workingNums.reserve(10);
    nums.reserve(16);

    for (i32 i = 0; text[i] != 0; i++)
    {

        //ASSERT((data.text[i] - '0' >= 0  && data.text[i] - '0' <= '9') || data.text[i] == lineEnd);
        if (text[i] - '0' >= 0  && text[i] - '0' <= '9')
        {
            i32 value = text[i] - '0';
            workingNums.push_back(value);
        }
        else if (text[i] == lineEnd)
        {
            i32 value = 0;
            for (size_t j = 0; j < workingNums.size(); j++)
            {
                value += (workingNums[j] * (int)pow(10, workingNums.size() - 1 - j));
            }
            ASSERT(value); //should not occurr
            if (value)
            {
                nums.push_back(value);
                workingNums.clear();
            }
        }
    }
    delete text;
    return nums;
}

std::vector<i32> TextToIntArray(const char* text)
{
    std::vector<int> nums;
    //std::string workingText = text;
    std::string_view test = text;
    i32 index = 0;

    for (i32 i = 0; i == 0 || text[i - 1] != 0; i++)
    {
        if (text[i] >= '0' && text[i] <= '9')
            index++;
        else if (index)
        {
            //nums.push_back(atoi(workingText.substr(i - index, index).c_str()));
            nums.push_back(atoi(test.substr(i - index, index).data()));
            index = 0;
        }
    }
    return nums;
}

void TextRemoval(std::string& string, const std::string& removalRef)
{
    std::string_view sv = string;
    auto begin = string.begin();
    auto end = string.begin();
    i32 i = 0;
    while (end != string.end())
    {
        if (sv.substr(i, removalRef.size()) == removalRef)
        {
            begin = end;
            for (size_t j = 0; j < removalRef.size(); j++)
                end++;
            string.erase(begin, end);
            end = string.begin();
            i = 0;
            //return;

        }
        else
        {
            end++;
            i++;
        }
    }
}

void TextRemoveAfter(std::string& string, const std::string& removalRef)
{
    std::string_view sv = string;
    auto begin = string.begin();
    auto end = string.begin();
    i32 i = 0;
    while (end != string.end())
    {
        if (sv.substr(i, removalRef.size()) == removalRef)
        {
            begin = end;
            string.erase(begin, string.end());
            return;
        }
        else
        {
            end++;
            i++;
        }
    }
}

void TextAddition(std::string& string, const std::string& additionalText, const std::string& after)
{
    std::string_view sv = string;
    for (size_t i = 0; i < string.size() - after.size(); i++)
    {
        if (sv.substr(i, (after.size())) == after && !(sv.substr((size_t)i, (after.size() + additionalText.size())) == (after + additionalText)))
        {
            string.insert(i + after.size(), additionalText);
        }
    }
}

std::vector<std::string> TextToStringArray(const char* text)
{
    std::string_view sv = text;
    std::vector<std::string> result;

    i32 tokenLength = 0;
    for (i32 i = 0; text[i - 1] != 0; i++)
    {
        if (text[i] >= ' ' && text[i] <= '~')
        {
            tokenLength++;
        }
        else
        {
            if (tokenLength)
            {

                std::string token;
                token = sv.substr(i - tokenLength, tokenLength);
                result.push_back(token);
                tokenLength = 0;
            }
        }
    }
    ASSERT(tokenLength == 0);
    return result;
}

std::vector<std::string> TextToStringArray(const char* text, const char* lineEnd)
{
    ASSERT(lineEnd);
    ASSERT(text);

    std::string_view sv = text;
    std::vector<std::string> result;

    i32 tokenLength = 0;
    i32 i = 0;
    for (; text[i] != 0; i++)
    {
        bool isLineEnd = false;
        if (text[i] == lineEnd[0] || text[i + 1] == 0)
        {
            i32 incrimenter = 0;
            while (text[i + incrimenter] == lineEnd[incrimenter])
            {

                if (lineEnd[incrimenter + 1] == 0)
                {
                    isLineEnd = true;
                    break;
                }
                incrimenter++;
            }
            bool next_is_end = text[i + 1] == 0;
            if (tokenLength && (isLineEnd || next_is_end))
            {

                std::string token;
                token = sv.substr(i - tokenLength, tokenLength + (i32)next_is_end);
                result.push_back(token);
                tokenLength = 0;
                i += incrimenter;
                isLineEnd = true;
            }
        }
        if (!isLineEnd && *text != *lineEnd)
            tokenLength++;

    }
    if (tokenLength)
    {
        std::string token(sv.substr(sv.size() - tokenLength, tokenLength));
        result.push_back(token);
    }
    return result;
}

std::vector<std::string> TextCsvToStringArray(const char* text)
{
    VALIDATE_V(text, {});

    std::string_view sv = text;
    std::vector<std::string> result;

    i32 prev = 0;
    bool in_quote = false;
    for (i32 i = 0; text[i] != 0; i++)
    {
        if (text[i] == '\"')
        {
            if (in_quote)
            {
                if (text[i + 1] == ',' ||
                    text[i + 1] == '\r' ||
                    text[i + 1] == '\n')
                {
                    result.push_back(std::string(sv.substr(prev, i - prev)));
                    prev = i;
                    in_quote = false;
                    continue;
                }
            }
            else
            {
                prev = i + 1;
                in_quote = true;
            }
        }
    }
    return result;
}

std::vector<std::string> FileToStringArray(const char* fileName)
{
    const char* str = ReadEntireFileAsString(fileName);
    if (str == nullptr)
        return {};
    else
        return TextToStringArray(str);
}

std::vector<std::string> FileToStringArray(const char* fileName, const char* lineEnd)
{
    const char* str = ReadEntireFileAsString(fileName);
    if (str == nullptr)
        return {};
    else
        return TextToStringArray(str, lineEnd);
}

std::vector<i32> FileToIntArray(const char* fileName, const char lineEnd)
{
    const char* str = ReadEntireFileAsString(fileName);
    if (str == nullptr)
        return {};
    else
        return TextToIntArray(str, lineEnd);
}

std::vector<i32> FileToIntArray(const char* fileName)
{
    const char* str = ReadEntireFileAsString(fileName);
    if (str == nullptr)
        return {};
    else
        return TextToIntArray(str);
}

i32 NumberLengthInString(const std::string& string, i32& i)
{

    i32 j = 0;
    while (string[i + j] >= '0' && string[i + j] <= '9')
        j++;
    return j;
}

std::string ToString(const char* fmt, ...)
{
    va_list args;
    va_start(args, fmt);
    char buffer[4096];
    const i32 char_written = Sprintf(buffer, arrsize(buffer), fmt, args);
    ASSERT(char_written < sizeof(buffer));
    va_end(args);
    return buffer;
}

std::wstring ToString(const wchar_t* fmt, ...)
{
    va_list args;
    va_start(args, fmt);
    wchar_t buffer[4096];
    const i32 char_written = vswprintf(buffer, arrsize(buffer), fmt, args);
    ASSERT(char_written < sizeof(buffer));
    va_end(args);
    return buffer;
}
std::string ToString(const Path& path)
{
    std::string r;
    SysConvertWideCharToMultiByte(r, path.wstring());
    return r;
}

void ToLower(std::wstring& s)
{
    std::transform(s.begin(), s.end(), s.begin(),
        [](std::wint_t c) { return std::towlower(c); }
    );
    FAIL; //untested
}

void ToLower(std::string& s)
{
    std::transform(s.begin(), s.end(), s.begin(),
        [](unsigned char c) { return std::tolower(c); }
    );
}



i32 StringToInt(const std::string& string, i32 i, i32 length)
{
    return atoi(string.substr(i, length).c_str());
}

i32 StringToInt(const std::string& string, i32 i)
{
    return StringToInt(string, i, NumberLengthInString(string, i));
}

void StringGetReadableByteSize(std::string& out, const u64 v)
{
    if (v > Terabytes(1LLU))
    {
        double tb = ToTerabytesFloat(v);
        out = ToString("%.2fTB", tb);
    }
    else if (v > Gigabytes(1))
    {
        double tb = ToGigabytesFloat(v);
        out = ToString("%.2fGB", tb);
    }
    else if (v > Megabytes(1))
    {
        double tb = ToMegabytesFloat(v);
        out = ToString("%.2fMB", tb);
    }
    else if (v > Kilobytes(1))
    {
        double tb = ToKilobytesFloat(v);
        out = ToString("%.2fKB", tb);
    }
}


bool StringRemoveLeading(std::string& s, const char r)
{
    i32 i = 0;
    for (; i < s.size() - 1; ++i)
    {
        if (s[i] != r)
            break;
    }
    if (i != 0)
    {
        s = s.substr(i, s.size() - 1);
        return true;
    }
    return false;
}

bool StringRemoveTrailing(std::string& s, const char r)
{
    i32 i = (i32)(s.size() - 1);
    for (; i >= 0; --i)
    {
        if (s[i] != r)
            break;
    }
    i++;
    if (i != s.size())
    {
        s = s.substr(0, i);
        return true;
    }
    return false;
}

const char* StringRemoveLeading(Arena* arena, const char* s, const char r)
{
    size_t i = 0;
    const size_t len = strlen(s);
    for (; i < len - 1; ++i)
    {
        if (s[i] != r)
            break;
    }
    FAIL; //untested
    if (i != 0)
    {
        return (const char*)ArenaPush(arena, s + i);
    }
    return (const char*)ArenaPush(arena, s);
}

const char* StringRemoveTrailing(Arena* arena, const char* s, const char r)
{
    const size_t len = strlen(s);
    size_t i = len - 1;
    for (; i >= 0; --i)
    {
        if (s[i] != r)
            break;
    }
    i++;
    FAIL; //untested
    if (i != len)
    {
        return (const char*)ArenaPush(arena, i);
    }
    return (const char*)ArenaPush(arena, s);
}

const char* StringEatWhitespace(const char* input)
{
    while (*input && StringIsWhitespace(*input))
        input++;
    return input;
}
const char* StringEatWord(const char* word)
{
    while (*word && !StringIsWhitespace(*word))
        word++;
    return word;
}
const char* StringEatWordAndWhitespace(const char* input)
{
    const char* word_end = StringEatWord(input);
    const char* whitespace_end = StringEatWhitespace(word_end);
    return whitespace_end;
}

bool StringGetToken(char* buf, size_t size, const char** input)
{
    if (size == 0)
        return 0;
    else if (!input)
    {
        buf[0] = 0;
        return 0;
    }

    *input = StringEatWhitespace(*input);
    const char* word_end = StringEatWord(*input);
    size_t len = size_t(word_end - *input);
    len = Min(size - 1, len);
    memmove(buf, *input, len);
    buf[len] = 0;
    *input = word_end;
    return int(len) > 0;
}

#if defined(_WIN32) || defined(_MSC_VER)
    #define SYS_STRICMP _stricmp
    #define SYS_STRNICMP _strnicmp
#else
    #include <strings.h>
    #define SYS_STRICMP stricmp
    #define SYS_STRICMP strcasecmp
#endif

bool StringCompare(StringCase case_sensitivity, const std::string& a, const std::string& b, const i32 num_chars)
{
    if (a.empty() || a.empty())
        return false;
    return StringCompare(case_sensitivity, a.c_str(), b.c_str(), num_chars);
}
bool StringCompare(StringCase case_sensitivity, const char* a, const char* b, const i32 num_chars)
{
    switch (case_sensitivity)
    {
    case StringCase_Sensitive:      return strncmp      (a, b, num_chars) == 0;
    case StringCase_Insensitive:    return SYS_STRNICMP (a, b, num_chars) == 0;
    case StringCase_Count: [[fallthrough]];
    default: FAIL; break;
    }
    return false;
}

bool StringCompare(StringCase case_sensitivity, const std::string& a, const std::string& b)
{
    if (a.size() != b.size())
        return false;
    return StringCompare(case_sensitivity, a.c_str(), b.c_str());
}
bool StringCompare(StringCase case_sensitivity, const char* a, const char* b)
{
    switch (case_sensitivity)
    {
    case StringCase_Sensitive:      return strcmp       (a, b) == 0;
    case StringCase_Insensitive:    return SYS_STRICMP  (a, b) == 0;
    case StringCase_Count: [[fallthrough]];
    default: FAIL; break;
    }
    return false;
}


bool StringFindFirst(const std::string& source, const std::string& find, StringCase case_insensitive)
{
    std::string s = source;
    std::string f = find;
    if (case_insensitive == StringCase_Insensitive)
    {
        ToLower(s);
        ToLower(f);
    }
    if (s.find(f.c_str()))
        return true;
    return false;
}

bool StringFindFirst(const std::wstring& source, const std::wstring& find, StringCase case_insensitive)
{
    std::string s;
    SysConvertWideCharToMultiByte(s, source);
    std::string f;
    SysConvertWideCharToMultiByte(f, find);
    return StringFindFirst(s, f, case_insensitive);
}

const char* StringFindFirst(const char* source, const char find)
{
    return strchr(source, find);
}
const char* StringFindFirst(const char* source, const char* find)
{
    return strstr(source, find);
}
char* StringFindFirst(char* source, const char find)
{
    return strchr(source, find);
}
char* StringFindFirst(char* source, const char* find)
{
    return strstr(source, find);
}

const char* StringFindLast(const char* source, const char find)
{
    return strrchr(source, find);
}
const char* StringFindLast(const char* source, const char* find)
{
    if (!source || !find)
        return nullptr;
    const size_t slen = strlen(source);
    const size_t flen = strlen(find);
    if (flen == 0 || slen == 0)
        return nullptr;
    if (flen > slen)
        return nullptr;

    for (const char* p = source + (slen - flen); ; --p)
    {
        if (memcmp(p, find, flen) == 0)
            return p;
        if (p == source)
            break;
    }
    return nullptr;
}

char* StringFindLast(char* source, const char find)
{
    return strrchr(source, find);
}
char* StringFindLast(char* source, const char* find)
{
    if (!source || !find)
        return nullptr;
    const size_t slen = strlen(source);
    const size_t flen = strlen(find);
    if (flen == 0 || slen == 0)
        return nullptr;
    if (flen > slen)
        return nullptr;

    for (char* p = source + (slen - flen); ; --p)
    {
        if (memcmp(p, find, flen) == 0)
            return p;
        if (p == source)
            break;
    }
    return nullptr;
}

bool CopyFile(const Path& source, const Path& dest)
{
    if (source.empty() || dest.empty())
    {
        DebugPrint("Failed to copy file EMPTY: src: \"%s\" dest: \"%s\"", ToString(source).c_str(), ToString(dest).c_str());
        FAIL;
        return false;
    }
    std::error_code ec;
    fs::create_directories(dest.parent_path(), ec);
    if (ec)
    {
        DebugPrint("Failed to create directories for dest: \"%s\"", ToString(dest).c_str());
        DebugPrint("create_directories Failure: \"%d\", \"%s\"", ec.value(), ec.message().c_str());
        FAIL;
        return false;
    }

    bool result = std::filesystem::copy_file(source, dest, std::filesystem::copy_options::overwrite_existing, ec);
    if (ec)
    {
        DebugPrint("Failed to copy file src: \"%s\" dest: \"%s\"", ToString(source).c_str(), ToString(dest).c_str());
        DebugPrint("copy_file Failure: \"%d\", \"%s\"", ec.value(), ec.message().c_str());
        FAIL;
        return false;
    }
    return result;
}

bool CopyFileRelative(const Path& source, const Path& dest, const Path& relative)
{
    if (source.empty() || dest.empty() || relative.empty())
    {
        DebugPrint("Failed to copy file EMPTY: src: \"%s\" dest: \"%s\" relative: \"%s\"", ToString(source).c_str(), ToString(dest).c_str(), ToString(relative).c_str());
        FAIL;
        return false;
    }
    Path full_source = source / relative;
    Path full_dest = dest / relative;

    std::error_code ec;
    fs::create_directories(full_dest.parent_path(), ec);
    if (ec)
    {
        DebugPrint("Failed to create directories for dest: \"%s\"", ToString(full_dest).c_str());
        DebugPrint("create_directories Failure: \"%d\", \"%s\"", ec.value(), ec.message().c_str());
        FAIL;
        return false;
    }

    bool result = fs::copy_file(full_source, full_dest, fs::copy_options::overwrite_existing, ec);
    if (ec)
    {
        DebugPrint("Failed to copy file src: \"%s\" dest: \"%s\"", ToString(full_source).c_str(), ToString(full_dest).c_str());
        DebugPrint("copy_file Failure: \"%d\", \"%s\"", ec.value(), ec.message().c_str());
        FAIL;
        return false;
    }
    return result;
}

bool CopyFolderRelative(const Path& source, const Path& dest, const Path& relative)
{
    if (source.empty() || dest.empty() || relative.empty())
    {
        DebugPrint("Failed to copy folder EMPTY: src: \"%s\" dest: \"%s\" relative: \"%s\"", ToString(source).c_str(), ToString(dest).c_str(), ToString(relative).c_str());
        FAIL;
        return false;
    }
    Path full_source = source / relative;
    Path full_dest = dest / relative;

    std::error_code ec;
    fs::create_directories(full_dest.parent_path(), ec);
    if (ec)
    {
        DebugPrint("Failed to create directories for dest: \"%s\"", ToString(full_dest.parent_path()).c_str());
        DebugPrint("create_directories Failure: \"%d\", \"%s\"", ec.value(), ec.message().c_str());
        FAIL;
        return false;
    }

    fs::copy(full_source, full_dest, fs::copy_options::recursive | fs::copy_options::overwrite_existing, ec);
    if (ec)
    {
        DebugPrint("Failed to copy folder src: \"%s\" dest: \"%s\" ", ToString(full_source).c_str(), ToString(full_dest).c_str());
        DebugPrint("copy_file Failure: \"%d\", \"%s\"", ec.value(), ec.message().c_str());
        FAIL;
        return false;
    }
    return true;
}

u64 StringCopy(char* dest, const char* source, const u64 buffer_length)
{
#if 0
    VALIDATE_V(dest && source && buffer_length, 0);
#else
    ASSERT(dest && buffer_length);
    if (!source || !source[0])
    {
        dest[0] = 0;
        return 0;
    }
#endif
    const size_t d_len = SYS_STRNLEN(dest,   buffer_length);
    const size_t s_len = SYS_STRNLEN(source, buffer_length);
    const size_t min_len = Min(s_len, d_len);
    size_t len = min_len;
    if (min_len >= buffer_length)
    {
        len = buffer_length - 1;  //NOTE(CSH): This is to preserve null termination
    }
    memmove(dest, source, len);
    dest[len] = 0;
    return len;
}

std::string PathRemoveExtension(const std::string& path)
{
    size_t end = path.find_last_of(".");
    if (end == std::string::npos)
    {
        LOG(LogLevel_Warning, "GetFilename() failed to find \".\" for %s", path.c_str());
        return path;
    }

    std::string r = path;
    r.erase(end, path.size() - end);
    return r;
}

std::string PathGetFilename(const std::string& path)
{
    size_t start = path.find_last_of("\\");
    if (start == std::string::npos)
        start = path.find_last_of("/");
    if (start == std::string::npos)
    {
        DebugPrint("GetFilename() failed to find \"/\" or \"\\\" for %s", path.c_str());
        return path;
    }

    size_t end = path.find_last_of(".");
    if (end == std::string::npos)
    {
        DebugPrint("GetFilename() failed to find \".\" for %s", path.c_str());
        return "";
    }

    if (start++ >= path.size() || start >= end)
    {
        DebugPrint("GetFilename() failed to get valid range for start(%i) and end(%i) for %s", start, end, path.c_str());
        return "";
    }

    std::string_view view = path;
    std::string result;
    result = view.substr(start, end - start);
    return result;
}

std::string PathGetFilenameWithExtension(const std::string& path)
{
    std::string result = "";
    size_t start = path.find_last_of("\\");
    if (start == std::string::npos)
        start = path.find_last_of("/");
    if (start == std::string::npos)
    {
        DebugPrint("Failed to GetFilename() for %s", path.c_str());
        return result;
    }

    if (start++ >= path.size())
    {
        DebugPrint("Failed to GetFilename() for %s", path.c_str());
        return result;
    }

    std::string_view view = path;
    result = view.substr(start);
    return result;
}

std::string PathGetExtension(const std::string& path)
{
    std::string result = "";
    const size_t start = path.find_last_of('.');
    if (start == std::string::npos)
    {
        DebugPrint("Failed to GetFilenameExtension() for %s", path.c_str());
        return result;
    }

    std::string_view view = path;
    result = view.substr(start);
    return result;
}

std::string PathCleanSlashs(const std::string& s)
{
    size_t pos = s.find('\\');
    std::string r = s;
    while (pos != std::string::npos)
    {
        r.replace(pos, 1, "/", 1);
        pos = s.find('\\');
    }
    return r;
}

std::string PathAddEndSlash(const std::string& s)
{
    if (s.back() != '/')
        return s + "/";
    return s;
}

std::string PathConcat(const std::string& a, const std::string& b)
{
    if (a.back() == '/' || a.back() == '\\')
    {
        return a + b;
    }
    return a + "/" + b;
}





//========================
//   Path Arena + char*
//========================

const char* PathGetFilename(Arena* arena, const char* s)
{
    const char* start = StringFindLast(s, '\\');
    if (!start)
        start = StringFindLast(s, '/');
    if (!start)
    {
        LOG(LogLevel_Info, "PathFilename() failed to find \"/\" or \"\\\" for %s", s);
        const char* end = StringFindLast(s, '.');
        if (end)
        {
            LOG(LogLevel_Warning, "PathFilename() failed to find \".\" for %s", s);
            const size_t len = end - s;
            char* r = (char*)ArenaPush(arena, len + 1);
            memmove(r, s, len);
            return r;
        }
        return ArenaPush(arena, s);
    }
    start++;

    const char* end = StringFindLast(s, '.');
    if (!end)
    {
        LOG(LogLevel_Warning, "PathFilename() failed to find \".\" for %s", s);
        return ArenaPush(arena, start);
    }

    const size_t len = end - start;
    if (start + 1 < s || start >= end)
    {
        LOG(LogLevel_Warning, "PathFilename() failed to get valid range for start(\"%s\") and end(\"%s\") for \"%s\"", start, end, s);
        return ArenaPush(arena, start);
    }

    char* r = (char*)ArenaPush(arena, len + 1);
    memmove(r, start, len);
    r[len] = '\0';
    return r;
}

const char* PathGetFilenameWithExtension(Arena* arena, const char* s)
{
    const char* start = StringFindLast(s, '\\');
    if (!start)
        start = StringFindLast(s, '/');
    if (!start)
    {
        LOG(LogLevel_Info, "PathFilename() failed to find \"/\" or \"\\\" for %s", s);
        return ArenaPush(arena, s);
    }
    start++;

    const char* r = (const char*)ArenaPush(arena, start);
    return r;
}

const char* PathGetExtension(Arena* arena, const char* s)
{
    const char* start = StringFindLast(s, '.');
    if (!start)
    {
        LOG(LogLevel_Warning, "PathFilename() failed to find \".\" for %s", s);
        return nullptr;
    }

    const char* r = (const char*)ArenaPush(arena, start);
    return r;
}

const char* PathCleanSlashes(Arena* arena, const char* s)
{
    char* r = (char*)ArenaPush(arena, s);
    while (char* c = StringFindFirst(r, '\\'))
    {
        *c = '/';
    }
    return r;
}

const char* PathRemoveExtension(Arena* arena, const char* s)
{
    const char* d = StringFindFirst(s, '.');
    if (!d)
        return (const char*)ArenaPush(arena, s);

    const size_t len = d - s;
    char* r = (char*)ArenaPush(arena, len + 1);
    memmove(r, s, len);
    r[len] = '\0';
    return r;
}

const char* PathAddEndSlash(Arena* arena, const char* s)
{
    size_t len = strlen(s);
    if (PathHasEndSlash(s))
        len++;

    char* r = (char*)ArenaPush(arena, len + 1);
    memmove(r, s, len);
    r[len - 1] = '/';
    r[len] = '\0';
    return r;
}

bool PathIsSlash(const char c)
{
    return (c == '\\' || c == '/');
}

bool PathHasEndSlash(const char* s)
{
    size_t len = strlen(s);
    const char c = s[len - 1];
    return PathIsSlash(c);
}

const char* PathConcat(Arena* arena, const char* a, const char* b)
{
    size_t alen = strlen(a);
    size_t blen = strlen(b);
    size_t bstart = 0;
    if (PathIsSlash(a[alen - 1]))
        alen--;
    if (PathIsSlash(b[0]))
        bstart++, blen--;
    const size_t len = alen + blen + 1;
    char* r = (char*)ArenaPush(arena, len + 1);
    //
    memmove(r, a, alen);
    r[alen] = '/';
    memmove(r + alen + 1, b + bstart, blen);
    r[len] = '\0';
    //
    return r;
}

void CreateParentDirectories(const Path& path)
{
    if (!path.empty() && path.has_parent_path())
    {
        std::error_code ec;
        fs::create_directories(path.parent_path(), ec);
        if (ec)
        {
            DebugPrint("Failed to create directories for \"%s\"", ToString(path).c_str());
            DebugPrint("create_directories Failure: \"%d\", \"%s\"", ec.value(), ec.message().c_str());
            FAIL;
        }
    }
}

void TuiProgressBar(u64 count, u64 max)
{
    const i32 bar_width = 50;

    float progress = (float)count / max;
    i32 bar_length = i32(progress * (float)bar_width);

    printf("\rProgress: [");
    for (i32 i = 0; i < bar_length; ++i)
    {
        printf("#");
    }
    for (i32 i = bar_length; i < bar_width; ++i)
    {
        printf(" ");
    }
    printf("] %.2f%%", progress * 100);

    fflush(stdout);
}

i32 Sprintf(char* buffer, i32 count, char const* fmt, ...)
{
    va_list args;
    va_start(args, fmt);
    i32 r = stbsp_vsnprintf(buffer, count, fmt, args);
    va_end(args);
    return r;
}
i32 Sprintf(ArrayView<char> buffer, char const* fmt, ...)
{
    va_list args;
    va_start(args, fmt);
    i32 r = stbsp_vsnprintf(buffer.data, (i32)buffer.count, fmt, args);
    va_end(args);
    return r;
}
i32 SprintfArgs(char* buffer, i32 count, char const* fmt, va_list args)
{
    return stbsp_vsnprintf(buffer, count, fmt, args);
}
i32 SprintfArgs(ArrayView<char> buffer, char const* fmt, va_list args)
{
    return stbsp_vsnprintf(buffer.data, (i32)buffer.count, fmt, args);
}
