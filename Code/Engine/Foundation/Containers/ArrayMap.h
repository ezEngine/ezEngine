#pragma once

#include <Foundation/Containers/DynamicArray.h>

/// \brief An associative container, similar to ezMap, but all data is stored in a sorted contiguous array, which makes frequent lookups more
/// efficient.
///
/// Prefer this container over ezMap when you modify the container less often than you look things up (which is in most cases), and when
/// you do not need to store iterators to elements and require them to stay valid when the container is modified.
///
/// ezArrayMapBase also allows to store multiple values under the same key (like a multi-map).
template <typename KEY, typename VALUE>
class ezArrayMapBase
{
  /// \todo Custom comparer

public:
  struct Pair
  {
    KEY key;
    VALUE value;

    EZ_DETECT_TYPE_CLASS(KEY, VALUE);

    EZ_ALWAYS_INLINE bool operator<(const Pair& rhs) const { return key < rhs.key; }

    EZ_ALWAYS_INLINE bool operator==(const Pair& rhs) const { return key == rhs.key; }
  };

  /// \brief Constructor.
  explicit ezArrayMapBase(ezAllocator* pAllocator); // [tested]

  /// \brief Copy-Constructor.
  ezArrayMapBase(const ezArrayMapBase& rhs, ezAllocator* pAllocator); // [tested]

  /// \brief Copy assignment operator.
  void operator=(const ezArrayMapBase& rhs); // [tested]

  /// \brief Returns the number of elements stored in the map.
  ezUInt32 GetCount() const; // [tested]

  /// \brief True if the map contains no elements.
  bool IsEmpty() const; // [tested]

  /// \brief Purges all elements from the map.
  void Clear(); // [tested]

  /// \brief Always inserts a new value under the given key. Duplicates are allowed.
  /// Returns the index of the newly added element.
  template <typename CompatibleKeyType, typename CompatibleValueType>
  ezUInt32 Insert(CompatibleKeyType&& key, CompatibleValueType&& value); // [tested]

  /// \brief Ensures the internal data structure is sorted. This is done automatically every time a lookup needs to be made.
  void Sort() const; // [tested]

  /// \brief Returns an index to one element with the given key. If the key is inserted multiple times, there is no guarantee which one is returned.
  /// Returns ezInvalidIndex when no such element exists.
  template <typename CompatibleKeyType>
  ezUInt32 Find(const CompatibleKeyType& key) const; // [tested]

  /// \brief Returns the index to the first element with a key equal or larger than the given key.
  /// Returns ezInvalidIndex when no such element exists.
  /// If there are multiple keys with the same value, the one at the smallest index is returned.
  template <typename CompatibleKeyType>
  ezUInt32 LowerBound(const CompatibleKeyType& key) const; // [tested]

  /// \brief Returns the index to the first element with a key that is LARGER than the given key.
  /// Returns ezInvalidIndex when no such element exists.
  /// If there are multiple keys with the same value, the one at the smallest index is returned.
  template <typename CompatibleKeyType>
  ezUInt32 UpperBound(const CompatibleKeyType& key) const; // [tested]

  /// \brief Returns the key that is stored at the given index.
  const KEY& GetKey(ezUInt32 uiIndex) const; // [tested]

  /// \brief Returns the value that is stored at the given index.
  const VALUE& GetValue(ezUInt32 uiIndex) const; // [tested]

  /// \brief Returns the value that is stored at the given index.
  VALUE& GetValue(ezUInt32 uiIndex); // [tested]

  /// \brief Returns a reference to the map data array.
  ezDynamicArray<Pair>& GetData();

  /// \brief Returns a constant reference to the map data array.
  const ezDynamicArray<Pair>& GetData() const;

  /// \brief Returns the value stored at the given key. If none exists, one is created. \a bExisted indicates whether an element needed to be created.
  template <typename CompatibleKeyType>
  VALUE& FindOrAdd(const CompatibleKeyType& key, bool* out_pExisted = nullptr); // [tested]

  /// \brief Same as FindOrAdd.
  template <typename CompatibleKeyType>
  VALUE& operator[](const CompatibleKeyType& key); // [tested]

  /// \brief Returns the key/value pair at the given index.
  const Pair& GetPair(ezUInt32 uiIndex) const; // [tested]

  /// \brief Removes the element at the given index.
  ///
  /// If the map is sorted and bKeepSorted is true, the element will be removed such that the map stays sorted.
  /// This is only useful, if only a single (or very few) elements are removed before the next lookup. If multiple values
  /// are removed, or new values are going to be inserted, as well, \a bKeepSorted should be left to false.
  void RemoveAtAndCopy(ezUInt32 uiIndex, bool bKeepSorted = false);

  /// \brief Removes one element with the given key. Returns true, if one was found and removed. If the same key exists multiple times, you need to
  /// call this function multiple times to remove them all.
  ///
  /// If the map is sorted and bKeepSorted is true, the element will be removed such that the map stays sorted.
  /// This is only useful, if only a single (or very few) elements are removed before the next lookup. If multiple values
  /// are removed, or new values are going to be inserted, as well, \a bKeepSorted should be left to false.
  template <typename CompatibleKeyType>
  bool RemoveAndCopy(const CompatibleKeyType& key, bool bKeepSorted = false); // [tested]

  /// \brief Returns whether an element with the given key exists.
  template <typename CompatibleKeyType>
  bool Contains(const CompatibleKeyType& key) const; // [tested]

  /// \brief Returns whether an element with the given key and value already exists.
  template <typename CompatibleKeyType>
  bool Contains(const CompatibleKeyType& key, const VALUE& value) const; // [tested]

  /// \brief Reserves enough memory to store \a size elements.
  void Reserve(ezUInt32 uiSize); // [tested]

  /// \brief Compacts the internal memory to not waste any space.
  void Compact(); // [tested]

  /// \brief Compares the two containers for equality.
  bool operator==(const ezArrayMapBase<KEY, VALUE>& rhs) const; // [tested]
  EZ_ADD_DEFAULT_OPERATOR_NOTEQUAL(const ezArrayMapBase<KEY, VALUE>&);

  /// \brief Returns the amount of bytes that are currently allocated on the heap.
  ezUInt64 GetHeapMemoryUsage() const { return m_Data.GetHeapMemoryUsage(); } // [tested]

  using const_iterator = typename ezDynamicArray<Pair>::const_iterator;
  using const_reverse_iterator = typename ezDynamicArray<Pair>::const_reverse_iterator;
  using iterator = typename ezDynamicArray<Pair>::iterator;
  using reverse_iterator = typename ezDynamicArray<Pair>::reverse_iterator;

private:
  mutable bool m_bSorted;
  mutable ezDynamicArray<Pair> m_Data;
};

/// \brief See ezArrayMapBase for details.
template <typename KEY, typename VALUE, typename AllocatorWrapper = ezDefaultAllocatorWrapper>
class ezArrayMap : public ezArrayMapBase<KEY, VALUE>
{
public:
  EZ_DECLARE_MEM_RELOCATABLE_TYPE();

  ezArrayMap();
  explicit ezArrayMap(ezAllocator* pAllocator);

  ezArrayMap(const ezArrayMap<KEY, VALUE, AllocatorWrapper>& rhs);
  ezArrayMap(const ezArrayMapBase<KEY, VALUE>& rhs);

  void operator=(const ezArrayMap<KEY, VALUE, AllocatorWrapper>& rhs);
  void operator=(const ezArrayMapBase<KEY, VALUE>& rhs);
};


template <typename KEY, typename VALUE>
typename ezArrayMapBase<KEY, VALUE>::iterator begin(ezArrayMapBase<KEY, VALUE>& ref_container)
{
  return begin(ref_container.GetData());
}

template <typename KEY, typename VALUE>
typename ezArrayMapBase<KEY, VALUE>::const_iterator begin(const ezArrayMapBase<KEY, VALUE>& container)
{
  return begin(container.GetData());
}
template <typename KEY, typename VALUE>
typename ezArrayMapBase<KEY, VALUE>::const_iterator cbegin(const ezArrayMapBase<KEY, VALUE>& container)
{
  return cbegin(container.GetData());
}

template <typename KEY, typename VALUE>
typename ezArrayMapBase<KEY, VALUE>::reverse_iterator rbegin(ezArrayMapBase<KEY, VALUE>& ref_container)
{
  return rbegin(ref_container.GetData());
}

template <typename KEY, typename VALUE>
typename ezArrayMapBase<KEY, VALUE>::const_reverse_iterator rbegin(const ezArrayMapBase<KEY, VALUE>& container)
{
  return rbegin(container.GetData());
}

template <typename KEY, typename VALUE>
typename ezArrayMapBase<KEY, VALUE>::const_reverse_iterator crbegin(const ezArrayMapBase<KEY, VALUE>& container)
{
  return crbegin(container.GetData());
}

template <typename KEY, typename VALUE>
typename ezArrayMapBase<KEY, VALUE>::iterator end(ezArrayMapBase<KEY, VALUE>& ref_container)
{
  return end(ref_container.GetData());
}

template <typename KEY, typename VALUE>
typename ezArrayMapBase<KEY, VALUE>::const_iterator end(const ezArrayMapBase<KEY, VALUE>& container)
{
  return end(container.GetData());
}

template <typename KEY, typename VALUE>
typename ezArrayMapBase<KEY, VALUE>::const_iterator cend(const ezArrayMapBase<KEY, VALUE>& container)
{
  return cend(container.GetData());
}

template <typename KEY, typename VALUE>
typename ezArrayMapBase<KEY, VALUE>::reverse_iterator rend(ezArrayMapBase<KEY, VALUE>& ref_container)
{
  return rend(ref_container.GetData());
}

template <typename KEY, typename VALUE>
typename ezArrayMapBase<KEY, VALUE>::const_reverse_iterator rend(const ezArrayMapBase<KEY, VALUE>& container)
{
  return rend(container.GetData());
}

template <typename KEY, typename VALUE>
typename ezArrayMapBase<KEY, VALUE>::const_reverse_iterator crend(const ezArrayMapBase<KEY, VALUE>& container)
{
  return crend(container.GetData());
}


#include <Foundation/Containers/Implementation/ArrayMap_inl.h>
