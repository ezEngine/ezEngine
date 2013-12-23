#pragma once

#include <Foundation/Threading/ThreadLocalPointer.h>

struct ezNullAllocatorWrapper
{
  EZ_FORCE_INLINE static ezIAllocator* GetAllocator()
  {
    EZ_REPORT_FAILURE("This method should never be called");
    return NULL;
  }
};

struct ezDefaultAllocatorWrapper
{
  EZ_FORCE_INLINE static ezIAllocator* GetAllocator()
  {
    return ezFoundation::GetDefaultAllocator();
  }
};

struct ezStaticAllocatorWrapper
{
  EZ_FORCE_INLINE static ezIAllocator* GetAllocator()
  {
    return ezFoundation::GetStaticAllocator();
  }
};

struct EZ_FOUNDATION_DLL ezLocalAllocatorWrapper
{
  EZ_FORCE_INLINE ezLocalAllocatorWrapper(ezIAllocator* pAllocator)
  {
    m_pAllocator = pAllocator;
  }

  EZ_FORCE_INLINE void Reset()
  {
    m_pAllocator = NULL;
  }

  EZ_FORCE_INLINE static ezIAllocator* GetAllocator()
  {
    return m_pAllocator;
  }

private:
  static ezThreadLocalPointer<ezIAllocator> m_pAllocator;
};

