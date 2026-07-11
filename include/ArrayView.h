#pragma once

#include "Math.h"
#include "Debug.h"

template <typename T>
struct ArrayView {
    u64 count = 0;
    T* data = 0;

    inline [[nodiscard]] T& operator[](const u64 index) const
    {
        ASSERT(index < count);
        return data[index];
    }

    inline [[nodiscard]] u64 size() const
    {
        return count;
    }
    inline [[nodiscard]] u64 Bytes() const
    {
        return sizeof(T) * count;
    }
    inline [[nodiscard]] T* begin() const
    {
        return data;
    }
    inline [[nodiscard]] T* end() const
    {
        return data + count;
    }
    inline [[nodiscard]] T& First() const
    {
        ASSERT(count);
        ASSERT(data);
        return data[0];
    }
    inline [[nodiscard]] T& Last() const
    {
        ASSERT(count);
        ASSERT(data);
        return data[count - 1];
    }
    inline [[nodiscard]] bool IsValid() const
    {
        return count && data;
    }
    inline [[nodiscard]] u64 ElementBytes() const
    {
        return sizeof(T);
    }

    void CopyFrom(const ArrayView<T> source)
    {
        if (Bytes() != source.Bytes())
        {
            std::string msg = ToString("Trying to CopyArrayView but memory size is missmatched source: %u dest: %u", source.Bytes(), Bytes());
            DebugPrint(msg.c_str());
            ASSERT_MSG(false, msg.c_str());
            return;
        }
        memmove((void*)data, (void*)source.data, source.Bytes());
        count = source.count;
    }
};

typedef ArrayView<char>     StringView;
typedef ArrayView<wchar_t> WStringView;

template<typename T, u64 size>
[[nodiscard]] ArrayView<T> CreateArrayView(T(&source)[size])
{
    ArrayView<T> view;
    view.count = size;
    view.data = source;
    return view;
}

template<typename T>
[[nodiscard]] ArrayView<T> CreateArrayView(T* source, u64 count)
{
    ArrayView<T> view;
    view.count = count;
    view.data = source;
    return view;
}

template<typename T>
[[nodiscard]] ArrayView<T> CreateArrayView(std::vector<T>& source)
{
    ArrayView<T> view;
    view.count = source.size();
    view.data = source.data();
    return view;
}

template<typename T>
[[nodiscard]] ArrayView<const T> CreateArrayView(const std::vector<T>& source)
{
    ArrayView<const T> view = {
        .count = source.size(),
        .data = source.data(),
    };
    return view;
}

[[nodiscard]] inline StringView CreateArrayView(std::string& source)
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
        std::string msg = ToString("Trying to create SubArrayView with length: %u(%i) longer than the source: %u(%i)", length, length, source.count, source.count);
        DebugPrint(msg.c_str());
        ASSERT_MSG(false, msg.c_str());
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
        std::string msg = ToString("Trying to create SubArrayView with start + length: %u(%i) longer than the source: %u(%i)", end, end, source.count, source.count);
        DebugPrint(msg.c_str());
        ASSERT_MSG(false, msg.c_str());
        return source;
    }

    if (length < 0)
    {
        std::string msg = ToString("Trying to create SubArrayView with length: %u(%i) shorter than the source: %u(%i)", length, length, source.count, source.count);
        DebugPrint(msg.c_str());
        ASSERT_MSG(false, msg.c_str());
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
        std::string msg = ToString("Trying to CopyArrayView but memory size is missmatched source: %u dest: %u", source.Bytes(), dest.Bytes());
        DebugPrint(msg.c_str());
        ASSERT_MSG(false, msg.c_str());
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
    u64 size = Min(src_size, dst_size);
    memmove((void*)dest.data, (void*)source.data, size);
}
