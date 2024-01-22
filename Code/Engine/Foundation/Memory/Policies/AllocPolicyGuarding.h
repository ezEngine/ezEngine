#pragma once

#include <Foundation/Containers/StaticRingBuffer.h>
#include <Foundation/Math/Math.h>
#include <Foundation/Threading/Lock.h>
#include <Foundation/Threading/Mutex.h>

namespace ezMemoryPolicies
{
  class ezAllocPolicyGuarding
  {
  public:
    ezAllocPolicyGuarding(ezAllocatorBase* pParent);
    EZ_ALWAYS_INLINE ~ezAllocPolicyGuarding() = default;

    void* Allocate(size_t uiSize, size_t uiAlign);
    void Deallocate(void* pPtr);

    EZ_ALWAYS_INLINE ezAllocatorBase* GetParent() const { return nullptr; }

  private:
    ezMutex m_Mutex;

    ezUInt32 m_uiPageSize;

    ezStaticRingBuffer<void*, (1 << 16)> m_AllocationsToFreeLater;
  };
} // namespace ezMemoryPolicies
