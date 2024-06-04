#pragma once

#include <Foundation/Memory/Allocator.h>

struct ezNullAllocatorWrapper
{
  EZ_FORCE_INLINE static ezAllocator* GetAllocator()
  {
    EZ_REPORT_FAILURE("This method should never be called");
    return nullptr;
  }
};

struct ezDefaultAllocatorWrapper
{
  EZ_ALWAYS_INLINE static ezAllocator* GetAllocator() { return ezFoundation::GetDefaultAllocator(); }
};

struct ezStaticsAllocatorWrapper
{
  EZ_ALWAYS_INLINE static ezAllocator* GetAllocator() { return ezFoundation::GetStaticsAllocator(); }
};

struct ezAlignedAllocatorWrapper
{
  EZ_ALWAYS_INLINE static ezAllocator* GetAllocator() { return ezFoundation::GetAlignedAllocator(); }
};

struct EZ_FOUNDATION_DLL ezLocalAllocatorWrapper
{
  ezLocalAllocatorWrapper(ezAllocator* pAllocator);

  void Reset();

  static ezAllocator* GetAllocator();
};
