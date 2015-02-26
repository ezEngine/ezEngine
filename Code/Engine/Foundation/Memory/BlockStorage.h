#pragma once

#include <Foundation/Containers/Bitfield.h>
#include <Foundation/Memory/LargeBlockAllocator.h>

template <typename T, ezUInt32 BlockSizeInByte, bool CompactStorage>
class ezBlockStorage
{
public:
  class ConstIterator
  {
  public:
    const T& operator*() const;
    const T* operator->() const;

    operator const T*() const;
    
    void Next();
    bool IsValid() const;
    
    void operator++();
    
  protected:
    friend class ezBlockStorage<T, BlockSizeInByte, CompactStorage>;

    ConstIterator(const ezBlockStorage<T, BlockSizeInByte, CompactStorage>& storage, ezUInt32 uiStartIndex, ezUInt32 uiCount);
    
    T& CurrentElement() const;
    
    const ezBlockStorage<T, BlockSizeInByte, CompactStorage>& m_Storage;
    ezUInt32 m_uiCurrentIndex;
    ezUInt32 m_uiEndIndex;
  };

  class Iterator : public ConstIterator
  {
  public:
    T& operator*();
    T* operator->();

    operator T*();

  private:
    friend class ezBlockStorage<T, BlockSizeInByte, CompactStorage>;

    Iterator(const ezBlockStorage<T, BlockSizeInByte, CompactStorage>& storage, ezUInt32 uiStartIndex, ezUInt32 uiCount);
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

  ezBlockStorage(ezLargeBlockAllocator<BlockSizeInByte>* pBlockAllocator, ezAllocatorBase* pAllocator);
  ~ezBlockStorage();
  
  Entry Create();
  void Delete(Entry entry);
  void Delete(Entry entry, T*& out_pMovedObject);
  
  ezUInt32 GetCount() const;
  Iterator GetIterator(ezUInt32 uiStartIndex = 0, ezUInt32 uiCount = ezInvalidIndex);
  ConstIterator GetIterator(ezUInt32 uiStartIndex = 0, ezUInt32 uiCount = ezInvalidIndex) const;
  
private:
  ezLargeBlockAllocator<BlockSizeInByte>* m_pBlockAllocator;

  ezDynamicArray<ezDataBlock<T, BlockSizeInByte> > m_Blocks;
  ezUInt32 m_uiCount;

  ezUInt32 m_uiFreelistStart;

  ezDynamicBitfield m_UsedEntries;
};

#include <Foundation/Memory/Implementation/BlockStorage_inl.h>

