#include "CashMemoryArena.h"
#include "CashSystem.h"

bool ArenaCommit(Arena* arena, const u64 size)
{
    VALIDATE_V(size, false);
    const u64 free_space = arena->reserved - arena->used;
    VALIDATE_V(free_space > size, false);

    const u64 new_used_size = arena->used + size;
    const u64 committed_available = arena->reserved - arena->committed;
    const u64 need_to_commit = new_used_size - committed_available;
    const u64 pages_to_commit = (u64)need_to_commit / (u64)SysGetOsPageSize() + 1;
    const u64 pages_in_bytes = pages_to_commit * SysGetOsPageSize();
    SysCommitMemory(arena->data, arena->committed + pages_in_bytes);
    arena->committed += pages_in_bytes;
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

void* _ArenaPush(Arena* arena, u64 size, u64 alignment, bool zero)
{
    VALIDATE_V(arena->data, nullptr);
    VALIDATE_V(arena->reserved, nullptr);
    const u64 free_space = arena->reserved - arena->used;
    VALIDATE_V(free_space > size, nullptr);

    u64 new_used = arena->used + size;
    const u64 committed_available = arena->reserved - arena->committed;
    if (new_used > committed_available)
    {
        ArenaCommit(arena, size - committed_available);
    }
    if (new_used > arena->reserved)
    {
        //reserve more memory?
        FAIL;
    }
    void* new_data_pointer = (void*)((u64)arena->data + arena->used);
    arena->used += size;
    return new_data_pointer;
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
