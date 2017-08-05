#pragma once

#include <Foundation/Algorithm/Hashing.h>
#include <Foundation/Types/ArrayPtr.h>
#include <Foundation/Math/Math.h>
#include <Foundation/Memory/AllocatorWrapper.h>

/// \brief Implementation of a hashtable which stores key/value pairs.
///
/// The hashtable maps keys to values by using the hash of the key as an index into the table.
/// This implementation uses linear-probing to resolve hash collisions which means all key/value pairs are stored
/// in a linear array.
/// All insertion/erasure/lookup functions take O(1) time if the table does not need to be expanded,
/// which happens when the load gets greater than 60%.
/// The hash function can be customized by providing a Hasher helper class like ezHashHelper.

/// \see ezHashHelper
template <typename KeyType, typename ValueType, typename Hasher>
class ezHashTableBase
{
public:
  /// \brief Const iterator.
  struct ConstIterator
  {
    EZ_DECLARE_POD_TYPE();

    /// \brief Checks whether this iterator points to a valid element.
    bool IsValid() const; // [tested]

    /// \brief Checks whether the two iterators point to the same element.
    bool operator==(const typename ezHashTableBase<KeyType, ValueType, Hasher>::ConstIterator& it2) const;

    /// \brief Checks whether the two iterators point to the same element.
    bool operator!=(const typename ezHashTableBase<KeyType, ValueType, Hasher>::ConstIterator& it2) const;

    /// \brief Returns the 'key' of the element that this iterator points to.
    const KeyType& Key() const; // [tested]

    /// \brief Returns the 'value' of the element that this iterator points to.
    const ValueType& Value() const; // [tested]

    /// \brief Advances the iterator to the next element in the map. The iterator will not be valid anymore, if the end is reached.
    void Next(); // [tested]

    /// \brief Shorthand for 'Next'
    void operator++(); // [tested]

  protected:
    friend class ezHashTableBase<KeyType, ValueType, Hasher>;

    explicit ConstIterator(const ezHashTableBase<KeyType, ValueType, Hasher>& hashTable);

    const ezHashTableBase<KeyType, ValueType, Hasher>* m_hashTable;
    ezUInt32 m_uiCurrentIndex; // current element index that this iterator points to.
    ezUInt32 m_uiCurrentCount; // current number of valid elements that this iterator has found so far.
  };

  /// \brief Iterator with write access.
  struct Iterator : public ConstIterator
  {
    EZ_DECLARE_POD_TYPE();

    /// \brief Creates a new itarator from another.
    EZ_ALWAYS_INLINE Iterator(const Iterator& rhs); // [tested]

    /// \brief Assigns one itarator no another.
    EZ_ALWAYS_INLINE void operator=(const Iterator& rhs); // [tested]

    // this is required to pull in the const version of this function
    using ConstIterator::Value;

    /// \brief Returns the 'value' of the element that this iterator points to.
    EZ_FORCE_INLINE ValueType& Value(); // [tested]

    /// \brief Returns the 'value' of the element that this iterator points to.
    EZ_ALWAYS_INLINE ValueType& operator*() { return Value(); }

  private:
    friend class ezHashTableBase<KeyType, ValueType, Hasher>;

    explicit Iterator(const ezHashTableBase<KeyType, ValueType, Hasher>& hashTable);
  };

protected:
  /// \brief Creates an empty hashtable. Does not allocate any data yet.
  ezHashTableBase(ezAllocatorBase* pAllocator); // [tested]

  /// \brief Creates a copy of the given hashtable.
  ezHashTableBase(const ezHashTableBase<KeyType, ValueType, Hasher>& rhs, ezAllocatorBase* pAllocator); // [tested]

  /// \brief Moves data from an existing hashtable into this one.
  ezHashTableBase(ezHashTableBase<KeyType, ValueType, Hasher>&& rhs, ezAllocatorBase* pAllocator); // [tested]

  /// \brief Destructor.
  ~ezHashTableBase(); // [tested]

  /// \brief Copies the data from another hashtable into this one.
  void operator= (const ezHashTableBase<KeyType, ValueType, Hasher>& rhs); // [tested]

  /// \brief Moves data from an existing hashtable into this one.
  void operator= (ezHashTableBase<KeyType, ValueType, Hasher>&& rhs); // [tested]

public:

  /// \brief Compares this table to another table.
  bool operator== (const ezHashTableBase<KeyType, ValueType, Hasher>& rhs) const; // [tested]

   /// \brief Compares this table to another table.
  bool operator!= (const ezHashTableBase<KeyType, ValueType, Hasher>& rhs) const; // [tested]

  /// \brief Expands the hashtable by over-allocating the internal storage so that the load factor is lower or equal to 60% when inserting the given number of entries.
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
  bool Insert(CompatibleKeyType&& key, CompatibleValueType&& value, ValueType* out_oldValue = nullptr); // [tested]

  /// \brief Removes the entry with the given key. Returns if an entry was removed and optionally writes out the old value to out_oldValue.
  template <typename CompatibleKeyType>
  bool Remove(const CompatibleKeyType& key, ValueType* out_oldValue = nullptr); // [tested]

  /// \brief Erases the key/value pair at the given Iterator. Returns an iterator to the element after the given iterator.
  Iterator Remove(const Iterator& pos); // [tested]

  /// \brief Returns if an entry with the given key was found and if found writes out the corresponding value to out_value.
  template <typename CompatibleKeyType>
  bool TryGetValue(const CompatibleKeyType& key, ValueType& out_value) const; // [tested]

  /// \brief Returns if an entry with the given key was found and if found writes out the pointer to the corresponding value to out_pValue.
  template <typename CompatibleKeyType>
  bool TryGetValue(const CompatibleKeyType& key, const ValueType*& out_pValue) const; // [tested]

  /// \brief Returns if an entry with the given key was found and if found writes out the pointer to the corresponding value to out_pValue.
  template <typename CompatibleKeyType>
  bool TryGetValue(const CompatibleKeyType& key, ValueType*& out_pValue); // [tested]

  /// \brief Returns a pointer to the value of the entry with the given key if found, otherwise returns nullptr.
  template <typename CompatibleKeyType>
  const ValueType* GetValue(const CompatibleKeyType& key) const; // [tested]

  /// \brief Returns a pointer to the value of the entry with the given key if found, otherwise returns nullptr.
  template <typename CompatibleKeyType>
  ValueType* GetValue(const CompatibleKeyType& key); // [tested]

  /// \brief Returns the value to the given key if found or creates a new entry with the given key and a default constructed value.
  ValueType& operator[](const KeyType& key); // [tested]

  /// \brief Returns if an entry with given key exists in the table.
  template <typename CompatibleKeyType>
  bool Contains(const CompatibleKeyType& key) const; // [tested]

  /// \brief Returns an Iterator to the very first element.
  Iterator GetIterator(); // [tested]

  /// \brief Returns a constant Iterator to the very first element.
  ConstIterator GetIterator() const; // [tested]

  /// \brief Returns the allocator that is used by this instance.
  ezAllocatorBase* GetAllocator() const;

  /// \brief Returns the amount of bytes that are currently allocated on the heap.
  ezUInt64 GetHeapMemoryUsage() const; // [tested]

private:

  struct Entry
  {
    KeyType key;
    ValueType value;
  };

  Entry* m_pEntries;
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
  ezHashTable(ezAllocatorBase* pAllocator);

  ezHashTable(const ezHashTable<KeyType, ValueType, Hasher, AllocatorWrapper>& other);
  ezHashTable(const ezHashTableBase<KeyType, ValueType, Hasher>& other);

  ezHashTable(ezHashTable<KeyType, ValueType, Hasher, AllocatorWrapper>&& other);
  ezHashTable(ezHashTableBase<KeyType, ValueType, Hasher>&& other);


  void operator=(const ezHashTable<KeyType, ValueType, Hasher, AllocatorWrapper>& rhs);
  void operator=(const ezHashTableBase<KeyType, ValueType, Hasher>& rhs);

  void operator=(ezHashTable<KeyType, ValueType, Hasher, AllocatorWrapper>&& rhs);
  void operator=(ezHashTableBase<KeyType, ValueType, Hasher>&& rhs);
};

#include <Foundation/Containers/Implementation/HashTable_inl.h>

