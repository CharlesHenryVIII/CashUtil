#pragma once
#include "CashMath.h"
#include "CashMemoryArena.h"
#include "CashArrayView.h"
//#include "CashConsole.h"

typedef u16 DataIDIndex;
typedef u16 DataIDGen;
#define DATAID_TYPE(__name)                                         \
union __name {                                                      \
    u32 e;                                                          \
    struct {                                                        \
        DataIDIndex index;                                          \
        DataIDGen   generation;                                     \
    };                                                              \
    constexpr bool IsValid() const { return e > 0; };               \
    __name(u16 _i, u16 _gen) : index(_i), generation(_gen) {};      \
    __name(u32 _e) : e(_e) {};                                      \
    __name() = default;                                             \
                                                                    \
    friend inline [[nodiscard]] bool operator==(__name a, __name b) \
    {                                                               \
        return a.e == b.e;                                          \
    }                                                               \
    friend inline [[nodiscard]] bool operator!=(__name a, __name b) \
    {                                                               \
        return a.e != b.e;                                          \
    }                                                               \
};

#define foreach(it__, idarray__) for (auto it__ = (idarray__).Iterate(); (idarray__.NextValidElement(&it__));)

//NOTE(CSH): This is to get around recursive includes
void* _IdArrayReserve(u64 bytes);
void _IdArrayCommit(void* p, u64 bytes);
bool _IdArrayFree(void* p, u64 bytes);

constexpr u16 _idarray_invalid_index = -1;

#define PAGESIZE 4096
template <typename T, typename TID>
struct IdArray
{
    IdArray()
    {
        data = (T*)_IdArrayReserve(sizeof(T) * max_elements);
        const u16 initial_elements = 64;
        u32 requested_size = sizeof(T) * initial_elements;
        CommitMoreMemory(requested_size);

        for (u16 i = 0; i < initial_elements; i++)
        {
            if (i + 1 == initial_elements)
                data[i].data_id.index = _idarray_invalid_index;
            else
                data[i].data_id.index = i + 1;

            data[i].data_id.generation = 1;
        }
    }
    ~IdArray()
    {
        for (auto a = Iterate(); NextValidElement(&a);)
        {
            Erase(a->data_id);
        }
        _IdArrayFree(data, 0);
    }

    bool IsEmpty() const { return highest_active == _idarray_invalid_index; };

    [[nodiscard]] T* TryGet(const TID id)
    {
        if (id.index <= highest_active)
        {
            if (data[id.index].data_id.generation == id.generation)
            {
                return &data[id.index];
            }
        }
        return nullptr;
    }

    [[nodiscard]] const T* TryGet(const TID id) const
    {
        if (id.index <= highest_active)
        {
            if (data[id.index].data_id.generation == id.generation)
            {
                return &data[id.index];
            }
        }
        return nullptr;
    }

    [[nodiscard]] T* CreateNew()
    {
        TID id;
        if (first_inactive == _idarray_invalid_index)
        {
            highest_active++;
            id.index = highest_active;
            id.generation = data[id.index].data_id.generation;// = 1;
        }
        else
        {
            id.index = first_inactive;
            id.generation = data[first_inactive].data_id.generation;
            first_inactive = data[first_inactive].data_id.index;
        }

        if (id.index >= committed_memory_until_index)
        {
            const u32 requested_size = committed_memory_until_index * sizeof(T) + PAGESIZE;
            CommitMoreMemory(requested_size);
        }

        new (&data[id.index]) T();
        data[id.index].data_id = id;
        return &data[id.index];
    }

    bool Erase(const TID id)
    {
        T* element = TryGet(id);
        if (element)
        {
            TID this_id = element->data_id;

            if (this_id.index == highest_active)
            {
                T* previous_element = element;
                if (PreviousValidElement(&previous_element))
                {
                    highest_active = previous_element->data_id.index;
                }
                else
                {
                    //NOTE(CSH): You get here by deleting every element in the id array.
                    //Usually done when the IdArray destructor is called
                    //FAIL; //is it valid to be here?
                    highest_active = _idarray_invalid_index;
                    first_inactive = _idarray_invalid_index;
                }
            }

            element->~T();
            this_id.generation++;
            if (this_id.generation == 0)
                this_id.generation = 1;
            //ERROR(CSH): Not setting INDEX!!??

#ifdef _DEBUG
            memset(element, 0xFF, sizeof(T));
#endif

            element->data_id = TID(first_inactive, this_id.generation);
            first_inactive = (highest_active == _idarray_invalid_index) ? highest_active : id.index;
            return true;
        }
        return false;
    }

    T* Iterate() const
    {
        return nullptr;
        if (IsEmpty())
            return nullptr;
        for (u16 i = 0; i <= highest_active; i++)
        {
            if (IsValid(i))
                return &data[i];
        }
        return nullptr;
    }


    bool IsValid(u16 index) const
    {
        return data[index].data_id.index == index;
    }

    bool NextValidElement(T** element) const
    {
        u16 start = 0;
        if (!element || !*element)
            start = 0;
        else
            start = (*element)->data_id.index + 1;
        if (highest_active == u16(-1))
            return false;
        for (u16 i = start; i <= highest_active; i++)
        {
            if (IsValid(i))
            {
                *element = &data[i];
                return true;
            }
        }
        return false;
    }

    bool PreviousValidElement(T** element)
    {
        if (element && *element)
        {
            for (u16 i = (*element)->data_id.index - 1; i != _idarray_invalid_index; i--)
            {
                if (IsValid(i))
                {
                    *element = &data[i];
                    return true;
                }
            }
        }
        return false;
    }

 private:
        void CommitMoreMemory(u32 requested_size)
        {
            _IdArrayCommit(data, requested_size);
            const u16 page_count = (u16)ceil(float(requested_size) / PAGESIZE);
            committed_memory_until_index = (page_count * PAGESIZE) / sizeof(T);
        }
public:

    T* data;
    u16 first_inactive = _idarray_invalid_index;
    u16 highest_active = _idarray_invalid_index;
    u16 committed_memory_until_index = _idarray_invalid_index;
    const static u16 max_elements = -2;
};


template <typename T, typename TID, u16 _size>
struct StaticIdArray
{
    StaticIdArray()
    {
        memset(storage, 0xFF, sizeof(storage));
    }
    ~StaticIdArray()
    {
        for (auto a = Iterate(); NextValidElement(&a);)
        {
            Erase(a->data_id);
        }
    }

    [[nodiscard]] T* TryGet(const TID id)
    {
        if (id.index <= highest_active)
        {
            T* d =  GetData();
            if (d[id.index].data_id.generation == id.generation)
            {
                return &d[id.index];
            }
        }
        return nullptr;
    }

    [[nodiscard]] const T* TryGet(const TID id) const
    {
        if (id.index <= highest_active)
        {
            const T* d =  GetData();
            if (d[id.index].data_id.generation == id.generation)
            {
                return &d[id.index];
            }
        }
        return nullptr;
    }

    [[nodiscard]] T* CreateNew()
    {
        TID id;
        T* d = GetData();
        if (first_inactive == (u16)-1)
        {
            highest_active++;
            id.index = highest_active;
            id.generation = 1;
        }
        else
        {
            id.index = first_inactive;
            id.generation = d[first_inactive].data_id.generation;
            first_inactive = d[first_inactive].data_id.index;
        }

        new (&d[id.index]) T();
        d[id.index].data_id = id;
        return &d[id.index];
    }

    bool Erase(const TID id)
    {
        T* element = TryGet(id);
        if (element)
        {
            TID this_id = element->data_id;

            if (this_id.index == highest_active)
            {
                T* previous_element = element;
                if (PreviousValidElement(&previous_element))
                {
                    highest_active = previous_element->data_id.index;
                }
                else
                {
                    //NOTE(CSH): You get here by deleting every element in the id array.
                    //Usually done when the IdArray destructor is called
                    //FAIL; //is it valid to be here?
                    highest_active = 0;
                }
            }

            element->~T();
            this_id.generation++;
            if (this_id.generation == 0)
                this_id.generation = 1;

#ifdef _DEBUG
            memset(element, 0xFF, sizeof(T));
#endif

            element->data_id = TID(first_inactive, this_id.generation);
            first_inactive = id.index;
            return true;
        }
        return false;
    }

    T* Iterate() const
    {
        return nullptr;
        //if (highest_active == (u16)(-1))
        //    return nullptr;
        //for (u16 i = 0; i <= highest_active; i++)
        //{
        //    if (IsValid(i))
        //        return &data[i];
        //}
        //return nullptr;
    }


    bool IsValid(u16 index) const
    {
        return GetData()[index].data_id.index == index;
    }

    bool NextValidElement(const T** element) const
    {
        u16 start = 0;
        if (!element || !*element)
            start = 0;
        else
            start = (*element)->data_id.index + 1;
        if (highest_active == u16(-1))
            return false;
        for (u16 i = start; i <= highest_active; i++)
        {
            if (IsValid(i))
            {
                *element = &GetData()[i];
                return true;
            }
        }
        return false;
    }

    bool NextValidElement(T** element)
    {
        u16 start = 0;
        if (!element || !*element)
            start = 0;
        else
            start = (*element)->data_id.index + 1;
        if (highest_active == u16(-1))
            return false;
        for (u16 i = start; i <= highest_active; i++)
        {
            if (IsValid(i))
            {
                *element = &GetData()[i];
                return true;
            }
        }
        return false;
    }

    bool PreviousValidElement(T** element)
    {
        if (element && *element)
        {
            for (u16 i = (*element)->data_id.index - 1; i != (u16)-1; i--)
            {
                if (IsValid(i))
                {
                    *element = &GetData()[i];
                    return true;
                }
            }
        }
        return false;
    }

            T* GetData()        { return reinterpret_cast<      T*>(storage); }
    const   T* GetData() const  { return reinterpret_cast<const T*>(storage); }

    const static u16 max_elements = _size < -2 ? -2 : _size;
    const static u64 bytes = max_elements * sizeof(T);
    alignas(T) u8 storage[bytes];
    u16 first_inactive = -1;
    u16 highest_active = -1;
    static_assert(_size <= (u16)(-2));
};


template <typename T, u64 count>
struct StaticArray
{
    u64 used = 0;
    T data[count];
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
        VALIDATE_VM(used < count, nullptr, LogLevel_Warning, "Trying to insert past size of static array: %i elements", count);
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
    inline u8* AddRaw(const u8* in_data, const u64 size)
    {
        VALIDATE_V(size + used < count, nullptr);
        memmove((void*)(data + used), in_data, size);
        u8* element = (u8*)(data + used);
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

template <typename T, u64 count>
[[nodiscard]] constexpr inline ArrayView<T> CreateArrayView(StaticArray<T, count>& source)
{
    ArrayView<T> view = {
        .count = source.used,
        .data = source.data,
    };
    return view;
}
template <typename T, u64 count>
[[nodiscard]] constexpr inline ArrayView<const T> CreateArrayView(const StaticArray<T, count>& source)
{
    ArrayView<const T> view = {
        .count = source.used,
        .data = source.data,
    };
    return view;
}

template <u64 _count>
struct InlineString : StaticArray<char, _count>
{
    InlineString() = default;
    InlineString(const char* str)
    {
        if (str)
            CopyFrom(str);
    }

    u8* CopyFrom(const char* s, u64 len)
    {
        if (len <= _count)
            return this->AddRaw((u8*)s, len);

        FAIL;
        len = _count - 1;
        u8* c = this->AddRaw((u8*)s, len);
        this->Add(0);
        return c;
    }

    u8* CopyFrom(const char* str)           { return CopyFrom(str,      strlen(str) + 1); };
    u8* CopyFrom(const InlineString& str)   { return CopyFrom(str.data, str.used); };

    InlineString& operator=(const char* str)
    {
        this->used = 0;
        if (str)
            CopyFrom(str);
        return *this;
    }
    InlineString& operator=(const InlineString& str)
    {
        this->used = 0;
        CopyFrom(str);
        return *this;
    }
};


template <typename T>
struct DynamicArray
{
    u64 used = 0;
    u64 cap = 0; //default = 16
    T* data = nullptr;
    Arena* arena = nullptr;
    //static constexpr u64 invalid_index = (u64)(-1);
    DynamicArray(Arena* _arena) : arena(_arena) { Realloc(16); };
    ~DynamicArray() { ArenaFree(arena); };

    T* Push(T& a)
    {
        const u64 _count = 1;
        Add(_count);
        data[used - _count] = a;
        return &(data[used - _count]);
    }

    void Add(u64 _count)
    {
        if (_count + used > cap)
            Realloc(_count + used);
        used += _count;
    }

    T* Insert(T& a, u64 pos, u64 _count = 1)
    {
        Add(_count);
        memmove(&(data[pos + _count]), &(data[pos]), sizeof(T) * (used - pos - _count));
        return &(data[pos]);
    }

    //TODO(CSH): Replace with ArrayView<T>
    void Append(T* a, u64 _count)
    {
        if (_count + used > cap)
        {
            Realloc(_count);
        }
        memmove(&(data[used]), a, _count * sizeof(T));
        used += _count;
    }

    void Reserve(u64 total_count)
    {
        if (total_count > cap)
            Realloc(total_count - cap);
    }

    T* First() { return used ?  data       : nullptr; };
    T* Last () { return used ? &data[used] : nullptr; };

    u64 GetBytesUsed() { return used * sizeof(T); };
    u64 GetBytesCapacity() { return cap * sizeof(T); };

private:
    void Realloc(u64 needed)
    {
        VALIDATE(arena && arena->IsValid());//, LogLevel_Error, "Invalid arena when trying to realloc dynamic array");

        u64 new_cap = (u64)Ceiling(1.25f * cap);
        if (needed + cap > new_cap)
            new_cap = (u64)Ceiling(1.25f * (needed + cap));

        T* new_data = (T*)ArenaPush(arena, new_cap);
        memmove((void*)new_data, (void*)data, GetBytesUsed());
        memset(new_data + GetBytesUsed(), 0, new_cap * sizeof(T) - GetBytesUsed());
        data = new_data;
        cap = new_cap;
    }
};

template <typename T>
[[nodiscard]] constexpr inline ArrayView<T> CreateArrayView(DynamicArray<T>& source)
{
    ArrayView<T> view = {
        .count = source.used,
        .data = source.data,
    };
    return view;
}
template <typename T>
[[nodiscard]] constexpr inline ArrayView<const T> CreateArrayView(const DynamicArray<T>& source)
{
    ArrayView<const T> view = {
        .count = source.used,
        .data = source.data,
    };
    return view;
}
