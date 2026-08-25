#pragma once
#include "CashMath.h"


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

Arena ArenaAlloc(const u64 reserve_size = Mebibytes(64), const u64 commit_size = 0);
void ArenaFree(Arena* arena);
void* _ArenaPush(Arena* arena, u64 size, u64 alignment, bool zero);

#define ArenaPush(arena, size)                     _ArenaPush(arena, size, DefaultAlignment, true)
#define ArenaPushStruct(arena, type)        (type*)_ArenaPush(arena, sizeof(size), alignof(type), true)
#define ArenaPushArray(arena, count, type)  (type*)_ArenaPush(arena, (count) * sizeof(type), alignof(type[1]), true)
#define ArenaPushString(arena, string)      (char*)_ArenaPush(arena, strlen(string), DefaultAlignment, true)

void ArenaClear  (Arena* arena);
void ArenaRelease(Arena* arena);