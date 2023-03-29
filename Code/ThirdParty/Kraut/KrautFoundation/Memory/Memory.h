#ifndef AE_FOUNDATION_MEMORY_MEMORY_H
#define AE_FOUNDATION_MEMORY_MEMORY_H

#include "../Defines.h"

namespace AE_NS_FOUNDATION
{
  //! Provides static functions for working with raw memory.
  struct AE_FOUNDATION_DLL aeMemory
  {
    //! Copies uiBytes bytes from pSource to pDest
    static void Copy(const void* pSource, void* pDest, aeUInt32 uiBytes);

    //! Compares uiBytes of bytes from pSource1 and pSource1. Returns true, if all are equal.
    static bool Compare(const void* pSource1, const void* pSource2, aeUInt32 uiBytes);

    //! Sets the data at pDest to all zero.
    static void FillWithZeros(void* pDest, aeUInt32 uiBytes);
  };
} // namespace AE_NS_FOUNDATION

#include "Inline/Memory.inl"

#endif
