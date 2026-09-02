#pragma once
#include "CashMath.h"

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
                data[i].data_id.index = -1;
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
        if (first_inactive == (u16)-1)
        {
            highest_active++;
            id.index = highest_active;
            id.generation = 1;
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
        if (highest_active == (u16)(-1))
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
            for (u16 i = (*element)->data_id.index - 1; i != (u16)-1; i--)
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
    u16 first_inactive = -1;
    u16 highest_active = -1;
    u16 committed_memory_until_index = -1;
    const static u16 max_elements = -2;
};


template <typename T, typename TID, u16 _size>
struct StaticIdArray
{
    StaticIdArray()
    {
        memset(data, 0xFF, _size * sizeof(T));
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
        if (first_inactive == (u16)-1)
        {
            highest_active++;
            id.index = highest_active;
            id.generation = 1;
        }
        else
        {
            id.index = first_inactive;
            id.generation = data[first_inactive].data_id.generation;
            first_inactive = data[first_inactive].data_id.index;
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
        return data[index].data_id.index == index;
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
                *element = &data[i];
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
            for (u16 i = (*element)->data_id.index - 1; i != (u16)-1; i--)
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

public:

    const static u16 max_elements = _size > -2 ? -2 : _size;
    T data[max_elements];
    u16 first_inactive = -1;
    u16 highest_active = -1;
    static_assert(_size <= (u16)(-2));
};
