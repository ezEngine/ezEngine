#pragma once

#include <Foundation/Containers/DynamicArray.h>
#include <Foundation/Memory/Policies/PageAllocation.h>
#include <Foundation/Threading/Lock.h>
#include <Foundation/Threading/Mutex.h>

/// \brief This struct represents a 4k block of type T.
template <typename T>
struct ezDataBlock
{
public:
  EZ_DECLARE_POD_TYPE();

  enum 
  { 
    SIZE_IN_BYTES = 4096, 
    CAPACITY = SIZE_IN_BYTES / sizeof(T) 
  };

  EZ_FORCE_INLINE ezDataBlock(T* pData, ezUInt32 uiCount)
  {
    m_pData = pData;
    m_uiCount = uiCount;
  }

  EZ_FORCE_INLINE T* ReserveBack()
  {
    EZ_ASSERT(m_uiCount < CAPACITY, "Block is full.");
    return m_pData + m_uiCount++;
  }

  EZ_FORCE_INLINE bool IsFull() const
  {
    return m_uiCount == CAPACITY;
  }

  EZ_FORCE_INLINE T& operator[](ezUInt32 uiIndex) const
  {
    EZ_ASSERT(uiIndex < m_uiCount, "Out of bounds access. Data block has %i elements, trying to access element at index %i.", m_uiCount, uiIndex);
    return m_pData[uiIndex];
  }

  T* m_pData;
  ezUInt32 m_uiCount;
};

/// \brief A block allocator which can only allocates 4k blocks of memory at once.
class EZ_FOUNDATION_DLL ezLargeBlockAllocator : public ezIAllocator
{
public:
  ezLargeBlockAllocator(const char* szName, ezIAllocator* pParent);
  ~ezLargeBlockAllocator();

  template <typename T>
  ezDataBlock<T> AllocateBlock()
  {
    ezDataBlock<T> block(static_cast<T*>(Allocate(ezDataBlock<T>::SIZE_IN_BYTES, EZ_ALIGNMENT_OF(T))), 0);
    return block;
  }

  template <typename T>
  void DeallocateBlock(ezDataBlock<T>& block)
  {
    Deallocate(block.m_pData);
    block.m_pData = NULL;
    block.m_uiCount = 0;
  }
  
  size_t AllocatedSize(const void* ptr) EZ_OVERRIDE;
  size_t UsedMemorySize(const void* ptr) EZ_OVERRIDE;
  void GetStats(Stats& stats) const EZ_OVERRIDE;

private:
  void* Allocate(size_t uiSize, size_t uiAlign) EZ_OVERRIDE;
  void Deallocate(void* ptr) EZ_OVERRIDE;

  ezMutex m_mutex;
  ezThreadHandle m_threadHandle;

  ezMemoryPolicies::ezPageAllocation m_allocator;

  struct SuperBlock
  {
    EZ_DECLARE_POD_TYPE();

    enum
    {
      NUM_BLOCKS = 16,
      SIZE_IN_BYTES = ezDataBlock<int>::SIZE_IN_BYTES * NUM_BLOCKS
    };

    void* m_pBasePtr;

    ezUInt32 m_uiUsedBlocks;
  };

  ezUInt64 m_uiNumAllocations;
  ezUInt64 m_uiNumDeallocations;

  ezDynamicArray<SuperBlock> m_superBlocks;
  ezDynamicArray<ezUInt32> m_freeBlocks;
};
