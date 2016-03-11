#pragma once

#include <Foundation/Containers/StaticRingBuffer.h>
#include <Foundation/Math/Math.h>
#include <Foundation/Threading/Lock.h>
#include <Foundation/Threading/Mutex.h>

namespace ezMemoryPolicies
{
  class ezGuardedAllocation
  {
  public:
    ezGuardedAllocation(ezAllocatorBase* pParent);
    EZ_FORCE_INLINE ~ezGuardedAllocation() { }

    void* Allocate(size_t uiSize, size_t uiAlign);
    void Deallocate(void* ptr);

    EZ_FORCE_INLINE ezAllocatorBase* GetParent() const { return nullptr; }

  private:
    ezMutex m_mutex;

    ezUInt32 m_uiPageSize;

    ezStaticRingBuffer<void*, (1 << 16)> m_AllocationsToFreeLater;
  };
}
