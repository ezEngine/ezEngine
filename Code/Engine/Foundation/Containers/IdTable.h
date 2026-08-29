#pragma once

#include <Foundation/Memory/AllocatorWrapper.h>
#include <Foundation/Types/Id.h>

/// Implementation of an id mapping table which stores id/value pairs.
///
/// An id contains an index into the table and a generation counter to detect if a table entry was re-used.
/// All insertion/erasure/lookup functions take O(1) time if the table does not need to be expanded.
/// Lookup is nearly as fast as a simple array lookup.
/// The table stores a free-list in its free elements to ensure fast insertion/erasure.
///
/// \note Valid IDs will never be all zero (index + generation).
///
/// \see ezGenericId
template <typename IdType, typename ValueType>
class ezIdTableBase
{
public:
  using IndexType = typename IdType::StorageType;
  using TypeOfId = IdType;

  /// Const iterator.
  class ConstIterator
  {
  public:
    /// Checks whether this iterator points to a valid element.
    bool IsValid() const; // [tested]

    /// Checks whether the two iterators point to the same element.
    bool operator==(const typename ezIdTableBase<IdType, ValueType>::ConstIterator& it2) const;

    /// Checks whether the two iterators point to the same element.
    bool operator!=(const typename ezIdTableBase<IdType, ValueType>::ConstIterator& it2) const;

    /// Returns the 'id' of the element that this iterator points to.
    IdType Id() const; // [tested]

    /// Returns the 'value' of the element that this iterator points to.
    const ValueType& Value() const; // [tested]

    /// Advances the iterator to the next element in the map. The iterator will not be valid anymore, if the end is reached.
    void Next(); // [tested]

    /// Shorthand for 'Next'
    void operator++(); // [tested]

  protected:
    friend class ezIdTableBase<IdType, ValueType>;

    explicit ConstIterator(const ezIdTableBase<IdType, ValueType>& idTable);

    const ezIdTableBase<IdType, ValueType>& m_IdTable;
    IndexType m_CurrentIndex; // current element index that this iterator points to.
    IndexType m_CurrentCount; // current number of valid elements that this iterator has found so far.
  };

  /// Iterator with write access.
  struct Iterator : public ConstIterator
  {
  public:
    // this is required to pull in the const version of this function
    using ConstIterator::Value;

    /// Returns the 'value' of the element that this iterator points to.
    ValueType& Value(); // [tested]

  private:
    friend class ezIdTableBase<IdType, ValueType>;

    explicit Iterator(const ezIdTableBase<IdType, ValueType>& idTable);
  };

protected:
  /// Creates an empty id-table. Does not allocate any data yet.
  explicit ezIdTableBase(ezAllocator* pAllocator); // [tested]

  /// Creates a copy of the given id-table.
  ezIdTableBase(const ezIdTableBase<IdType, ValueType>& rhs, ezAllocator* pAllocator); // [tested]

  /// Destructor.
  ~ezIdTableBase(); // [tested]

  /// Copies the data from another table into this one.
  void operator=(const ezIdTableBase<IdType, ValueType>& rhs); // [tested]

public:
  /// Expands the table so it can at least store the given capacity.
  void Reserve(IndexType capacity); // [tested]

  /// Returns the number of active entries in the table.
  IndexType GetCount() const; // [tested]

  /// Returns the capacity of the table.
  IndexType GetCapacity() const; // [tested]

  /// Returns true, if the table does not contain any elements.
  bool IsEmpty() const; // [tested]

  /// Clears the table.
  void Clear(); // [tested]

  /// Inserts the value into the table and returns the corresponding id.
  IdType Insert(const ValueType& value); // [tested]

  /// Inserts the temporary value into the table and returns the corresponding id.
  IdType Insert(ValueType&& value);

  /// Removes the entry with the given id. Returns if an entry was removed and optionally writes out the old value to out_oldValue.
  bool Remove(const IdType id, ValueType* out_pOldValue = nullptr); // [tested]

  /// Returns if an entry with the given id was found and if found writes out the corresponding value to out_value.
  [[nodiscard]] bool TryGetValue(const IdType id, ValueType& out_value) const; // [tested]

  /// Returns if an entry with the given id was found and if found writes out the pointer to the corresponding value to out_pValue.
  [[nodiscard]] bool TryGetValue(const IdType id, ValueType*& out_pValue) const; // [tested]

  /// Returns the value to the given id. Does bounds checks in debug builds.
  const ValueType& operator[](const IdType id) const; // [tested]

  /// Returns the value to the given id. Does bounds checks in debug builds.
  ValueType& operator[](const IdType id); // [tested]

  /// Returns the value at the given index. Does bounds checks in debug builds but does not check for stale access.
  const ValueType& GetValueUnchecked(const IndexType index) const;

  /// Returns the value at the given index. Does bounds checks in debug builds but does not check for stale access.
  ValueType& GetValueUnchecked(const IndexType index);

  /// Returns the id of the entry at the given index. Does bounds checks in debug builds but does not check for stale access.
  IdType GetIdUnchecked(const IndexType index) const;

  /// Returns if the table contains an entry corresponding to the given id.
  bool Contains(const IdType id) const; // [tested]

  /// Returns an Iterator to the very first element.
  Iterator GetIterator(); // [tested]

  /// Returns a constant Iterator to the very first element.
  ConstIterator GetIterator() const; // [tested]

  /// Returns the allocator that is used by this instance.
  ezAllocator* GetAllocator() const;

  /// Returns whether the internal free-list is valid. For testing purpose only.
  bool IsFreelistValid() const;

private:
  enum
  {
    CAPACITY_ALIGNMENT = 16
  };

  struct Entry
  {
    IdType id;
    ValueType value;
  };

  Entry* m_pEntries;

  IndexType m_Count;
  IndexType m_Capacity;

  IndexType m_FreelistEnqueue;
  IndexType m_FreelistDequeue;

  ezAllocator* m_pAllocator;

  void SetCapacity(IndexType uiCapacity);
  void InitializeFreelist(IndexType uiStart, IndexType uiEnd);
};

/// \see ezIdTableBase
template <typename IdType, typename ValueType, typename AllocatorWrapper = ezDefaultAllocatorWrapper>
class ezIdTable : public ezIdTableBase<IdType, ValueType>
{
public:
  ezIdTable();
  explicit ezIdTable(ezAllocator* pAllocator);

  ezIdTable(const ezIdTable<IdType, ValueType, AllocatorWrapper>& other);
  ezIdTable(const ezIdTableBase<IdType, ValueType>& other);

  void operator=(const ezIdTable<IdType, ValueType, AllocatorWrapper>& rhs);
  void operator=(const ezIdTableBase<IdType, ValueType>& rhs);
};

#include <Foundation/Containers/Implementation/IdTable_inl.h>
