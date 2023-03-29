#ifndef AE_FOUNDATION_MEMORY_MEMORY_INL
#define AE_FOUNDATION_MEMORY_MEMORY_INL

#include <memory>
#include <cstring>

namespace AE_NS_FOUNDATION
{
  inline void aeMemory::Copy (const void* pSource, void* pDest, aeUInt32 uiBytes)
  {
    std::memcpy (pDest, pSource, uiBytes);
  }

  inline bool aeMemory::Compare (const void* pSource1, const void* pSource2, aeUInt32 uiBytes)
  {
    return (std::memcmp (pSource1, pSource2, uiBytes) == 0);
  }

  inline void aeMemory::FillWithZeros (void* pDest, aeUInt32 uiBytes)
  {
    std::memset (pDest, 0, uiBytes);
  }

}

#endif

