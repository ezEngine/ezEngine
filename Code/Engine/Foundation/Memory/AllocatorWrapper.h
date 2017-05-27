#pragma once

#include <Foundation/Threading/ThreadLocalPointer.h>

struct ezNullAllocatorWrapper
{
  EZ_FORCE_INLINE static ezAllocatorBase* GetAllocator()
  {
    EZ_REPORT_FAILURE("This method should never be called");
    return nullptr;
  }
};

struct ezDefaultAllocatorWrapper
{
  EZ_ALWAYS_INLINE static ezAllocatorBase* GetAllocator()
  {
    return ezFoundation::GetDefaultAllocator();
  }
};

struct ezStaticAllocatorWrapper
{
  EZ_ALWAYS_INLINE static ezAllocatorBase* GetAllocator()
  {
    return ezFoundation::GetStaticAllocator();
  }
};

struct ezAlignedAllocatorWrapper
{
  EZ_ALWAYS_INLINE static ezAllocatorBase* GetAllocator()
  {
    return ezFoundation::GetAlignedAllocator();
  }
};

struct EZ_FOUNDATION_DLL ezLocalAllocatorWrapper
{
  EZ_ALWAYS_INLINE ezLocalAllocatorWrapper(ezAllocatorBase* pAllocator)
  {
    m_pAllocator = pAllocator;
  }

  EZ_ALWAYS_INLINE void Reset()
  {
    m_pAllocator = nullptr;
  }

  EZ_ALWAYS_INLINE static ezAllocatorBase* GetAllocator()
  {
    return m_pAllocator;
  }

private:
  static ezThreadLocalPointer<ezAllocatorBase> m_pAllocator;
};

