#include <Foundation/PCH.h>
#include <Foundation/Memory/AllocatorBase.h>

void* ezAllocatorBase::Reallocate(void* ptr, size_t uiCurrentSize, size_t uiNewSize, size_t uiAlign)
{
    void * pNewMem = Allocate(uiNewSize, uiAlign);
    memcpy(pNewMem, ptr, uiCurrentSize);
    Deallocate(ptr);
    return pNewMem;
}



EZ_STATICLINK_FILE(Foundation, Foundation_Memory_Implementation_AllocatorBase);

