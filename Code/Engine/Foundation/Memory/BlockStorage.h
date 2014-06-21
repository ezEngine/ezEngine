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
    T* operator->() const;

    operator T*() const;
    
    void Next();
    bool IsValid() const;
    
    void operator++();
    
  private:
    friend class ezBlockStorage<T>;

    Iterator(const ezBlockStorage<T>& storage, ezUInt32 uiStartIndex, ezUInt32 uiCount);
    
    const ezBlockStorage<T>& m_Storage;
    ezUInt32 m_uiCurrentIndex;
    ezUInt32 m_uiEndIndex;
  };

  struct Entry
  {
    EZ_DECLARE_POD_TYPE();

    T* m_Ptr;
    ezUInt32 m_uiIndex;

    bool operator<(const Entry& rhs) const;
    bool operator>(const Entry& rhs) const;
    bool operator==(const Entry& rhs) const;
  };

  ezBlockStorage(ezLargeBlockAllocator* pBlockAllocator, ezAllocatorBase* pAllocator); 
  ~ezBlockStorage();
  
  Entry Create();
  void Delete(Entry entry);
  void Delete(Entry entry, T*& out_pMovedObject);
  
  ezUInt32 GetCount() const;
  Iterator GetIterator(ezUInt32 uiStartIndex = 0, ezUInt32 uiCount = ezInvalidIndex) const;
  
private:
  ezLargeBlockAllocator* m_pBlockAllocator;

  ezDynamicArray<ezDataBlock<T> > m_Blocks;
  ezUInt32 m_uiCount;
};

#include <Foundation/Memory/Implementation/BlockStorage_inl.h>

