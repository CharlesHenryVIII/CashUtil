#include "CashMath.h"
#include "CashArrayView.h"

template <typename T, u64 _capacity>
struct StaticArray
{
    const u64 count = _capacity;
    u64 new_index = 0;
    T data[_capacity];

    StaticArray() = default;
    ~StaticArray() = default;

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
        ASSERT(new_index);
        return data[0];
    }
    //Returns the last valid/filled data
    [[nodiscard]] inline T& Last() const
    {
        ASSERT(count);
        ASSERT(new_index);
        return data[new_index - 1];
    }

    inline T* Add(const T& item)
    {
        ASSERT(new_index < count);
        data[new_index] = item;
        T* element = data[new_index];
        new_index++;
        return element;
    }

    inline void Clear()
    {
        for (i32 i = 0; i < new_index; i++)
        {
            T* element = &data[i];
            element->~T();
        }
        new_index = 0;
    }

    [[nodiscard]] inline T& operator[](u64 i)
    {
        ASSERT(i < count);
        ASSERT(i <= new_index);
        return data[i];
    }
    
    [[nodiscard]] inline operator ArrayView<T>()
    {
        return { .count = count , .data = data };
    }
};

template <u64 _capacity>
struct InlineString : StaticArray<char, _capacity>
{
    //ArrayView<T> ToArrayView() { return CreateArrayView(s, capacity); };
    //StringView   ToStringView()  { return ToArrayView(); };
};
