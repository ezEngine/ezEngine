#pragma once

#include <Foundation/Containers/DynamicArray.h>
#include <Foundation/Containers/HashTable.h>
#include <Foundation/Memory/AllocatorWithPolicy.h>
#include <Foundation/Memory/Policies/AllocPolicyLinear.h>
#include <Foundation/Threading/Lock.h>
#include <Foundation/Threading/Mutex.h>

/// \brief Linear allocator that allocates memory sequentially.
///
/// This allocator allocates memory by simply advancing a pointer within pre-allocated chunks.
/// Individual deallocations are not supported - the entire allocator must be reset at once.
/// Very efficient for short lived allocations.
///
/// Template parameters:
/// - TrackingMode: Controls allocation tracking and debugging features
/// - OverwriteMemoryOnReset: If true, fills memory with debug pattern on reset (debug builds only)
///
/// Performance characteristics:
/// - Allocation: O(1) - just pointer arithmetic
/// - Deallocation: Not supported individually
/// - Reset: O(1) - just resets pointer
/// - Memory overhead: Minimal
///
/// Use when:
/// - Temporary allocations with predictable lifetime
/// - Stack-like allocation pattern
/// - High allocation frequency (parsing, temporary buffers)
/// - Frame-based or scope-based memory management
template <ezAllocatorTrackingMode TrackingMode = ezAllocatorTrackingMode::Default, bool OverwriteMemoryOnReset = false>
class ezLinearAllocator : public ezAllocatorWithPolicy<ezAllocPolicyLinear<OverwriteMemoryOnReset>, TrackingMode>
{
  using SUPER = ezAllocatorWithPolicy<ezAllocPolicyLinear<OverwriteMemoryOnReset>, TrackingMode>;
  using PolicyLinear = ezAllocPolicyLinear<OverwriteMemoryOnReset>;

public:
  ezLinearAllocator(ezStringView sName, ezAllocator* pParent, ezUInt32 uiInitialSize);
  ~ezLinearAllocator();

  virtual void* Allocate(size_t uiSize, size_t uiAlign, ezMemoryUtils::DestructorFunction destructorFunc) override;
  virtual void Deallocate(void* pPtr) override;

  /// \brief Resets the allocator, freeing all memory and calling destructors.
  ///
  /// This resets the allocation pointer to the beginning and calls destructors for all
  /// objects that were allocated with destructor functions. After reset, all previously
  /// allocated pointers become invalid and must not be used.
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
