#pragma once

#include <Foundation/Containers/DynamicArray.h>
#include <Foundation/Memory/PageAllocator.h>
#include <Foundation/Memory/MemoryTracker.h>
#include <Foundation/Threading/Lock.h>
#include <Foundation/Threading/Mutex.h>

/// \brief This struct represents a 4k block of type T.
template <typename T>
struct ezDataBlock
{
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

  EZ_FORCE_INLINE T* PopBack()
  {
    EZ_ASSERT(m_uiCount > 0, "Block is empty");
    --m_uiCount;
    return m_pData + m_uiCount;
  }

  EZ_FORCE_INLINE bool IsEmpty() const
  {
    return m_uiCount == 0;
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
class EZ_FOUNDATION_DLL ezLargeBlockAllocator
{
public:
  ezLargeBlockAllocator(const char* szName, ezAllocatorBase* pParent, ezBitflags<ezMemoryTrackingFlags> flags = ezMemoryTrackingFlags::Default);
  ~ezLargeBlockAllocator();

  template <typename T>
  EZ_FORCE_INLINE ezDataBlock<T> AllocateBlock()
  {
    ezDataBlock<T> block(static_cast<T*>(Allocate(EZ_ALIGNMENT_OF(T))), 0);
    return block;
  }

  template <typename T>
  EZ_FORCE_INLINE void DeallocateBlock(ezDataBlock<T>& block)
  {
    Deallocate(block.m_pData);
    block.m_pData = NULL;
    block.m_uiCount = 0;
  }

  const char* GetName() const;
  
  EZ_FORCE_INLINE ezAllocatorId GetId() const
  {
    return m_Id;
  }

  const ezAllocatorBase::Stats& GetStats() const;
  
private:
  void* Allocate(size_t uiAlign);
  void Deallocate(void* ptr);

  ezAllocatorId m_Id;

  ezMutex m_mutex;
  ezThreadHandle m_threadHandle;

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

  ezDynamicArray<SuperBlock> m_superBlocks;
  ezDynamicArray<ezUInt32> m_freeBlocks;
};

