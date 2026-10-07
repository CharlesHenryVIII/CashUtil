#pragma once
#include "CashMath.h"

#define CASH_DEFAULT_ALIGNMENT 16
//#define ARENA_PREFIX [[nodiscard]] inline

struct Arena {
    u64 reserved;
    u64 committed;
    u64 used;
    void* data;
};

struct ArenaParams {
    u64 reserve_size = Mebibytes(64);
    u64 commit_size = 0;
};


[[nodiscard]]   Arena   ArenaAlloc(const u64 reserve_size = Mebibytes(64), const u64 commit_size = 0);
                void    ArenaFree(Arena* arena);
[[nodiscard]]   void*   ArenaPush(Arena* arena, u64 size, u64 alignment = CASH_DEFAULT_ALIGNMENT, bool zero = true);

#define ArenaPushStruct(arena, type)        (type*)_ArenaPush(arena, sizeof(size), alignof(type), true)
#define ArenaPushArray(arena, count, type)  (type*)_ArenaPush(arena, (count) * sizeof(type), alignof(type[1]), true)



[[nodiscard]]   char* ArenaPush(Arena* arena, const char* fmt, ...);
[[nodiscard]]   char* ArenaPushArgs(Arena* arena, const char* fmt, va_list args);

//Zeroes memory
void ArenaClear  (Arena* arena);
//Releases memory
void ArenaRelease(Arena* arena);