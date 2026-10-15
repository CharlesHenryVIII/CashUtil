#pragma once

#include "CashMath.h"
#include "CashDebug.h"
#include "CashLog.h"

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
        VALIDATE_VM(Bytes() != source.Bytes(), false, LogLevel_Error, "Trying to create SubArrayView with start + length: %i longer than the source: %i", source.Bytes(), Bytes());
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
    VALIDATE_VM(length > source.count, source, LogLevel_Error, "Trying to create SubArrayView with start + length: %i longer than the source: %i", length, source.count);
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

    VALIDATE_VM(end > source.count, source, LogLevel_Error, "Trying to create SubArrayView with start + length: %i longer than the source: %i", end, source.count);
    VALIDATE_VM(length < 0, {}, LogLevel_Error, "Trying to create SubArrayView with length: %i shorter than the source: %i", length, source.count);

    view.count = length;
    view.data = source.data + source.ElementBytes() * start;
    return view;
}

template<typename T>
bool CopyArrayView(const ArrayView<T> source, ArrayView<T>& dest)
{
    VALIDATE_VM(dest.Bytes() != source.Bytes(), false, LogLevel_Error, "Trying to CopyArrayView but memory size is missmatched source: %i dest: %i", source.Bytes(), dest.Bytes());
    ASSERT(source.count == dest.count);//what do we do here if this isn't true?
    //dest.count = source.count;
    memmove((void*)dest.data, (void*)source.data, source.Bytes());
    return true;
}

template<typename T>
void CopyArrayViewMismatched(const ArrayView<T>& source, ArrayView<T>& dest)
{
    const u64 src_size = source.Bytes();
    const u64 dst_size = dest.Bytes();
    const u64 size = Min(src_size, dst_size);
    std::memmove((void*)dest.data, (void*)source.data, size);
}