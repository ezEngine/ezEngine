#include <Foundation/FoundationPCH.h>
#include <Foundation/Memory/Allocator.h>

void* ezAllocator::Reallocate(void* pPtr, size_t uiCurrentSize, size_t uiNewSize, size_t uiAlign)
{
  void* pNewMem = Allocate(uiNewSize, uiAlign);
  memcpy(pNewMem, pPtr, uiCurrentSize);
  Deallocate(pPtr);
  return pNewMem;
}
