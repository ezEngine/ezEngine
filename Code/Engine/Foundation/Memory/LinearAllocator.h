#pragma once

#include <Foundation/Containers/DynamicArray.h>
#include <Foundation/Containers/HashTable.h>
#include <Foundation/Memory/AllocatorWithPolicy.h>
#include <Foundation/Memory/Policies/AllocPolicyLinear.h>
#include <Foundation/Threading/Lock.h>
#include <Foundation/Threading/Mutex.h>

template <ezAllocatorTrackingMode TrackingMode = ezAllocatorTrackingMode::Default, bool OverwriteMemoryOnReset = false>
class ezLinearAllocator : public ezAllocatorWithPolicy<ezAllocPolicyLinear<OverwriteMemoryOnReset>, TrackingMode>
{
  using PolicyStack = ezAllocPolicyLinear<OverwriteMemoryOnReset>;

public:
  ezLinearAllocator(ezStringView sName, ezAllocator* pParent);
  ~ezLinearAllocator();

  virtual void* Allocate(size_t uiSize, size_t uiAlign, ezMemoryUtils::DestructorFunction destructorFunc) override;
  virtual void Deallocate(void* pPtr) override;

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

#include <Foundation/Memory/Implementation/LinearAllocator_inl.h>
