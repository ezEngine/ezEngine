#pragma once

#include <Foundation/Containers/Bitfield.h>
#include <Foundation/Memory/LargeBlockAllocator.h>

/// \brief Defines storage strategies for block-based container management.
struct ezBlockStorageType
{
  enum Enum
  {
    Compact, ///< Maintains elements in contiguous memory by moving last element to fill gaps
    FreeList ///< Uses a free list to track available slots, preserving element positions
  };
};

/// \brief High-performance container for objects with pluggable storage strategies.
///
/// This container manages objects in blocks of memory, using different strategies for handling
/// gaps when objects are removed. It's designed for scenarios where you need fast allocation
/// and deallocation of many objects, with the choice between compact memory layout or stable
/// object addressing.
///
/// Storage strategies:
/// - Compact: Moves the last element to fill gaps when objects are deleted, maintaining
///   contiguous memory but invalidating iterators and pointers to moved objects
/// - FreeList: Uses a free list to reuse deleted slots, preserving object positions but
///   potentially creating memory fragmentation
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

  ezBlockStorage(ezLargeBlockAllocator<BlockSizeInByte>* pBlockAllocator, ezAllocator* pAllocator);
  ~ezBlockStorage();

  /// \brief Removes all objects and deallocates all blocks.
  void Clear();

  /// \brief Creates a new object and returns a pointer to it.
  ///
  /// The object is default-constructed. Returns nullptr if allocation fails.
  T* Create();

  /// \brief Deletes the specified object.
  void Delete(T* pObject);

  /// \brief Deletes the specified object and reports any moved object.
  ///
  /// For Compact storage, if another object is moved to fill the gap,
  /// out_pMovedObject will point to the moved object's new location.
  /// For FreeList storage, out_pMovedObject is always set to nullptr.
  void Delete(T* pObject, T*& out_pMovedObject);

  /// \brief Returns the total number of objects currently stored.
  ezUInt32 GetCount() const;

  /// \brief Returns an iterator for traversing objects in a specified range.
  Iterator GetIterator(ezUInt32 uiStartIndex = 0, ezUInt32 uiCount = ezInvalidIndex);

  /// \brief Returns a const iterator for traversing objects in a specified range.
  ConstIterator GetIterator(ezUInt32 uiStartIndex = 0, ezUInt32 uiCount = ezInvalidIndex) const;

private:
  void Delete(T* pObject, T*& out_pMovedObject, ezTraitInt<ezBlockStorageType::Compact>);
  void Delete(T* pObject, T*& out_pMovedObject, ezTraitInt<ezBlockStorageType::FreeList>);

  ezLargeBlockAllocator<BlockSizeInByte>* m_pBlockAllocator;

  ezDynamicArray<ezDataBlock<T, BlockSizeInByte>> m_Blocks;
  ezUInt32 m_uiCount = 0;

  ezUInt32 m_uiFreelistStart = ezInvalidIndex;

  ezDynamicBitfield m_UsedEntries;
};

#include <Foundation/Memory/Implementation/BlockStorage_inl.h>
