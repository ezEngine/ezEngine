#pragma once

#include <Foundation/Memory/LargeBlockAllocator.h>

template <typename T>
class ezBlockStorage
{
public:
  class Iterator
  {
  public:
    T& operator*() const;
    
    void Next();
    bool IsValid() const;
    
    void operator++();
    
  private:
    friend class ezBlockStorage<T>;

    Iterator(ezBlockStorage<T>& storage, ezUInt32 uiStartIndex, ezUInt32 uiCount);
    
    ezBlockStorage<T>& m_Storage;
    ezUInt32 m_uiCurrentIndex;
    ezUInt32 m_uiEndIndex;
  };

  ezBlockStorage(ezLargeBlockAllocator* pBlockAllocator, ezIAllocator* pAllocator); 
  ~ezBlockStorage();
  
  T* Create();
  void Delete(ezUInt32 uiIndex);
  
  Iterator GetIterator(ezUInt32 uiStartIndex = 0, ezUInt32 uiCount = ezInvalidIndex);
  
private:
  ezLargeBlockAllocator* m_pBlockAllocator;

  ezDynamicArray<ezDataBlock<T> > m_Blocks;
  ezUInt32 m_uiCount;
};

#include <Foundation/Memory/Implementation/BlockStorage_inl.h>
