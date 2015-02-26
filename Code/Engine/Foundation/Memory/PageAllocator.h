#pragma once

#include <Foundation/Basics.h>

/// \brief This helper class can reserve and allocate whole memory pages.
class EZ_FOUNDATION_DLL ezPageAllocator
{
public:
  static void* AllocatePage(size_t uiSize);
  static void DeallocatePage(void* ptr);
};
