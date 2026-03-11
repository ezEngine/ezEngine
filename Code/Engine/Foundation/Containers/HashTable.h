#pragma once

#include <Foundation/Algorithm/HashingUtils.h>
#include <Foundation/Math/Math.h>
#include <Foundation/Memory/AllocatorWrapper.h>

template <typename KeyType, typename ValueType, typename Hasher>
class ezHashTableBase;

/// \brief Const iterator.
template <typename KeyType, typename ValueType, typename Hasher>
struct ezHashTableBaseConstIterator
{
  using iterator_category = std::forward_iterator_tag;
  using value_type = ezHashTableBaseConstIterator;
  using difference_type = std::ptrdiff_t;
  using pointer = ezHashTableBaseConstIterator*;
  using reference = ezHashTableBaseConstIterator&;

  EZ_DECLARE_POD_TYPE();

  ezHashTableBaseConstIterator() = default;

  /// \brief Checks whether this iterator points to a valid element.
  bool IsValid() const; // [tested]

  /// \brief Checks whether the two iterators point to the same element.
  bool operator==(const ezHashTableBaseConstIterator& rhs) const;
  EZ_ADD_DEFAULT_OPERATOR_NOTEQUAL(const ezHashTableBaseConstIterator&);

  /// \brief Returns the 'key' of the element that this iterator points to.
  const KeyType& Key() const; // [tested]

  /// \brief Returns the 'value' of the element that this iterator points to.
  const ValueType& Value() const; // [tested]

  /// \brief Advances the iterator to the next element in the map. The iterator will not be valid anymore, if the end is reached.
  void Next(); // [tested]

  /// \brief Shorthand for 'Next'
  void operator++(); // [tested]

  /// \brief Returns '*this' to enable foreach
  EZ_ALWAYS_INLINE ezHashTableBaseConstIterator& operator*() { return *this; } // [tested]

protected:
  friend class ezHashTableBase<KeyType, ValueType, Hasher>;

  explicit ezHashTableBaseConstIterator(const ezHashTableBase<KeyType, ValueType, Hasher>& hashTable);
  void SetToBegin();
  void SetToEnd();

  const ezHashTableBase<KeyType, ValueType, Hasher>* m_pHashTable = nullptr;
  ezUInt32 m_uiCurrentIndex = 0; // current element index that this iterator points to.
  ezUInt32 m_uiCurrentCount = 0; // current number of valid elements that this iterator has found so far.

#if EZ_ENABLED(EZ_USE_CPP20_OPERATORS)
public:
  struct Pointer
  {
    std::pair<const KeyType&, const ValueType&> value;
    const std::pair<const KeyType&, const ValueType&>* operator->() const { return &value; }
  };

  EZ_ALWAYS_INLINE Pointer operator->() const
  {
    return Pointer{.value = {Key(), Value()}};
  }

  // These function is used to return the values for structured bindings.
  // The number and type of type of each slot are defined in the inl file.
  template <std::size_t Index>
  std::tuple_element_t<Index, ezHashTableBaseConstIterator>& get() const
  {
    if constexpr (Index == 0)
      return Key();
    if constexpr (Index == 1)
      return Value();
  }
#endif
};

/// \brief Iterator with write access.
template <typename KeyType, typename ValueType, typename Hasher>
struct ezHashTableBaseIterator : public ezHashTableBaseConstIterator<KeyType, ValueType, Hasher>
{
  EZ_DECLARE_POD_TYPE();

  /// \brief Creates a new iterator from another.
  EZ_ALWAYS_INLINE ezHashTableBaseIterator(const ezHashTableBaseIterator& rhs); // [tested]

  /// \brief Assigns one iterator no another.
  EZ_ALWAYS_INLINE void operator=(const ezHashTableBaseIterator& rhs); // [tested]

  // this is required to pull in the const version of this function
  using ezHashTableBaseConstIterator<KeyType, ValueType, Hasher>::Value;

  /// \brief Returns the 'value' of the element that this iterator points to.
  EZ_FORCE_INLINE ValueType& Value(); // [tested]

  /// \brief Returns the 'value' of the element that this iterator points to.
  EZ_FORCE_INLINE ValueType& Value() const;

  /// \brief Returns '*this' to enable foreach
  EZ_ALWAYS_INLINE ezHashTableBaseIterator& operator*() { return *this; } // [tested]

private:
  friend class ezHashTableBase<KeyType, ValueType, Hasher>;

  explicit ezHashTableBaseIterator(const ezHashTableBase<KeyType, ValueType, Hasher>& hashTable);

#if EZ_ENABLED(EZ_USE_CPP20_OPERATORS)
public:
  struct Pointer
  {
    std::pair<const KeyType&, ValueType&> value;
    const std::pair<const KeyType&, ValueType&>* operator->() const { return &value; }
  };

  EZ_ALWAYS_INLINE Pointer operator->() const
  {
    return Pointer{.value = {ezHashTableBaseConstIterator<KeyType, ValueType, Hasher>::Key(), Value()}};
  }

  // These functions are used to return the values for structured bindings.
  // The number and type of type of each slot are defined in the inl file.
  template <std::size_t Index>
  std::tuple_element_t<Index, ezHashTableBaseIterator>& get()
  {
    if constexpr (Index == 0)
      return ezHashTableBaseConstIterator<KeyType, ValueType, Hasher>::Key();
    if constexpr (Index == 1)
      return Value();
  }

  template <std::size_t Index>
  std::tuple_element_t<Index, ezHashTableBaseIterator>& get() const
  {
    if constexpr (Index == 0)
      return ezHashTableBaseConstIterator<KeyType, ValueType, Hasher>::Key();
    if constexpr (Index == 1)
      return Value();
  }
#endif
};

/// \brief Implementation of a hashtable which stores key/value pairs.
///
/// The hashtable maps keys to values by using the hash of the key as an index into the table.
/// This implementation uses linear-probing to resolve hash collisions which means all key/value pairs are stored
/// in a linear array. Automatic resizing maintains a load factor below 60% for optimal performance.
///
/// Performance characteristics:
/// - Average case: O(1) - insertion, erasure, lookup
/// - Worst case: O(n) - when all keys hash to the same location (very rare with good hash functions)
/// - Resizing: O(n) - occurs when load factor exceeds 60%, amortized cost is still O(1) per operation
/// - Memory usage: More memory efficient than tree-based containers, ~1.67x element storage
/// - Iteration: O(n) in hash order (not sorted)
///
/// Use when:
/// - Fast lookup/insertion/removal is the primary concern
/// - You don't need sorted iteration
/// - Memory efficiency is important
/// - You have a good hash function for your key type
///
/// Consider ezMap instead when:
/// - You need sorted iteration by key
/// - You need stable element addresses (no reallocation)
/// - You need range queries (lower_bound, upper_bound)
/// - Predictable O(log n) performance is more important than average O(1)
///
/// The hash function can be customized by providing a Hasher helper class like ezHashHelper.
/// \see ezHashHelper
template <typename KeyType, typename ValueType, typename Hasher>
class ezHashTableBase
{
public:
  using Iterator = ezHashTableBaseIterator<KeyType, ValueType, Hasher>;
  using ConstIterator = ezHashTableBaseConstIterator<KeyType, ValueType, Hasher>;

protected:
  /// \brief Creates an empty hashtable. Does not allocate any data yet.
  explicit ezHashTableBase(ezAllocator* pAllocator); // [tested]

  /// \brief Creates a copy of the given hashtable.
  ezHashTableBase(const ezHashTableBase<KeyType, ValueType, Hasher>& rhs, ezAllocator* pAllocator); // [tested]

  /// \brief Moves data from an existing hashtable into this one.
  ezHashTableBase(ezHashTableBase<KeyType, ValueType, Hasher>&& rhs, ezAllocator* pAllocator); // [tested]

  /// \brief Destructor.
  ~ezHashTableBase(); // [tested]

  /// \brief Copies the data from another hashtable into this one.
  void operator=(const ezHashTableBase<KeyType, ValueType, Hasher>& rhs); // [tested]

  /// \brief Moves data from an existing hashtable into this one.
  void operator=(ezHashTableBase<KeyType, ValueType, Hasher>&& rhs); // [tested]

public:
  /// \brief Compares this table to another table.
  bool operator==(const ezHashTableBase<KeyType, ValueType, Hasher>& rhs) const; // [tested]
  EZ_ADD_DEFAULT_OPERATOR_NOTEQUAL(const ezHashTableBase<KeyType, ValueType, Hasher>&);

  /// \brief Expands the hashtable by over-allocating the internal storage so that the load factor is lower or equal to 60% when inserting the given
  /// number of entries.
  void Reserve(ezUInt32 uiCapacity); // [tested]

  /// \brief Tries to compact the hashtable to avoid wasting memory.
  ///
  /// The resulting capacity is at least 'GetCount' (no elements get removed).
  /// Will deallocate all data, if the hashtable is empty.
  void Compact(); // [tested]

  /// \brief Returns the number of active entries in the table.
  ezUInt32 GetCount() const; // [tested]

  /// \brief Returns true, if the hashtable does not contain any elements.
  bool IsEmpty() const; // [tested]

  /// \brief Clears the table.
  void Clear(); // [tested]

  /// \brief Inserts the key value pair or replaces value if an entry with the given key already exists.
  ///
  /// Returns true if an existing value was replaced and optionally writes out the old value to out_oldValue.
  template <typename CompatibleKeyType, typename CompatibleValueType>
  bool Insert(CompatibleKeyType&& key, CompatibleValueType&& value, ValueType* out_pOldValue = nullptr); // [tested]

  /// \brief Removes the entry with the given key. Returns whether an entry was removed and optionally writes out the old value to out_oldValue.
  template <typename CompatibleKeyType>
  bool Remove(const CompatibleKeyType& key, ValueType* out_pOldValue = nullptr); // [tested]

  /// \brief Erases the key/value pair at the given Iterator. Returns an iterator to the element after the given iterator.
  Iterator Remove(const Iterator& pos); // [tested]

  /// \brief Cannot remove an element with just a ezHashTableBaseConstIterator
  void Remove(const ConstIterator& pos) = delete;

  /// \brief Returns whether an entry with the given key was found and if found writes out the corresponding value to out_value.
  template <typename CompatibleKeyType>
  bool TryGetValue(const CompatibleKeyType& key, ValueType& out_value) const; // [tested]

  /// \brief Returns whether an entry with the given key was found and if found writes out the pointer to the corresponding value to out_pValue.
  template <typename CompatibleKeyType>
  bool TryGetValue(const CompatibleKeyType& key, const ValueType*& out_pValue) const; // [tested]

  /// \brief Returns whether an entry with the given key was found and if found writes out the pointer to the corresponding value to out_pValue.
  template <typename CompatibleKeyType>
  bool TryGetValue(const CompatibleKeyType& key, ValueType*& out_pValue) const; // [tested]

  /// \brief Searches for key, returns a ezHashTableBaseConstIterator to it or an invalid iterator, if no such key is found. O(1) operation.
  template <typename CompatibleKeyType>
  ConstIterator Find(const CompatibleKeyType& key) const;

  /// \brief Searches for key, returns an Iterator to it or an invalid iterator, if no such key is found. O(1) operation.
  template <typename CompatibleKeyType>
  Iterator Find(const CompatibleKeyType& key);

  /// \brief Returns a pointer to the value of the entry with the given key if found, otherwise returns nullptr.
  template <typename CompatibleKeyType>
  const ValueType* GetValue(const CompatibleKeyType& key) const; // [tested]

  /// \brief Returns a pointer to the value of the entry with the given key if found, otherwise returns nullptr.
  template <typename CompatibleKeyType>
  ValueType* GetValue(const CompatibleKeyType& key); // [tested]

  /// \brief Returns the value to the given key if found or creates a new entry with the given key and a default constructed value.
  ValueType& operator[](const KeyType& key); // [tested]

  /// \brief Returns the value stored at the given key. If none exists, one is created. \a bExisted indicates whether an element needed to be created.
  ValueType& FindOrAdd(const KeyType& key, bool* out_pExisted = nullptr); // [tested]

  /// \brief Returns if an entry with given key exists in the table.
  template <typename CompatibleKeyType>
  bool Contains(const CompatibleKeyType& key) const; // [tested]

  /// \brief Returns an Iterator to the very first element.
  Iterator GetIterator(); // [tested]

  /// \brief Returns an Iterator to the first element that is not part of the hash-table. Needed to support range based for loops.
  Iterator GetEndIterator(); // [tested]

  /// \brief Returns a constant Iterator to the very first element.
  ConstIterator GetIterator() const; // [tested]

  /// \brief Returns a ezHashTableBaseConstIterator to the first element that is not part of the hash-table. Needed to support range based for loops.
  ConstIterator GetEndIterator() const; // [tested]

  /// \brief Returns the allocator that is used by this instance.
  ezAllocator* GetAllocator() const;

  /// \brief Returns the amount of bytes that are currently allocated on the heap.
  ezUInt64 GetHeapMemoryUsage() const; // [tested]

  /// \brief Swaps this map with the other one.
  void Swap(ezHashTableBase<KeyType, ValueType, Hasher>& other); // [tested]

private:
  friend struct ezHashTableBaseConstIterator<KeyType, ValueType, Hasher>;
  friend struct ezHashTableBaseIterator<KeyType, ValueType, Hasher>;

  struct Entry
  {
    KeyType key;
    ValueType value;
  };

  Entry* m_pEntries = nullptr;
  ezUInt32* m_pEntryFlags = nullptr;

  ezUInt32 m_uiCount = 0;
  ezUInt32 m_uiCapacity = 0;

  ezAllocator* m_pAllocator = nullptr;

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

/// \brief \see ezHashTableBase
template <typename KeyType, typename ValueType, typename Hasher = ezHashHelper<KeyType>, typename AllocatorWrapper = ezDefaultAllocatorWrapper>
class ezHashTable : public ezHashTableBase<KeyType, ValueType, Hasher>
{
public:
  ezHashTable();
  explicit ezHashTable(ezAllocator* pAllocator);

  ezHashTable(const ezHashTable<KeyType, ValueType, Hasher, AllocatorWrapper>& other);
  ezHashTable(const ezHashTableBase<KeyType, ValueType, Hasher>& other);

  ezHashTable(ezHashTable<KeyType, ValueType, Hasher, AllocatorWrapper>&& other);
  ezHashTable(ezHashTableBase<KeyType, ValueType, Hasher>&& other);


  void operator=(const ezHashTable<KeyType, ValueType, Hasher, AllocatorWrapper>& rhs);
  void operator=(const ezHashTableBase<KeyType, ValueType, Hasher>& rhs);

  void operator=(ezHashTable<KeyType, ValueType, Hasher, AllocatorWrapper>&& rhs);
  void operator=(ezHashTableBase<KeyType, ValueType, Hasher>&& rhs);
};

//////////////////////////////////////////////////////////////////////////
// begin() /end() for range-based for-loop support

template <typename KeyType, typename ValueType, typename Hasher>
typename ezHashTableBase<KeyType, ValueType, Hasher>::Iterator begin(ezHashTableBase<KeyType, ValueType, Hasher>& ref_container)
{
  return ref_container.GetIterator();
}

template <typename KeyType, typename ValueType, typename Hasher>
typename ezHashTableBase<KeyType, ValueType, Hasher>::ConstIterator begin(const ezHashTableBase<KeyType, ValueType, Hasher>& container)
{
  return container.GetIterator();
}

template <typename KeyType, typename ValueType, typename Hasher>
typename ezHashTableBase<KeyType, ValueType, Hasher>::ConstIterator cbegin(const ezHashTableBase<KeyType, ValueType, Hasher>& container)
{
  return container.GetIterator();
}

template <typename KeyType, typename ValueType, typename Hasher>
typename ezHashTableBase<KeyType, ValueType, Hasher>::Iterator end(ezHashTableBase<KeyType, ValueType, Hasher>& ref_container)
{
  return ref_container.GetEndIterator();
}

template <typename KeyType, typename ValueType, typename Hasher>
typename ezHashTableBase<KeyType, ValueType, Hasher>::ConstIterator end(const ezHashTableBase<KeyType, ValueType, Hasher>& container)
{
  return container.GetEndIterator();
}

template <typename KeyType, typename ValueType, typename Hasher>
typename ezHashTableBase<KeyType, ValueType, Hasher>::ConstIterator cend(const ezHashTableBase<KeyType, ValueType, Hasher>& container)
{
  return container.GetEndIterator();
}

#include <Foundation/Containers/Implementation/HashTable_inl.h>
