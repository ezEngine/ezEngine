#pragma once

#include <Foundation/Containers/StaticRingBuffer.h>
#include <Foundation/Math/Math.h>
#include <Foundation/Threading/Lock.h>
#include <Foundation/Threading/Mutex.h>

class ezAllocPolicyGuarding
{
public:
  ezAllocPolicyGuarding(ezAllocator* pParent);
  EZ_ALWAYS_INLINE ~ezAllocPolicyGuarding() = default;

  void* Allocate(size_t uiSize, size_t uiAlign);
  void Deallocate(void* pPtr);

  EZ_ALWAYS_INLINE ezAllocator* GetParent() const { return nullptr; }

private:
  ezMutex m_Mutex;

  ezUInt32 m_uiPageSize;

  ezStaticRingBuffer<void*, (1 << 16)> m_AllocationsToFreeLater;
};
