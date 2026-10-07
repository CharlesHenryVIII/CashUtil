#include "CashMemoryArena.h"
#include "CashSystem.h"
#include "CashConsole.h"

bool ArenaCommit(Arena* arena, const u64 size)
{
    VALIDATE_V(size, false);
    VALIDATE_V(arena->used <= arena->committed, false);
    const u64 reserved_avail = arena->reserved - arena->used;
    VALIDATE_V(reserved_avail > size, false);
    const u64 committed_unused = arena->committed - arena->used;
    const u64 committed_avail = arena->reserved - arena->committed;

    const u64 need_to_commit = size - committed_unused;
    const u64 pages_to_commit = need_to_commit / SysGetOsPageSize() + 1;
    const u64 pages_in_bytes = pages_to_commit * SysGetOsPageSize();
    SysCommitMemory(arena->data, arena->committed + pages_in_bytes);
    arena->committed += pages_in_bytes;
    ASSERT(arena->committed % 4096 == 0);
    return true;
}

Arena ArenaAlloc(const u64 reserve_size, const u64 commit_size)
{
    Arena arena = {};
    arena.data = SysReserveMemory(reserve_size);
    arena.reserved = reserve_size;
    if (commit_size)
        ArenaCommit(&arena, commit_size);
    return arena;
}

void ArenaFree(Arena* arena)
{
    SysFreeMemory(arena->data, arena->reserved);
    *arena = {};
}

void* ArenaPush(Arena* arena, u64 size, u64 alignment, bool zero)
{
    VALIDATE_V(arena->data, nullptr);
    VALIDATE_V(arena->reserved, nullptr);
    const u64 reserved_free = arena->reserved - arena->used;
    VALIDATE_V(reserved_free > size, nullptr);
    const u64 committed_free= arena->committed - arena->used;

    if (size > committed_free)
    {
        VALIDATE_MV(ArenaCommit(arena, size), nullptr, LogLevel_Error, "%s: needed: %i", "Failed to commit memory for Arena", size);
    }
    void* new_data_pointer = (void*)((u64)arena->data + arena->used);
    arena->used += size;
    return new_data_pointer;
}

[[nodiscard]] char* ArenaPushArgs(Arena* arena, const char* fmt, va_list args)
{
    char temp[4096];
    const i32 len = SYS_VSNPRINTF(temp, sizeof(temp), fmt, args);
    if (len < 0)
        return nullptr;

    char* r = (char*)ArenaPush(arena, (u64)len + 1, CASH_DEFAULT_ALIGNMENT, false);
    VALIDATE_MV(r, nullptr, LogLevel_Error, "nullptr return from _ArenaPush()!");
    const i32 size = Min<i32>(len, sizeof(temp) - 1);
    memcpy(r, temp, (size_t)size + 1);
    return r;
}

[[nodiscard]] char* ArenaPush(Arena* arena, const char* fmt, ...)
{
    va_list args;
    va_start(args, fmt);
    char* r = ArenaPushArgs(arena, fmt, args);
    va_end(args);
    return r;
}

[[nodiscard]] char* ArenaPush(Arena* arena, const char* string)
{
    const u64 len = strlen(string);
    char* r = (char*)ArenaPush(arena, len + 1, CASH_DEFAULT_ALIGNMENT, false);
    memcpy(r, string, len + 1);
    return r;
}

void ArenaClear(Arena* arena)
{
    memset(arena->data, 0, arena->committed);
    arena->used = 0;
}

void ArenaRelease(Arena* arena)
{
    SysFreeMemory(arena->data, arena->reserved);
    *arena = {};
}
