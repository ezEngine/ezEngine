#pragma once

#include <Foundation/Containers/DynamicArray.h>

/// \brief An associative container, similar to ezMap, but all data is stored in a sorted contiguous array, which makes frequent lookups more efficient.
///
/// Prefer this container over ezMap when you modify the container less often than you look things up (which is in most cases), and when
/// you do not need to store iterators to elements and require them to stay valid when the container is modified.
///
/// ezArrayMapBase also allows to store multiple values under the same key (like a multi-map).
template<typename KEY, typename VALUE>
class ezArrayMapBase
{
  /// \todo Custom comparer

public:
  struct Pair
  {
    KEY key;
    VALUE value;

    EZ_DETECT_TYPE_CLASS(KEY, VALUE);

    EZ_FORCE_INLINE bool operator<(const Pair& rhs) const
    {
      return key < rhs.key;
    }

    EZ_FORCE_INLINE bool operator==(const Pair& rhs) const
    {
      return key == rhs.key;
    }
  };

  /// \brief Constructor.
  ezArrayMapBase(ezAllocatorBase* pAllocator); // [tested]

  /// \brief Copy-Constructor.
  ezArrayMapBase(const ezArrayMapBase& rhs, ezAllocatorBase* pAllocator); // [tested]

  /// \brief Copy assignment operator.
  void operator=(const ezArrayMapBase& rhs); // [tested]

  /// \brief Returns the number of elements stored in the map.
  ezUInt32 GetCount() const; // [tested]

  /// \brief True if the map contains no elements.
  bool IsEmpty() const; // [tested]

  /// \brief Purges all elements from the map.
  void Clear(); // [tested]

  /// \brief Always inserts a new value under the given key. Duplicates are allowed. The returned index is only valid briefly, until the map is sorted or modified further.
  ezUInt32 Insert(const KEY& key, const VALUE& value); // [tested]

  /// \brief Ensures the internal data structure is sorted. This is done automatically every time a lookup needs to be made.
  void Sort() const; // [tested]

  /// \brief Returns an index to one element with the given key. If the key is inserted multiple times, there is no guarantee which one is returned.
  /// Returns ezInvalidIndex when no such element exists.
  ezUInt32 Find(const KEY& key) const; // [tested]

  /// \brief Returns an index to the element with a key equal or larger than the given key.
  /// Returns ezInvalidIndex when no such element exists.
  ezUInt32 LowerBound(const KEY& key) const; // [tested]

  /// \brief Returns an index to the element with a key that is LARGER than the given key.
  /// Returns ezInvalidIndex when no such element exists.
  ezUInt32 UpperBound(const KEY& key) const; // [tested]

  /// \brief Returns the key that is stored at the given index.
  const KEY& GetKey(ezUInt32 index) const; // [tested]

  /// \brief Returns the value that is stored at the given index.
  const VALUE& GetValue(ezUInt32 index) const; // [tested]

  /// \brief Returns the value that is stored at the given index.
  VALUE& GetValue(ezUInt32 index); // [tested]

  /// \brief Returns the value stored at the given key. If none exists, one is created. \a bExisted indicates whether an element needed to be created.
  VALUE& FindOrAdd(const KEY& key, bool* bExisted = nullptr); // [tested]

  /// \brief Same as FindOrAdd.
  VALUE& operator[](const KEY& key); // [tested]
  
  /// \brief Returns the key/value pair at the given index.
  const Pair& operator[](ezUInt32 index) const; // [tested]

  /// \brief Removes the element at the given index. 
  ///
  /// If the map is sorted and bKeepSorted is true, the element will be removed such that the map stays sorted.
  /// This is only useful, if only a single (or very few) elements are removed before the next lookup. If multiple values
  /// are removed, or new values are going to be inserted, as well, \a bKeepSorted should be left to false.
  void RemoveAt(ezUInt32 index, bool bKeepSorted = false);

  /// \brief Removes one element with the given key. Returns true, if one was found and removed. If the same key exists multiple times, you need to call this function multiple times to remove them all.
  ///
  /// If the map is sorted and bKeepSorted is true, the element will be removed such that the map stays sorted.
  /// This is only useful, if only a single (or very few) elements are removed before the next lookup. If multiple values
  /// are removed, or new values are going to be inserted, as well, \a bKeepSorted should be left to false.
  bool Remove(const KEY& key, bool bKeepSorted = false); // [tested]

  /// \brief Returns whether an element with the given key exists.
  bool Contains(const KEY& key) const; // [tested]

  /// \brief Reserves enough memory to store \a size elements.
  void Reserve(ezUInt32 size); // [tested]

  /// \brief Compacts the internal memory to not waste any space.
  void Compact(); // [tested]

  /// \brief Compares the two containers for equality.
  bool operator==(const ezArrayMapBase<KEY, VALUE>& rhs) const; // [tested]

  /// \brief Compares the two containers for equality.
  bool operator!=(const ezArrayMapBase<KEY, VALUE>& rhs) const; // [tested]

  /// \brief Returns the amount of bytes that are currently allocated on the heap.
  ezUInt64 GetHeapMemoryUsage() const { return m_Data.GetHeapMemoryUsage(); } // [tested]

private:

  mutable bool m_bSorted;
  mutable ezDynamicArray<Pair> m_Data;
};

/// \brief See ezArrayMapBase for details.
template<typename KEY, typename VALUE, typename AllocatorWrapper = ezDefaultAllocatorWrapper>
class ezArrayMap : public ezArrayMapBase<KEY, VALUE>
{
  EZ_DECLARE_MEM_RELOCATABLE_TYPE();

public:

  ezArrayMap();
  ezArrayMap(ezAllocatorBase* pAllocator);

  ezArrayMap(const ezArrayMap<KEY, VALUE, AllocatorWrapper>& rhs);
  ezArrayMap(const ezArrayMapBase<KEY, VALUE>& rhs);

  void operator=(const ezArrayMap<KEY, VALUE, AllocatorWrapper>& rhs);
  void operator=(const ezArrayMapBase<KEY, VALUE>& rhs);
};

#include <Foundation/Containers/Implementation/ArrayMap_inl.h>

