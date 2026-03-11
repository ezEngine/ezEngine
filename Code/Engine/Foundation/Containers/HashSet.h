#pragma once

#include <Foundation/Algorithm/HashingUtils.h>
#include <Foundation/Math/Math.h>
#include <Foundation/Memory/AllocatorWrapper.h>

/// \brief Implementation of a hashset.
///
/// The hashset stores values by using the hash as an index into the table.
/// This implementation uses linear-probing to resolve hash collisions which means all values are stored
/// in a linear array. Automatic resizing maintains a load factor below 60% for optimal performance.
///
/// Performance characteristics:
/// - Average case: O(1) - insertion, erasure, lookup
/// - Worst case: O(n) - when all keys hash to the same location (very rare with good hash functions)
/// - Resizing: O(n) - occurs when load factor exceeds 60%, amortized cost is still O(1) per operation
/// - Memory usage: More memory efficient than tree-based containers, ~1.67x element storage
/// - Iteration: O(n) in hash order (not sorted)
/// - Set operations (union, intersection, difference): O(n + m) average case
///
/// Use when:
/// - Fast lookup/insertion/removal is the primary concern
/// - You don't need sorted iteration
/// - Memory efficiency is important
/// - You have a good hash function for your key type
///
/// Consider ezSet instead when:
/// - You need sorted iteration
/// - You need stable element addresses (no reallocation)
/// - Predictable O(log n) performance is more important than average O(1)
///
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
    bool operator==(const typename ezHashSetBase<KeyType, Hasher>::ConstIterator& rhs) const;

    EZ_ADD_DEFAULT_OPERATOR_NOTEQUAL(const typename ezHashSetBase<KeyType, Hasher>::ConstIterator&);

    /// \brief Returns the 'key' of the element that this iterator points to.
    const KeyType& Key() const; // [tested]

    /// \brief Returns the 'key' of the element that this iterator points to.
    EZ_ALWAYS_INLINE const KeyType& operator*() const { return Key(); } // [tested]

    /// \brief Advances the iterator to the next element in the map. The iterator will not be valid anymore, if the end is reached.
    void Next(); // [tested]

    /// \brief Shorthand for 'Next'
    void operator++(); // [tested]

  protected:
    friend class ezHashSetBase<KeyType, Hasher>;

    explicit ConstIterator(const ezHashSetBase<KeyType, Hasher>& hashSet);
    void SetToBegin();
    void SetToEnd();

    const ezHashSetBase<KeyType, Hasher>* m_pHashSet = nullptr;
    ezUInt32 m_uiCurrentIndex = 0; // current element index that this iterator points to.
    ezUInt32 m_uiCurrentCount = 0; // current number of valid elements that this iterator has found so far.
  };

protected:
  /// \brief Creates an empty hashset. Does not allocate any data yet.
  explicit ezHashSetBase(ezAllocator* pAllocator); // [tested]

  /// \brief Creates a copy of the given hashset.
  ezHashSetBase(const ezHashSetBase<KeyType, Hasher>& rhs, ezAllocator* pAllocator); // [tested]

  /// \brief Moves data from an existing hashtable into this one.
  ezHashSetBase(ezHashSetBase<KeyType, Hasher>&& rhs, ezAllocator* pAllocator); // [tested]

  /// \brief Destructor.
  ~ezHashSetBase(); // [tested]

  /// \brief Copies the data from another hashset into this one.
  void operator=(const ezHashSetBase<KeyType, Hasher>& rhs); // [tested]

  /// \brief Moves data from an existing hashset into this one.
  void operator=(ezHashSetBase<KeyType, Hasher>&& rhs); // [tested]

public:
  /// \brief Compares this table to another table.
  bool operator==(const ezHashSetBase<KeyType, Hasher>& rhs) const; // [tested]
  EZ_ADD_DEFAULT_OPERATOR_NOTEQUAL(const ezHashSetBase<KeyType, Hasher>&);

  /// \brief Expands the hashset by over-allocating the internal storage so that the load factor is lower or equal to 60% when inserting the
  /// given number of entries.
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
  template <typename CompatibleKeyType>
  bool Remove(const CompatibleKeyType& key); // [tested]

  /// \brief Erases the key at the given Iterator. Returns an iterator to the element after the given iterator.
  ConstIterator Remove(const ConstIterator& pos); // [tested]

  /// \brief Returns if an entry with given key exists in the table.
  template <typename CompatibleKeyType>
  bool Contains(const CompatibleKeyType& key) const; // [tested]

  /// \brief Checks whether all keys of the given set are in the container.
  bool ContainsSet(const ezHashSetBase<KeyType, Hasher>& operand) const; // [tested]

  /// \brief Makes this set the union of itself and the operand.
  void Union(const ezHashSetBase<KeyType, Hasher>& operand); // [tested]

  /// \brief Makes this set the difference of itself and the operand, i.e. subtracts operand.
  void Difference(const ezHashSetBase<KeyType, Hasher>& operand); // [tested]

  /// \brief Makes this set the intersection of itself and the operand.
  void Intersection(const ezHashSetBase<KeyType, Hasher>& operand); // [tested]

  /// \brief Returns a constant Iterator to the very first element.
  ConstIterator GetIterator() const; // [tested]

  /// \brief Returns a constant Iterator to the first element that is not part of the hashset. Needed to implement range based for loop
  /// support.
  ConstIterator GetEndIterator() const;

  /// \brief Returns the allocator that is used by this instance.
  ezAllocator* GetAllocator() const;

  /// \brief Returns the amount of bytes that are currently allocated on the heap.
  ezUInt64 GetHeapMemoryUsage() const; // [tested]

  /// \brief Swaps this map with the other one.
  void Swap(ezHashSetBase<KeyType, Hasher>& other); // [tested]

  /// \brief Searches for key, returns a ConstIterator to it or an invalid iterator, if no such key is found. O(1) operation.
  template <typename CompatibleKeyType>
  ConstIterator Find(const CompatibleKeyType& key) const;

private:
  KeyType* m_pEntries;
  ezUInt32* m_pEntryFlags;

  ezUInt32 m_uiCount;
  ezUInt32 m_uiCapacity;

  ezAllocator* m_pAllocator;

  enum
  {
    FREE_ENTRY = 0,
    VALID_ENTRY = 1,
    DELETED_ENTRY = 2,
    FLAGS_MASK = 3,
    CAPACITY_ALIGNMENT = 32
  };

  void SetCapacity(ezUInt32 uiCapacity);

  void RemoveInternal(ezUInt32 uiIndex);

  template <typename CompatibleKeyType>
  ezUInt32 FindEntry(const CompatibleKeyType& key) const;

  template <typename CompatibleKeyType>
  ezUInt32 FindEntry(ezUInt32 uiHash, const CompatibleKeyType& key) const;

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
  explicit ezHashSet(ezAllocator* pAllocator);

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
typename ezHashSetBase<KeyType, Hasher>::ConstIterator begin(const ezHashSetBase<KeyType, Hasher>& set)
{
  return set.GetIterator();
}

template <typename KeyType, typename Hasher>
typename ezHashSetBase<KeyType, Hasher>::ConstIterator cbegin(const ezHashSetBase<KeyType, Hasher>& set)
{
  return set.GetIterator();
}

template <typename KeyType, typename Hasher>
typename ezHashSetBase<KeyType, Hasher>::ConstIterator end(const ezHashSetBase<KeyType, Hasher>& set)
{
  return set.GetEndIterator();
}

template <typename KeyType, typename Hasher>
typename ezHashSetBase<KeyType, Hasher>::ConstIterator cend(const ezHashSetBase<KeyType, Hasher>& set)
{
  return set.GetEndIterator();
}

#include <Foundation/Containers/Implementation/HashSet_inl.h>
