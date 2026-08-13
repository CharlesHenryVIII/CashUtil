#pragma once

#include "CashMath.h"

#include <cstring>

//#include <format>

template <typename T>
struct ArrayView {
    u64 count = 0;
    T* data = 0;

    [[nodiscard]] inline operator ArrayView<const T>() const
    {
        return { count, data };
    }
    [[nodiscard]] inline T& operator[](const u64 index) const
    {
        ASSERT(index < count);
        return data[index];
    }

    [[nodiscard]] inline u64 size() const
    {
        return count;
    }
    [[nodiscard]] inline u64 Bytes() const
    {
        return sizeof(T) * count;
    }
    [[nodiscard]] inline T* begin() const
    {
        return data;
    }
    [[nodiscard]] inline T* end() const
    {
        return data + count;
    }
    [[nodiscard]] inline T& First() const
    {
        ASSERT(count);
        ASSERT(data);
        return data[0];
    }
    [[nodiscard]] inline T& Last() const
    {
        ASSERT(count);
        ASSERT(data);
        return data[count - 1];
    }
    [[nodiscard]] inline bool IsValid() const
    {
        return count && data;
    }
    [[nodiscard]] inline u64 ElementBytes() const
    {
        return sizeof(T);
    }

    bool CopyFrom(const ArrayView<T> source)
    {
        if (Bytes() != source.Bytes())
        {
            //TODO(CSH): Fix this
            //std::string msg = std::format("Trying to create SubArrayView with start + length: {} longer than the source: {}", source.Bytes(), Bytes());
            //DebugPrint(msg.c_str());
            //ASSERT_MSG(false, msg.c_str());
            FAIL;
            return false;
        }
        memmove((void*)data, (void*)source.data, source.Bytes());
        count = source.count;
    }
};

typedef ArrayView<char>     StringView;
typedef ArrayView<wchar_t> WStringView;

template<typename T, u64 size>
[[nodiscard]] constexpr ArrayView<T> CreateArrayView(T(&source)[size])
{
    ArrayView<T> view;
    view.count = size;
    view.data = source;
    return view;
}

template<typename T>
[[nodiscard]] constexpr ArrayView<T> CreateArrayView(T* source, u64 count)
{
    ArrayView<T> view;
    view.count = count;
    view.data = source;
    return view;
}

template<typename T>
[[nodiscard]] constexpr ArrayView<T> CreateArrayView(std::vector<T>& source)
{
    ArrayView<T> view;
    view.count = source.size();
    view.data = source.data();
    return view;
}

template<typename T>
[[nodiscard]] constexpr ArrayView<const T> CreateArrayView(const std::vector<T>& source)
{
    ArrayView<const T> view = {
        .count = source.size(),
        .data = source.data(),
    };
    return view;
}

[[nodiscard]] constexpr inline StringView CreateArrayView(std::string& source)
{
    StringView view = {
        .count = source.size(),
        .data = source.data(),
    };
    return view;
}

template<typename T>
[[nodiscard]] ArrayView<T> CreateSubArrayView(const ArrayView<T> source, u64 length)
{
    ArrayView<T> view;
    if (length > source.count)
    {
        //TODO(CSH): Fix this
        //std::string msg = std::format("Trying to create SubArrayView with start + length: {} longer than the source: {}", length, source.count);
        //DebugPrint(msg.c_str());
        //ASSERT_MSG(false, msg.c_str());
        FAIL;
        return source;
    }

    view.count = length;
    view.data = source.data;
    return view;
}

template<typename T>
[[nodiscard]] ArrayView<T> CreateSubArrayView(const ArrayView<T> source, u64 start, u64 length)
{
    ArrayView<T> view;

    ASSERT(start < source.count);
    ASSERT(length < source.count);
    const u64 end = start + length;
    if (end > source.count)
    {

        //TODO(CSH): Fix this
        //std::string msg = std::format("Trying to create SubArrayView with start + length: {} longer than the source: {}", end, source.count);
        //DebugPrint(msg.c_str());
        //ASSERT_MSG(false, msg.c_str());
        FAIL;
        return source;
    }

    if (length < 0)
    {
        //TODO(CSH): Fix this
        //std::string msg = std::format("Trying to create SubArrayView with length: {} shorter than the source: {}", length, source.count);
        //DebugPrint(msg.c_str());
        //ASSERT_MSG(false, msg.c_str());
        FAIL;
        return {};
    }

    view.count = length;
    view.data = source.data + source.ElementBytes() * start;
    return view;
}

template<typename T>
void CopyArrayView(const ArrayView<T> source, ArrayView<T>& dest)
{
    if (dest.Bytes() != source.Bytes())
    {
        //TODO(CSH): Fix this
        //std::string msg = std::format("Trying to CopyArrayView but memory size is missmatched source: %{} dest: %{}", source.Bytes(), dest.Bytes());
        //DebugPrint(msg.c_str());
        //ASSERT_MSG(false, msg.c_str());
        FAIL;
        return;
    }
    ASSERT(source.count == dest.count);//what do we do here if this isn't true?
    //dest.count = source.count;
    memmove((void*)dest.data, (void*)source.data, source.Bytes());
}

template<typename T>
void CopyArrayViewMismatched(const ArrayView<T>& source, ArrayView<T>& dest)
{
    const u64 src_size = source.Bytes();
    const u64 dst_size = dest.Bytes();
    const u64 size = Min(src_size, dst_size);
    std::memmove((void*)dest.data, (void*)source.data, size);
}