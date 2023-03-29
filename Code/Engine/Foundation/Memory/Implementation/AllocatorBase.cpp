#include <Foundation/FoundationPCH.h>

void* ezAllocatorBase::Reallocate(void* pPtr, size_t uiCurrentSize, size_t uiNewSize, size_t uiAlign)
{
  void* pNewMem = Allocate(uiNewSize, uiAlign);
  memcpy(pNewMem, pPtr, uiCurrentSize);
  Deallocate(pPtr);
  return pNewMem;
}



EZ_STATICLINK_FILE(Foundation, Foundation_Memory_Implementation_AllocatorBase);
