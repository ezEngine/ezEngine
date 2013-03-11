#pragma once

#include <Foundation/Algorithm/Hashing.h>
#include <Foundation/Basics/Types/ArrayPtr.h>
#include <Foundation/Math/Math.h>

template <typename KeyType, typename ValueType, typename Hasher = ezHashHelper<KeyType> >
class ezHashTable
{
private:
  struct Entry;

public:
  /// Const iterator.
  class ConstIterator
  {
  public:
    /// Checks whether this iterator points to a valid element.
    bool IsValid() const;

    /// Checks whether the two iterators point to the same element.
    bool operator==(const typename ezHashTable<KeyType, ValueType, Hasher>::ConstIterator& it2) const;

    /// Checks whether the two iterators point to the same element.
    bool operator!=(const typename ezHashTable<KeyType, ValueType, Hasher>::ConstIterator& it2) const;

    /// Returns the 'key' of the element that this iterator points to.
    const KeyType& Key() const;

    /// Returns the 'value' of the element that this iterator points to.
    const ValueType& Value() const;

    /// Advances the iterator to the next element in the map. The iterator will not be valid anymore, if the end is reached.
    void Next();

    /// Shorthand for 'Next'
    void operator++();

  protected:
    friend class ezHashTable<KeyType, ValueType, Hasher>;

    explicit ConstIterator(const ezHashTable<KeyType, ValueType, Hasher>& hashTable);

    const ezHashTable<KeyType, ValueType, Hasher>& m_hashTable;
    ezUInt32 m_uiCurrentIndex; // current element index that this iterator points to.
    ezUInt32 m_uiCurrentCount; // current number of valid elements that this iterator has found so far.
  };

  /// Iterator with write access
  struct Iterator : public ConstIterator
  {
    /// Returns the 'value' of the element that this iterator points to.
    ValueType& Value();

  private:
    friend class ezHashTable<KeyType, ValueType, Hasher>;

    explicit Iterator(const ezHashTable<KeyType, ValueType, Hasher>& hashTable);
  };

public:
  /// Creates an empty hashtable. Does not allocate any data yet.
  ezHashTable(ezIAllocator* pAllocator = ezFoundation::GetDefaultAllocator()); // [tested]
  
  /// Creates a copy of the given hashtable.
  ezHashTable(const ezHashTable<KeyType, ValueType, Hasher>& rhs); // [tested]

  ~ezHashTable(); // [tested]

  /// Copies the data from another hashtable into this one.
  void operator= (const ezHashTable<KeyType, ValueType, Hasher>& rhs); // [tested]

  /// Expands the hashtable by over-allocated the internal storage so that the load factor is lower or equal to 60% 
  /// when inserting the given number of entries.
  void Reserve(ezUInt32 uiCapacity); // [tested]

  /// Tries to compact the hashtable to avoid wasting memory. The resulting capacity is at least 'GetCount' (no elements get removed). 
  /// Will deallocate all data, if the hashtable is empty.
  void Compact(); // [tested]

  /// Returns the number of active entries in the table.
  ezUInt32 GetCount() const; // [tested]

  /// Returns true, if the hashtable does not contain any elements.
  bool IsEmpty() const; // [tested]

  /// Clears the table.
  void Clear(); // [tested]

  /// Inserts the key value pair or replaces value if an entry with the given key already exists. 
  /// Returns if an existing value was replaced and optionally writes out the old value to out_oldValue.
  bool Insert(const KeyType& key, const ValueType& value, ValueType* out_oldValue = NULL); // [tested]

  /// Removes the entry with the given key. Returns if an entry was removed and optionally writes out the old value to out_oldValue.
  bool Remove(const KeyType& key, ValueType* out_oldValue = NULL); // [tested]

  /// Returns if an entry with the given key was found and if found writes out the corresponding value to out_value.
  bool TryGetValue(const KeyType& key, ValueType& out_value) const; // [tested]

  /// Returns if an entry with the given key was found and if found writes out the pointer to the corresponding value to out_pValue.
  bool TryGetValue(const KeyType& key, ValueType*& out_pValue) const; // [tested]
  
  /// Returns the value to the given key if found or creates a new entry with the given key and a default constructed value.
  ValueType& operator[](const KeyType& key); // [tested]

  /// Returns if an entry with given key exists in the table.
  bool KeyExists(const KeyType& key) const; // [tested]

  /// Returns an Iterator to the very first element.
  Iterator GetIterator(); // [tested]

  /// Returns a constant Iterator to the very first element.
  ConstIterator GetIterator() const; // [tested]

  /// Returns the allocator that is used by this instance.
  ezIAllocator* GetAllocator() const;

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
  
  ezIAllocator* m_pAllocator;

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

  bool IsFreeEntry(ezUInt32 uiEntryIndex) const;
  bool IsValidEntry(ezUInt32 uiEntryIndex) const;
  bool IsDeletedEntry(ezUInt32 uiEntryIndex) const;

  void MarkEntryAsValid(ezUInt32 uiEntryIndex);
  void MarkEntryAsDeleted(ezUInt32 uiEntryIndex);
};

#include <Foundation/Containers/Implementation/HashTable_inl.h>
