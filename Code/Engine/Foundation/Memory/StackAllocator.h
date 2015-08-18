#pragma once

#include <Foundation/Containers/DynamicArray.h>
#include <Foundation/Containers/HashTable.h>
#include <Foundation/Memory/Allocator.h>
#include <Foundation/Memory/Policies/StackAllocation.h>
#include <Foundation/Threading/Lock.h>
#include <Foundation/Threading/Mutex.h>

template <ezUInt32 TrackingFlags = ezMemoryTrackingFlags::Default>
class ezStackAllocator : public ezAllocator<ezMemoryPolicies::ezStackAllocation, TrackingFlags>
{
public:
  ezStackAllocator(const char* szName, ezAllocatorBase* pParent);

  virtual void* Allocate(size_t uiSize, size_t uiAlign, ezMemoryUtils::DestructorFunction destructorFunc) override;
  virtual void Deallocate(void* ptr) override;

  /// \brief
  ///   Resets the allocator freeing all memory.
  void Reset();

private:
  struct DestructData
  {
    EZ_DECLARE_POD_TYPE();

    ezMemoryUtils::DestructorFunction m_Func;
    void* m_Ptr;    
  };

  ezMutex m_Mutex;
  ezDynamicArray<DestructData> m_DestructData;
  ezHashTable<void*, ezUInt32> m_PtrToDestructDataIndexTable;
};

#include <Foundation/Memory/Implementation/StackAllocator_inl.h>
