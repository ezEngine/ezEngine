#pragma once

#include <Foundation/Containers/DynamicArray.h>
#include <Foundation/Memory/PageAllocator.h>
#include <Foundation/Memory/MemoryTracker.h>
#include <Foundation/System/SystemInformation.h>
#include <Foundation/Threading/Lock.h>
#include <Foundation/Threading/Mutex.h>
#include <Foundation/Threading/ThreadUtils.h>

/// \brief This struct represents a block of type T, typically 4kb.
template <typename T, ezUInt32 SizeInBytes>
struct ezDataBlock
{
  EZ_DECLARE_POD_TYPE();

  enum 
  { 
    SIZE_IN_BYTES = SizeInBytes,
    CAPACITY = SIZE_IN_BYTES / sizeof(T) 
  };

  ezDataBlock(T* pData, ezUInt32 uiCount);
  
  T* ReserveBack();
  T* PopBack();
  
  bool IsEmpty() const;
  bool IsFull() const;
  
  T& operator[](ezUInt32 uiIndex) const;
  
  T* m_pData;
  ezUInt32 m_uiCount;
};

/// \brief A block allocator which can only allocates blocks of memory at once.
template <ezUInt32 BlockSizeInByte>
class ezLargeBlockAllocator
{
public:
  ezLargeBlockAllocator(const char* szName, ezAllocatorBase* pParent, ezBitflags<ezMemoryTrackingFlags> flags = ezMemoryTrackingFlags::Default);
  ~ezLargeBlockAllocator();

  template <typename T>
  ezDataBlock<T, BlockSizeInByte> AllocateBlock();
  
  template <typename T>
  void DeallocateBlock(ezDataBlock<T, BlockSizeInByte>& block);
  

  const char* GetName() const;
  
  ezAllocatorId GetId() const;
  
  const ezAllocatorBase::Stats& GetStats() const;
  
private:
  void* Allocate(size_t uiAlign);
  void Deallocate(void* ptr);

  ezAllocatorId m_Id;

  ezMutex m_mutex;
  ezThreadID m_ThreadID;

  struct SuperBlock
  {
    EZ_DECLARE_POD_TYPE();

    enum
    {
      NUM_BLOCKS = 16,
      SIZE_IN_BYTES = BlockSizeInByte * NUM_BLOCKS
    };

    void* m_pBasePtr;

    ezUInt32 m_uiUsedBlocks;
  };

  ezDynamicArray<SuperBlock> m_superBlocks;
  ezDynamicArray<ezUInt32> m_freeBlocks;
};

#include <Foundation/Memory/Implementation/LargeBlockAllocator_inl.h>
