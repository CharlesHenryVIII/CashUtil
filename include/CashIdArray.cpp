#include "CashIdArray.h"
#include "CashSystem.h"

void* _IdArrayReserve(u64 bytes)
{
    return SysReserveMemory(bytes);
}
void _IdArrayCommit(void* p, u64 bytes)
{
    SysCommitMemory(p, bytes);
}
bool _IdArrayFree(void* p, u64 bytes)
{
    return SysFreeMemory(p, bytes);
}
