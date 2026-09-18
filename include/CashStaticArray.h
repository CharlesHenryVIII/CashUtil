#pragma once

#include "CashMath.h"
#include "CashArrayView.h"
///#include "CashConsole.h"

template <typename T, u64 count>
struct StaticArray
{
    u64 used = 0;
    T data[count] = {};
    static constexpr u64 invalid_index = (u64)(-1);

    StaticArray() = default;
    ~StaticArray() = default;

    // ===========================
    //          ACCESSORS
    // ===========================
    [[nodiscard]] inline T* begin()             { return data; }
    [[nodiscard]] inline T* end()               { return data + used; }
    [[nodiscard]] inline T& First()             { ASSERT(count && used); return data[0]; }
    [[nodiscard]] inline T& Last()              { ASSERT(count && used); return data[used - 1]; }
    [[nodiscard]] inline T& operator[](u64 i)   { ASSERT(i < count && i < used); return data[i]; }

    // ===========================
    //      CONST ACCESSORS
    // ===========================
    [[nodiscard]] inline const T* begin()   const   { return data; }
    [[nodiscard]] inline const T* end()     const   { return data + used; }
    [[nodiscard]] inline const T& First()   const   { ASSERT(count && used); return data[0]; }
    [[nodiscard]] inline const T& Last()    const   { ASSERT(count && used); return data[used - 1]; }
    [[nodiscard]] inline const T& operator[](u64 i) const { ASSERT(i < count && i < used); return data[i]; } 
    [[nodiscard]] inline u64 GetIndexOf(const T& a) const
    {
        for (u64 i = 0; i < used; i++)
        {
            if (data[i] == a)
                return i;
        }
        return invalid_index;
    }

    // ===========================
    //          GENERAL
    // ===========================

    //Bytes of currently used array
    [[nodiscard]] inline u64 Bytes() const { return sizeof(T) * used; }

    inline T* Add(const T& item)
    {
        ASSERT(used < count);
        data[used] = item;
        T* element = &(data[used]);
        used++;
        return element;
    }
    inline T* Add(const ArrayView<T>& items)
    {
        ASSERT(used < count);
        ASSERT(used + items.count < count);
        const u64 start_count = used;
        for (u64 i = 0; i < items.count && i + start_count < count; i++)
        {
            data[used] = items[i];
            used++;
        }
        return &data[start_count + 1];
    }
    //returns true if it already existed
    inline bool AddUnique(const T& item)
    {
        ASSERT(used < count);
        const u64 i = GetIndexOf(item);
        if (i != invalid_index)
            return &data[i];
        return Add(item);
    }
    inline u8* AddRaw(const u8* data, const u64 size)
    {
        VALIDATE_V(size + used < count, nullptr);
        memmove((void*)&data[used], data, size);
        u8* element = (u8*)&(data[used]);
        used = used + size;
        return element;
    }

    inline void Erase(u64 index)
    {
        VALIDATE(index < used && index != invalid_index);
        for (u64 i = index; i < used; i++)
        {
            data[i] = data[i + 1];
        }
        used--;
    }
    inline void Erase(const T& item)
    {
        const u64 index = GetIndexOf(item);
        if (index == invalid_index)
            return;
        Erase(index);
    }

    inline void Insert(const T& item, u64 index)
    {
        VALIDATE(index < used); //Log("StaticArray", LogLevel_Error, "Tried to Insert() beyond used space: %i of %i", index, used);
        used++;
        for (u64 i = used; i >= index && i != invalid_index; i--)
        {
            data[i] = data[i - 1];
        }
        data[index] = item;
    }

    inline void Clear()
    {
        for (u64 i = 0; i < used; i++)
        {
            T* element = &data[i];
            element->~T();
        }
        used = 0;
    }
    
    [[nodiscard]] inline operator ArrayView<T>()
    {
        return { .count = count , .data = data };
    }
    [[nodiscard]] inline operator ArrayView<const T>() const
    {
        return { .count = count , .data = data };
    }
};

template <u64 _count>
struct InlineString : StaticArray<char, _count>
{
    //ArrayView<T> ToArrayView() { return CreateArrayView(s, capacity); };
    //StringView   ToStringView()  { return ToArrayView(); };
};
