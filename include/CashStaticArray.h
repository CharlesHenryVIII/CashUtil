#include "CashMath.h"
#include "CashArrayView.h"

template <typename T, u64 count>
struct StaticArray
{
    u64 used = 0;
    T data[count];

    StaticArray() = default;
    ~StaticArray() = default;

    // ===========================
    //          ACCESSORS
    // ===========================
    [[nodiscard]] inline T* begin()             { return data; }
    [[nodiscard]] inline T* end()               { return data + count; }
    [[nodiscard]] inline T& First()             { ASSERT(count && used); return data[0]; }
    [[nodiscard]] inline T& Last()              { ASSERT(count && used); return data[used - 1]; }
    [[nodiscard]] inline T& operator[](u64 i)   { ASSERT(i < count && i < used); return data[i]; }

    // ===========================
    //      CONST ACCESSORS
    // ===========================
    [[nodiscard]] inline const T* begin()   const   { return data; }
    [[nodiscard]] inline const T* end()     const   { return data + count; }
    [[nodiscard]] inline const T& First()   const   { ASSERT(count && used); return data[0]; }
    [[nodiscard]] inline const T& Last()    const   { ASSERT(count && used); return data[used - 1]; }
    [[nodiscard]] inline const T& operator[](u64 i) const { ASSERT(i < count && i < used); return data[i]; } 

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
