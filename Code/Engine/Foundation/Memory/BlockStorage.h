#pragma once

#include <Foundation/Containers/Bitfield.h>
#include <Foundation/Memory/LargeBlockAllocator.h>

struct ezBlockStorageType
{
  enum Enum
  {
    Compact,
    FreeList
  };
};

template <typename T, ezUInt32 BlockSizeInByte, ezBlockStorageType::Enum StorageType>
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
    friend class ezBlockStorage<T, BlockSizeInByte, StorageType>;

    ConstIterator(const ezBlockStorage<T, BlockSizeInByte, StorageType>& storage, ezUInt32 uiStartIndex, ezUInt32 uiCount);

    T& CurrentElement() const;

    const ezBlockStorage<T, BlockSizeInByte, StorageType>& m_Storage;
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
    friend class ezBlockStorage<T, BlockSizeInByte, StorageType>;

    Iterator(const ezBlockStorage<T, BlockSizeInByte, StorageType>& storage, ezUInt32 uiStartIndex, ezUInt32 uiCount);
  };

  ezBlockStorage(ezLargeBlockAllocator<BlockSizeInByte>* pBlockAllocator, ezAllocatorBase* pAllocator);
  ~ezBlockStorage();

  T* Create();
  void Delete(T* pObject);
  void Delete(T* pObject, T*& out_pMovedObject);

  ezUInt32 GetCount() const;
  Iterator GetIterator(ezUInt32 uiStartIndex = 0, ezUInt32 uiCount = ezInvalidIndex);
  ConstIterator GetIterator(ezUInt32 uiStartIndex = 0, ezUInt32 uiCount = ezInvalidIndex) const;

private:
  void Delete(T* pObject, T*& out_pMovedObject, ezTraitInt<ezBlockStorageType::Compact>);
  void Delete(T* pObject, T*& out_pMovedObject, ezTraitInt<ezBlockStorageType::FreeList>);

  ezLargeBlockAllocator<BlockSizeInByte>* m_pBlockAllocator;

  ezDynamicArray<ezDataBlock<T, BlockSizeInByte> > m_Blocks;
  ezUInt32 m_uiCount;

  ezUInt32 m_uiFreelistStart;

  ezDynamicBitfield m_UsedEntries;
};

#include <Foundation/Memory/Implementation/BlockStorage_inl.h>

