#pragma once

#include <Foundation/Algorithm/Hashing.h>
#include <Foundation/Types/ArrayPtr.h>
#include <Foundation/Math/Math.h>
#include <Foundation/Memory/AllocatorWrapper.h>

/// \brief Implementation of a hashset.
///
/// The hashset stores values by using the hash as an index into the table.
/// This implementation uses linear-probing to resolve hash collisions which means all values are stored
/// in a linear array.
/// All insertion/erasure/lookup functions take O(1) time if the table does not need to be expanded,
/// which happens when the load gets greater than 60%.
/// The hash function can be customized by providing a Hasher helper class like ezHashHelper.

/// \see ezHashHelper
template <typename KeyType, typename Hasher>
class ezHashSetBase
{
public:
  /// \brief Const iterator.
  class ConstIterator
  {
  public:
    /// \brief Checks whether this iterator points to a valid element.
    bool IsValid() const; // [tested]

    /// \brief Checks whether the two iterators point to the same element.
    bool operator==(const typename ezHashSetBase<KeyType, Hasher>::ConstIterator& it2) const;

    /// \brief Checks whether the two iterators point to the same element.
    bool operator!=(const typename ezHashSetBase<KeyType, Hasher>::ConstIterator& it2) const;

    /// \brief Returns the 'key' of the element that this iterator points to.
    const KeyType& Key() const; // [tested]

    /// \brief Returns the 'key' of the element that this iterator points to.
    EZ_FORCE_INLINE const KeyType& operator*() { return Key(); }

    /// \brief Advances the iterator to the next element in the map. The iterator will not be valid anymore, if the end is reached.
    void Next(); // [tested]

    /// \brief Shorthand for 'Next'
    void operator++(); // [tested]

  protected:
    friend class ezHashSetBase<KeyType, Hasher>;

    explicit ConstIterator(const ezHashSetBase<KeyType, Hasher>& hashSet);
    void SetToBegin();
    void SetToEnd();

    const ezHashSetBase<KeyType, Hasher>& m_hashSet;
    ezUInt32 m_uiCurrentIndex; // current element index that this iterator points to.
    ezUInt32 m_uiCurrentCount; // current number of valid elements that this iterator has found so far.
  };

protected:
  /// \brief Creates an empty hashset. Does not allocate any data yet.
  ezHashSetBase(ezAllocatorBase* pAllocator); // [tested]

  /// \brief Creates a copy of the given hashset.
  ezHashSetBase(const ezHashSetBase<KeyType, Hasher>& rhs, ezAllocatorBase* pAllocator); // [tested]

  /// \brief Moves data from an existing hashtable into this one.
  ezHashSetBase(ezHashSetBase<KeyType, Hasher>&& rhs, ezAllocatorBase* pAllocator); // [tested]

  /// \brief Destructor.
  ~ezHashSetBase(); // [tested]

  /// \brief Copies the data from another hashset into this one.
  void operator= (const ezHashSetBase<KeyType, Hasher>& rhs); // [tested]

  /// \brief Moves data from an existing hashset into this one.
  void operator= (ezHashSetBase<KeyType, Hasher>&& rhs); // [tested]

public:

  /// \brief Compares this table to another table.
  bool operator== (const ezHashSetBase<KeyType, Hasher>& rhs) const; // [tested]

   /// \brief Compares this table to another table.
  bool operator!= (const ezHashSetBase<KeyType, Hasher>& rhs) const; // [tested]

  /// \brief Expands the hashset by over-allocating the internal storage so that the load factor is lower or equal to 60% when inserting the given number of entries.
  void Reserve(ezUInt32 uiCapacity); // [tested]

  /// \brief Tries to compact the hashset to avoid wasting memory.
  ///
  /// The resulting capacity is at least 'GetCount' (no elements get removed).
  /// Will deallocate all data, if the hashset is empty.
  void Compact(); // [tested]

  /// \brief Returns the number of active entries in the table.
  ezUInt32 GetCount() const; // [tested]

  /// \brief Returns true, if the hashset does not contain any elements.
  bool IsEmpty() const; // [tested]

  /// \brief Clears the table.
  void Clear(); // [tested]

  /// \brief Inserts the key. Returns whether the key was already existing.
  template <typename CompatibleKeyType>
  bool Insert(CompatibleKeyType&& key); // [tested]

  /// \brief Removes the entry with the given key. Returns if an entry was removed.
  bool Remove(const KeyType& key); // [tested]

  /// \brief Returns if an entry with given key exists in the table.
  bool Contains(const KeyType& key) const; // [tested]

  /// \brief Returns a constant Iterator to the very first element.
  ConstIterator GetIterator() const; // [tested]

  /// \brief Returns a constant Iterator to the first element that is not part of the hashset. Needed to implement range based for loop support.
  ConstIterator GetEndIterator() const;

  /// \brief Returns the allocator that is used by this instance.
  ezAllocatorBase* GetAllocator() const;

  /// \brief Returns the amount of bytes that are currently allocated on the heap.
  ezUInt64 GetHeapMemoryUsage() const; // [tested]

private:

  KeyType* m_pEntries;
  ezUInt32* m_pEntryFlags;

  ezUInt32 m_uiCount;
  ezUInt32 m_uiCapacity;

  ezAllocatorBase* m_pAllocator;

  enum
  {
    FREE_ENTRY = 0,
    VALID_ENTRY = 1,
    DELETED_ENTRY = 2,
    FLAGS_MASK = 3,
    CAPACITY_ALIGNMENT = 32
  };

  void SetCapacity(ezUInt32 uiCapacity);
  ezUInt32 FindEntry(const KeyType& key) const;
  ezUInt32 FindEntry(ezUInt32 uiHash, const KeyType& key) const;

  ezUInt32 GetFlagsCapacity() const;
  ezUInt32 GetFlags(ezUInt32* pFlags, ezUInt32 uiEntryIndex) const;
  void SetFlags(ezUInt32 uiEntryIndex, ezUInt32 uiFlags);

  bool IsFreeEntry(ezUInt32 uiEntryIndex) const;
  bool IsValidEntry(ezUInt32 uiEntryIndex) const;
  bool IsDeletedEntry(ezUInt32 uiEntryIndex) const;

  void MarkEntryAsFree(ezUInt32 uiEntryIndex);
  void MarkEntryAsValid(ezUInt32 uiEntryIndex);
  void MarkEntryAsDeleted(ezUInt32 uiEntryIndex);
};

/// \brief \see ezHashSetBase
template <typename KeyType, typename Hasher = ezHashHelper<KeyType>, typename AllocatorWrapper = ezDefaultAllocatorWrapper>
class ezHashSet : public ezHashSetBase<KeyType, Hasher>
{
public:
  ezHashSet();
  ezHashSet(ezAllocatorBase* pAllocator);

  ezHashSet(const ezHashSet<KeyType, Hasher, AllocatorWrapper>& other);
  ezHashSet(const ezHashSetBase<KeyType, Hasher>& other);

  ezHashSet(ezHashSet<KeyType, Hasher, AllocatorWrapper>&& other);
  ezHashSet(ezHashSetBase<KeyType, Hasher>&& other);

  void operator=(const ezHashSet<KeyType, Hasher, AllocatorWrapper>& rhs);
  void operator=(const ezHashSetBase<KeyType, Hasher>& rhs);

  void operator=(ezHashSet<KeyType, Hasher, AllocatorWrapper>&& rhs);
  void operator=(ezHashSetBase<KeyType, Hasher>&& rhs);
};

template <typename KeyType, typename Hasher>
typename ezHashSetBase<KeyType, Hasher>::ConstIterator begin(const ezHashSetBase<KeyType, Hasher>& set) { return set.GetIterator(); }

template <typename KeyType, typename Hasher>
typename ezHashSetBase<KeyType, Hasher>::ConstIterator cbegin(const ezHashSetBase<KeyType, Hasher>& set) { return set.GetIterator(); }

template <typename KeyType, typename Hasher>
typename ezHashSetBase<KeyType, Hasher>::ConstIterator end(const ezHashSetBase<KeyType, Hasher>& set) { return set.GetEndIterator(); }

template <typename KeyType, typename Hasher>
typename ezHashSetBase<KeyType, Hasher>::ConstIterator cend(const ezHashSetBase<KeyType, Hasher>& set) { return set.GetEndIterator(); }

#include <Foundation/Containers/Implementation/HashSet_inl.h>

