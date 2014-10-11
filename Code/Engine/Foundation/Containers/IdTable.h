#pragma once

#include <Foundation/Types/Id.h>
#include <Foundation/Memory/AllocatorWrapper.h>

/// \brief Implementation of an id mapping table which stores id/value pairs.
///
/// An id contains an index into the table and a generation counter to detect if a table entry was re-used.
/// All insertion/erasure/lookup functions take O(1) time if the table does not need to be expanded. 
/// Lookup is nearly as fast as a simple array lookup. 
/// The table stores a free-list in its free elements to ensure fast insertion/erasure.
///
/// \see ezGenericId
template <typename IdType, typename ValueType>
class ezIdTableBase
{
public:
  typedef typename IdType::StorageType IndexType;
  typedef IdType TypeOfId;

  /// \brief Const iterator.
  class ConstIterator
  {
  public:
    /// \brief Checks whether this iterator points to a valid element.
    bool IsValid() const; // [tested]

    /// \brief Checks whether the two iterators point to the same element.
    bool operator==(const typename ezIdTableBase<IdType, ValueType>::ConstIterator& it2) const;

    /// \brief Checks whether the two iterators point to the same element.
    bool operator!=(const typename ezIdTableBase<IdType, ValueType>::ConstIterator& it2) const;

    /// \brief Returns the 'id' of the element that this iterator points to.
    IdType Id() const; // [tested]

    /// \brief Returns the 'value' of the element that this iterator points to.
    const ValueType& Value() const; // [tested]

    /// \brief Advances the iterator to the next element in the map. The iterator will not be valid anymore, if the end is reached.
    void Next(); // [tested]

    /// \brief Shorthand for 'Next'
    void operator++(); // [tested]

  protected:
    friend class ezIdTableBase<IdType, ValueType>;

    explicit ConstIterator(const ezIdTableBase<IdType, ValueType>& idTable);

    const ezIdTableBase<IdType, ValueType>& m_idTable;
    IndexType m_uiCurrentIndex; // current element index that this iterator points to.
    IndexType m_uiCurrentCount; // current number of valid elements that this iterator has found so far.
  };

  /// \brief Iterator with write access.
  struct Iterator : public ConstIterator
  {
  public:
    // this is required to pull in the const version of this function
    using ConstIterator::Value;

    /// \brief Returns the 'value' of the element that this iterator points to.
    ValueType& Value(); // [tested]

  private:
    friend class ezIdTableBase<IdType, ValueType>;

    explicit Iterator(const ezIdTableBase<IdType, ValueType>& idTable);
  };

protected:
  /// \brief Creates an empty id-table. Does not allocate any data yet.
  ezIdTableBase(ezAllocatorBase* pAllocator); // [tested]
  
  /// \brief Creates a copy of the given id-table.
  ezIdTableBase(const ezIdTableBase<IdType, ValueType>& rhs, ezAllocatorBase* pAllocator); // [tested]

  /// \brief Destructor.
  ~ezIdTableBase(); // [tested]

  /// \brief Copies the data from another table into this one.
  void operator= (const ezIdTableBase<IdType, ValueType>& rhs); // [tested]

public:
  /// \brief Expands the table so it can at least store the given capacity.
  void Reserve(IndexType uiCapacity); // [tested]

  /// \brief Returns the number of active entries in the table.
  IndexType GetCount() const; // [tested]

  /// \brief Returns true, if the table does not contain any elements.
  bool IsEmpty() const; // [tested]

  /// \brief Clears the table.
  void Clear(); // [tested]

  /// \brief Inserts the value into the table and returns the corresponding id. 
  IdType Insert(const ValueType& value); // [tested]

  /// \brief Removes the entry with the given id. Returns if an entry was removed and optionally writes out the old value to out_oldValue.
  bool Remove(const IdType id, ValueType* out_oldValue = nullptr); // [tested]

  /// \brief Returns if an entry with the given id was found and if found writes out the corresponding value to out_value.
  bool TryGetValue(const IdType id, ValueType& out_value) const; // [tested]

  /// \brief Returns if an entry with the given id was found and if found writes out the pointer to the corresponding value to out_pValue.
  bool TryGetValue(const IdType id, ValueType*& out_pValue) const; // [tested]

  /// \brief Returns the value to the given id. Does bounds checks in debug builds.
  const ValueType& operator[](const IdType id) const; // [tested]

  /// \brief Returns the value to the given id. Does bounds checks in debug builds.
  ValueType& operator[](const IdType id); // [tested]

  /// \brief Returns the value at the given index. Does bounds checks in debug builds but does not check for stale access.
  const ValueType& GetValueUnchecked(const IndexType index) const;

  /// \brief Returns the value at the given index. Does bounds checks in debug builds but does not check for stale access.
  ValueType& GetValueUnchecked(const IndexType index);

  /// \brief Returns if the table contains an entry corresponding to the given id.
  bool Contains(const IdType id) const; // [tested]

  /// \brief Returns an Iterator to the very first element.
  Iterator GetIterator(); // [tested]

  /// \brief Returns a constant Iterator to the very first element.
  ConstIterator GetIterator() const; // [tested]

  /// \brief Returns the allocator that is used by this instance.
  ezAllocatorBase* GetAllocator() const;

  /// \brief Returns whether the internal free-list is valid. For testing purpose only.
  bool IsFreelistValid() const;

private:
  enum { CAPACITY_ALIGNMENT = 16 };

  struct Entry
  {
    IdType id;
    ValueType value;
  };

  Entry* m_pEntries;
  
  IndexType m_uiCount;
  IndexType m_uiCapacity;

  IndexType m_uiFreelistEnqueue;
  IndexType m_uiFreelistDequeue;
  
  ezAllocatorBase* m_pAllocator;

  void SetCapacity(IndexType uiCapacity);
  void InitializeFreelist(IndexType uiStart, IndexType uiEnd);
};

/// \brief \see ezIdTableBase
template <typename IdType, typename ValueType, typename AllocatorWrapper = ezDefaultAllocatorWrapper>
class ezIdTable : public ezIdTableBase<IdType, ValueType>
{
public:
  ezIdTable();
  ezIdTable(ezAllocatorBase* pAllocator);

  ezIdTable(const ezIdTable<IdType, ValueType, AllocatorWrapper>& other);
  ezIdTable(const ezIdTableBase<IdType, ValueType>& other);

  void operator=(const ezIdTable<IdType, ValueType, AllocatorWrapper>& rhs);
  void operator=(const ezIdTableBase<IdType, ValueType>& rhs);
};

#include <Foundation/Containers/Implementation/IdTable_inl.h>

