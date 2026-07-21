#include "CashString.h"
#include "CashSystem.h"

const char* ReadEntireFileAsString(const char* fileName)
{
    FILE* file;
    if (file = fopen(fileName, "rb"))
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

bool TextDetection(const std::string& string, const std::string& check)
{
    for (i32 i = 0; i + check.size() <= string.size(); i++)
    {
        if (string.substr(i, check.size()) == check)
            return true;
    }
    return false;
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
    i32 i = vsnprintf(buffer, arrsize(buffer), fmt, args);
    va_end(args);
    return buffer;
}

std::wstring ToString(const wchar_t* fmt, ...)
{
    va_list args;
    va_start(args, fmt);
    wchar_t buffer[4096];
    i32 i = vswprintf(buffer, arrsize(buffer), fmt, args);
    va_end(args);
    return buffer;
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

bool StringCompare(StringCase case_sensitivity, const char* a, const char* b)
{
    switch (case_sensitivity)
    {
    case StringCase_Sensitive:
        if (strcmp(a, b) == 0)
            return true;
        break;
    case StringCase_Insensitive:
        if (_stricmp(a, b) == 0)
            return true;
        break;
    }
    return false;
}

bool StringCompare(StringCase case_sensitivity, const std::string& a, const std::string& b)
{
    if (a.size() != b.size())
        return false;

    switch (case_sensitivity)
    {
    case StringCase_Sensitive:
        if (strcmp(a.c_str(), b.c_str()) == 0)
            return true;
        break;
    case StringCase_Insensitive:
        if (_stricmp(a.c_str(), b.c_str()) == 0)
            return true;
        break;
    }
    return false;
}

bool ContainsString(const std::wstring& source, const std::wstring& find, StringCase case_insensitive)
{
    std::string s;
    SysConvertWideCharToMultiByte(s, source);
    std::string f;
    SysConvertWideCharToMultiByte(f, find);
    if (case_insensitive == StringCase_Insensitive)
    {
        ToLower(s);
        ToLower(f);
    }
    if (s.find(f.c_str()))
        return true;
    return false;
}

bool CopyFile(const Path& source, const Path& dest)
{
    if (source.empty() || dest.empty())
    {
        DebugPrint("Failed to copy file EMPTY: src: \"%s\" dest: \"%s\"", source.string().c_str(), dest.string().c_str());
        FAIL;
        return false;
    }
    std::error_code ec;
    fs::create_directories(dest.parent_path(), ec);
    if (ec)
    {
        DebugPrint("Failed to create directories for dest: \"%s\"", dest.string().c_str());
        DebugPrint("create_directories Failure: \"%d\", \"%s\"", ec.value(), ec.message().c_str());
        FAIL;
        return false;
    }

    bool result = std::filesystem::copy_file(source, dest, std::filesystem::copy_options::overwrite_existing, ec);
    if (ec)
    {
        DebugPrint("Failed to copy file src: \"%s\" dest: \"%s\"", source.string().c_str(), dest.string().c_str());
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
        DebugPrint("Failed to copy file EMPTY: src: \"%s\" dest: \"%s\" relative: \"%s\"", source.string().c_str(), dest.string().c_str(), relative.string().c_str());
        FAIL;
        return false;
    }
    Path full_source = source / relative;
    Path full_dest = dest / relative;

    std::error_code ec;
    fs::create_directories(full_dest.parent_path(), ec);
    if (ec)
    {
        DebugPrint("Failed to create directories for dest: \"%s\"", (full_dest).string().c_str());
        DebugPrint("create_directories Failure: \"%d\", \"%s\"", ec.value(), ec.message().c_str());
        FAIL;
        return false;
    }

    bool result = fs::copy_file(full_source, full_dest, fs::copy_options::overwrite_existing, ec);
    if (ec)
    {
        DebugPrint("Failed to copy file src: \"%s\" dest: \"%s\"", full_source.string().c_str(), full_dest.string().c_str());
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
        DebugPrint("Failed to copy folder EMPTY: src: \"%s\" dest: \"%s\" relative: \"%s\"", source.string().c_str(), dest.string().c_str(), relative.string().c_str());
        FAIL;
        return false;
    }
    Path full_source = source / relative;
    Path full_dest = dest / relative;

    std::error_code ec;
    fs::create_directories(full_dest.parent_path(), ec);
    if (ec)
    {
        DebugPrint("Failed to create directories for dest: \"%s\"", full_dest.parent_path().string().c_str());
        DebugPrint("create_directories Failure: \"%d\", \"%s\"", ec.value(), ec.message().c_str());
        FAIL;
        return false;
    }

    fs::copy(full_source, full_dest, fs::copy_options::recursive | fs::copy_options::overwrite_existing, ec);
    if (ec)
    {
        DebugPrint("Failed to copy folder src: \"%s\" dest: \"%s\" ", full_source.string().c_str(), full_dest.string().c_str());
        DebugPrint("copy_file Failure: \"%d\", \"%s\"", ec.value(), ec.message().c_str());
        FAIL;
        return false;
    }
    return true;
}

void CopyString(char** dest, const char* source, const u64 max_length)
{
    memmove(*dest, source, (size_t)Min<u64>(max_length, strnlen_s(source, (size_t)max_length)));
}

void PathRemoveExtension(std::wstring& path)
{
    size_t end = path.find_last_of(L".");
    if (end == std::wstring::npos)
    {
        DebugPrint("GetFilename() failed to find \".\" for %s", path.c_str());
        return;
    }

    path.erase(end, path.size() - end);
}

std::wstring PathGetFilename(const std::wstring& path)
{
    size_t start = path.find_last_of(L"\\");
    if (start == std::wstring::npos)
        start = path.find_last_of(L"/");
    if (start == std::wstring::npos)
    {
        DebugPrint("GetFilename() failed to find \"/\" or \"\\\" for %s", path.c_str());
        return path;
    }

    size_t end = path.find_last_of(L".");
    if (end == std::wstring::npos)
    {
        DebugPrint("GetFilename() failed to find \".\" for %s", path.c_str());
        return L"";
    }

    if (start++ >= path.size() || start >= end)
    {
        DebugPrint("GetFilename() failed to get valid range for start(%i) and end(%i) for %s", start, end, path.c_str());
        return L"";
    }

    std::wstring_view view = path;
    std::wstring result;
    result = view.substr(start, end - start);
    return result;
}

std::wstring PathGetFilenameWithExtension(const std::wstring& path)
{
    std::wstring result = L"";
    size_t start = path.find_last_of(L"\\");
    if (start == std::wstring::npos)
        start = path.find_last_of(L"/");
    if (start == std::wstring::npos)
    {
        DebugPrint("Failed to GetFilename() for %s", path.c_str());
        return result;
    }

    if (start++ >= path.size())
    {
        DebugPrint("Failed to GetFilename() for %s", path.c_str());
        return result;
    }

    std::wstring_view view = path;
    result = view.substr(start);
    return result;
}

std::wstring PathGetExtension(const std::wstring& path)
{
    std::wstring result = L"";
    const size_t start = path.find_last_of(L'.');
    if (start == std::wstring::npos)
    {
        DebugPrint("Failed to GetFilenameExtension() for %s", path.c_str());
        return result;
    }

    std::wstring_view view = path;
    result = view.substr(start);
    return result;
}

void PathCleanSlashs(std::wstring& s)
{
    size_t pos = s.find(L'\\');
    while (pos != std::wstring::npos)
    {
        s.replace(pos, 1, L"/", 1);
        pos = s.find(L'\\');
    }
}

std::wstring PathAddEndSlash(const std::wstring& s)
{
    if (s.back() != L'/')
        return s + L"/";
    return s;
}

std::wstring PathConcat(const std::wstring& a, const std::wstring& b)
{
    if (a.back() == L'/' || a.back() == L'\\')
    {
        return a + b;
    }
    return a + L"/" + b;
}

std::string PathConcat(const std::string& a, const std::string& b)
{
    if (a.back() == '/' || a.back() == '\\')
    {
        return a + b;
    }
    return a + "/" + b;
}

void CreateParentDirectories(const Path& path)
{
    if (!path.empty() && path.has_parent_path())
    {
        std::error_code ec;
        fs::create_directories(path.parent_path(), ec);
        if (ec)
        {
            DebugPrint("Failed to create directories for \"%s\"", path.string().c_str());
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

u64 FileReadAll(std::string& out, const Path& filepath)
{
    std::ifstream file(filepath, std::ios::binary | std::ios::ate);
    if (!file)
    {
        DebugPrint("Error: Failed to open file: %s", filepath.string().c_str());
        FAIL;
        return 0;
    }
    const u64 file_size = (u64)file.tellg();
    if (file_size > out.size())
        out.resize(file_size);
    file.seekg(0, std::ios::beg);
    file.read((char*)out.data(), file_size);
    return file_size;
}

u64 FileReadAll(std::wstring& out, const Path& filepath)
{
    std::wifstream file(filepath, std::ios::binary | std::ios::ate);
    if (!file)
    {
        DebugPrint("Error: Failed to open file: %s", filepath.string().c_str());
        FAIL;
        return 0;
    }
    const u64 file_size = (u64)file.tellg();
    if (file_size > out.size())
        out.resize(file_size);
    file.seekg(0, std::ios::beg);
    file.read((wchar_t*)out.data(), file_size);
    return file_size;
}