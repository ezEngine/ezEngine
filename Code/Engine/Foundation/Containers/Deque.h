#pragma once

#include <Foundation/Algorithm/Sorting.h>
#include <Foundation/Memory/AllocatorWrapper.h>
#include <Foundation/Containers/Implementation/ArrayIterator.h>

#define ezInvalidIndex 0xFFFFFFFF

/// \brief A double ended queue container.
///
/// The deque allocates elements in fixed size chunks (usually around 4KB per chunk), 
/// but with at least 32 elements per chunk. It will allocate additional chunks when necessary, but never reallocate
/// already existing data. Therefore it is well suited when it is not known up front how many elements are needed,
/// as it can start with a small amount of data and grow on demand. Also an element in a deque is never moved around
/// in memory, thus it is safe to store pointers to elements inside the deque.
/// The deque uses one redirection to look up elements, for which it uses one index array. Thus a look up into a deque
/// is slightly slower than a pure array lookup, but still very fast, compared to all other containers.
/// Deques can easily grow and shrink at both ends and are thus well suited for use as a FIFO or LIFO queue or even
/// a ring-buffer. It is optimized to even handle the ring-buffer use case without any memory allocations, as long as
/// the buffer does not need to grow.
template <typename T, bool Construct>
class ezDequeBase
{
protected:

  /// \brief No memory is allocated during construction.
  ezDequeBase(ezAllocatorBase* pAllocator); // [tested]

  /// \brief Constructs this deque by copying from rhs.
  ezDequeBase(const ezDequeBase<T, Construct>& rhs, ezAllocatorBase* pAllocator); // [tested]

  /// \brief Constructs this deque by moving from rhs.
  ezDequeBase(ezDequeBase<T, Construct>&& rhs, ezAllocatorBase* pAllocator); // [tested]

  /// \brief Destructor.
  ~ezDequeBase(); // [tested]

  /// \brief Assignment operator.
  void operator=(const ezDequeBase<T, Construct>& rhs); // [tested]

  /// \brief Move operator.
  void operator=(ezDequeBase<T, Construct>&& rhs); // [tested]

public:
  /// \brief Destructs all elements and sets the count to zero. Does not deallocate any data.
  void Clear(); // [tested]

  /// \brief Rearranges the internal data structures such that the amount of reserved elements can be appended with as few allocations, as possible.
  ///
  /// This does not reserve the actual amount of chunks, that would be needed, but only grows the index array 
  /// (for redirections) as much as needed.
  /// Thus you can use it to reserve enough bookkeeping storage, without actually allocating all that data.
  /// In contrast to the ezDynamicArray and ezHybridArray containers, ezDeque does not require you to reserve space
  /// up front, to be fast. However, if useful information is available, 'Reserve' can be used to prevent a few
  /// unnecessary reallocations of its internal data structures.
  void Reserve(ezUInt32 uiCount); // [tested]

  /// \brief This function deallocates as much memory as possible to shrink the deque to the bare minimum size that it needs to work.
  ///
  /// This function is never called internally. It is only for the user to trigger a size reduction when he feels like it.
  /// The index array data is not reduced as much as possible, a bit spare memory is kept to allow for scaling the deque
  /// up again, without immediate reallocation of all data structures.
  /// If the deque is completely empty, ALL data is completely deallocated.
  void Compact(); // [tested]

  /// \brief Sets the number of active elements in the deque. All new elements are default constructed. If the deque is shrunk, elements at the end of the deque are destructed.
  void SetCount(ezUInt32 uiCount); // [tested]

  /// \brief Accesses the n-th element in the deque.
  T& operator[](ezUInt32 uiIndex); // [tested]

  /// \brief Accesses the n-th element in the deque.
  const T& operator[](ezUInt32 uiIndex) const; // [tested]

  /// \brief Grows the deque by one element and returns a reference to the newly created element.
  T& ExpandAndGetRef(); // [tested]

  /// \brief Adds one default constructed element to the back of the deque.
  void PushBack(); // [tested]

  /// \brief Adds one element to the back of the deque.
  void PushBack(const T& element); // [tested]

  /// \brief Removes the last element from the deque.
  void PopBack(ezUInt32 uiElements = 1); // [tested]

  /// \brief Adds one element to the front of the deque.
  void PushFront(const T& element); // [tested]

  /// \brief Adds one default constructed element to the front of the deque.
  void PushFront(); // [tested]

  /// \brief Removes the first element from the deque.
  void PopFront(ezUInt32 uiElements = 1); // [tested]

  /// \brief Checks whether no elements are active in the deque.
  bool IsEmpty() const; // [tested]

  /// \brief Returns the number of active elements in the deque.
  ezUInt32 GetCount() const; // [tested]

  /// \brief Returns the first element.
  const T& PeekFront() const; // [tested]

  /// \brief Returns the first element.
  T& PeekFront(); // [tested]

  /// \brief Returns the last element.
  const T& PeekBack() const; // [tested]

  /// \brief Returns the last element.
  T& PeekBack(); // [tested]

  /// \brief Checks whether there is any element in the deque with the given value.
  bool Contains(const T& value) const; // [tested]

  /// \brief Returns the first index at which an element with the given value could be found or ezInvalidIndex if nothing was found.
  ezUInt32 IndexOf(const T& value, ezUInt32 uiStartIndex = 0) const; // [tested]

  /// \brief Returns the last index at which an element with the given value could be found or ezInvalidIndex if nothing was found.
  ezUInt32 LastIndexOf(const T& value, ezUInt32 uiStartIndex = ezInvalidIndex) const; // [tested]

  /// \brief Removes the element at the given index and fills the gap with the last element in the deque.
  void RemoveAtSwap(ezUInt32 uiIndex); // [tested]

  /// \brief Removes the element at index and fills the gap by shifting all following elements
  void RemoveAt(ezUInt32 uiIndex); // [tested]

  /// \brief Removes the first occurrence of value and fills the gap by shifting all following elements
  bool Remove(const T& value); // [tested]

  /// \brief Removes the first occurrence of value and fills the gap with the last element in the deque.
  bool RemoveSwap(const T& value); // [tested]

  /// \brief Inserts value at index by shifting all following elements. Valid insert positions are [0; GetCount].
  void Insert(const T& value, ezUInt32 uiIndex); // [tested]

  /// \brief Sort with explicit comparer
  template <typename Comparer>
  void Sort(const Comparer& comparer); // [tested]

  /// \brief Sort with default comparer
  void Sort(); // [tested]

  /// \brief Returns the allocator that is used by this instance.
  ezAllocatorBase* GetAllocator() const { return m_pAllocator; }

  typedef const_iterator_base<ezDequeBase<T, Construct>, T, false> const_iterator;
  typedef const_iterator_base<ezDequeBase<T, Construct>, T, true> const_reverse_iterator;
  typedef iterator_base<ezDequeBase<T, Construct>, T, false> iterator;
  typedef iterator_base<ezDequeBase<T, Construct>, T, true> reverse_iterator;

  /// \brief Returns the number of elements after uiStartIndex that are stored in contiguous memory.
  ///
  /// That means one can do a memcpy or memcmp from position uiStartIndex up until uiStartIndex + range.
  ezUInt32 GetContiguousRange(ezUInt32 uiStartIndex) const; // [tested]

  /// \brief Comparison operator
  bool operator==(const ezDequeBase<T, Construct>& rhs) const; // [tested]

  /// \brief Comparison operator
  bool operator!=(const ezDequeBase<T, Construct>& rhs) const; // [tested]

  /// \brief Returns the amount of bytes that are currently allocated on the heap.
  ezUInt64 GetHeapMemoryUsage() const; // [tested]

private:

  /// \brief A common constructor function.
  void Constructor(ezAllocatorBase* pAllocator);

  /// \brief Reduces the index array to take up less memory.
  void CompactIndexArray(ezUInt32 uiMinChunksToKeep);

  /// \brief Moves the index chunk array to the left by swapping all elements from right to left
  void MoveIndexChunksLeft(ezUInt32 uiChunkDiff);

  /// \brief Moves the index chunk array to the right by swapping all elements from left to right
  void MoveIndexChunksRight(ezUInt32 uiChunkDiff);

  /// \brief Computes which chunk contains the element at index 0
  ezUInt32 GetFirstUsedChunk() const;

  /// \brief Computes which chunk would contain the last element if the deque had 'uiAtSize' elements (m_uiCount), returns the chunk of element 0, if the deque is currently empty.
  ezUInt32 GetLastUsedChunk(ezUInt32 uiAtSize) const;

  /// \brief Returns which chunk contains the currently last element.
  ezUInt32 GetLastUsedChunk() const;

  /// \brief Computes how many chunks would be required if the deque had a size of 'uiAtSize' (the value of m_uiFirstElement) affects the result.
  ezUInt32 GetRequiredChunks(ezUInt32 uiAtSize) const;

  /// \brief Goes through all the unused chunks and deallocates chunks until no more than 'uiMaxChunks' are still allocated (in total).
  void DeallocateUnusedChunks(ezUInt32 uiMaxChunks);

  /// \brief Resets the counter when the next size reduction will be done.
  void ResetReduceSizeCounter();

  /// \brief This function will periodically deallocate unused chunks to prevent unlimited memory usage. 
  ///
  /// However it tries not to deallocate too much, so it does not immediately deallocate 'everything'.
  /// Instead the deque will track its upper limited of used elements in between size reductions and keep enough
  /// chunks allocated to fulfill those requirements.
  /// If after a large amount of data was in use, the deque's size decreases drastically, the amount of chunks
  /// is reduced gradually (but not immediately).
  /// Size reductions are mostly triggered by PopBack and PopFront (but also by SetCount).
  /// The worst case scenario should be this:
  /// * SetCount(very large amount) -> allocates lots of chunks
  /// * PopBack / PopFront until the deque is empty -> every once in a while a reduction is triggered, this will deallocate a few chunks every time
  /// * PushBack / PopBack for a long time -> Size does not change, but the many PopBack calls will trigger reductions 
  ///    -> The number of allocated chunks will shrink over time until no more than the required number of chunks (+2) remains
  /// * SetCount(very large amount) -> lots of chunks need to be allocated again
  void ReduceSize(ezInt32 iReduction);

  /// \brief Computes how many elements could be handled without rearranging the index array
  ezUInt32 GetCurMaxCount() const;

  /// \brief Searches through the unused chunks for an allocated chunk and returns it. Allocates a new chunk if necessary.
  T* GetUnusedChunk();

  /// \brief Returns a reference to the element at the given index. Makes sure the chunk that should contain that element is allocated. Used before elements are constructed.
  T& ElementAt(ezUInt32 uiIndex);

  /// \brief Deallocates all data, resets the deque to the state after construction.
  void DeallocateAll();

  ezAllocatorBase* m_pAllocator;
  T** m_pChunks;                ///< The chunk index array for redirecting accesses. Not all chunks must be allocated.
  ezUInt32 m_uiChunks;          ///< The size of the m_pChunks array. Determines how many elements could theoretically be stored in the deque.
  ezUInt32 m_uiFirstElement;    ///< Which element (across all chunks) is considered to be the first.
  ezUInt32 m_uiCount;           ///< How many elements are actually active at the moment.
  ezUInt32 m_uiAllocatedChunks; ///< How many entries in the m_pChunks array are allocated at the moment.
  ezInt32 m_iReduceSizeTimer;   ///< Every time this counter reaches zero, a 'garbage collection' step is performed, which might deallocate chunks.
  ezUInt32 m_uiMaxCount;        ///< How many elements were maximally active since the last 'garbage collection' to prevent deallocating too much memory.

#if EZ_ENABLED(EZ_COMPILE_FOR_DEBUG)
  ezUInt32 m_uiChunkSize; // needed for debugger visualization
#endif
};

/// \brief \see ezDequeBase
template <typename T, typename AllocatorWrapper = ezDefaultAllocatorWrapper, bool Construct = true>
class ezDeque : public ezDequeBase<T, Construct>
{
public:
  ezDeque();
  ezDeque(ezAllocatorBase* pAllocator);

  ezDeque(const ezDeque<T, AllocatorWrapper, Construct>& other);
  ezDeque(const ezDequeBase<T, Construct>& other);

  ezDeque(ezDeque<T, AllocatorWrapper, Construct>&& other);
  ezDeque(ezDequeBase<T, Construct>&& other);

  void operator=(const ezDeque<T, AllocatorWrapper, Construct>& rhs);
  void operator=(const ezDequeBase<T, Construct>& rhs);

  void operator=(ezDeque<T, AllocatorWrapper, Construct>&& rhs);
  void operator=(ezDequeBase<T, Construct>&& rhs);
};

template <typename T, bool Construct>
typename ezDequeBase<T, Construct>::iterator begin(ezDequeBase<T, Construct>& container) { return typename ezDequeBase<T, Construct>::iterator(container, (size_t) 0); }

template <typename T, bool Construct>
typename ezDequeBase<T, Construct>::const_iterator  begin(const ezDequeBase<T, Construct>& container) { return typename ezDequeBase<T, Construct>::const_iterator(container, (size_t) 0); }

template <typename T, bool Construct>
typename ezDequeBase<T, Construct>::const_iterator cbegin(const ezDequeBase<T, Construct>& container) { return typename ezDequeBase<T, Construct>::const_iterator(container, (size_t) 0); }

template <typename T, bool Construct>
typename ezDequeBase<T, Construct>::reverse_iterator rbegin(ezDequeBase<T, Construct>& container) { return typename ezDequeBase<T, Construct>::reverse_iterator(container, (size_t) 0); }

template <typename T, bool Construct>
typename ezDequeBase<T, Construct>::const_reverse_iterator rbegin(const ezDequeBase<T, Construct>& container) { return typename ezDequeBase<T, Construct>::const_reverse_iterator(container, (size_t) 0); }

template <typename T, bool Construct>
typename ezDequeBase<T, Construct>::const_reverse_iterator crbegin(const ezDequeBase<T, Construct>& container) { return typename ezDequeBase<T, Construct>::const_reverse_iterator(container, (size_t) 0); }

template <typename T, bool Construct>
typename ezDequeBase<T, Construct>::iterator end(ezDequeBase<T, Construct>& container) { return typename ezDequeBase<T, Construct>::iterator(container, (size_t) container.GetCount()); }

template <typename T, bool Construct>
typename ezDequeBase<T, Construct>::const_iterator end(const ezDequeBase<T, Construct>& container) { return typename ezDequeBase<T, Construct>::const_iterator(container, (size_t) container.GetCount()); }

template <typename T, bool Construct>
typename ezDequeBase<T, Construct>::const_iterator cend(const ezDequeBase<T, Construct>& container) { return typename ezDequeBase<T, Construct>::const_iterator(container, (size_t) container.GetCount()); }

template <typename T, bool Construct>
typename ezDequeBase<T, Construct>::reverse_iterator rend(ezDequeBase<T, Construct>& container) { return typename ezDequeBase<T, Construct>::reverse_iterator(container, (size_t) container.GetCount()); }

template <typename T, bool Construct>
typename ezDequeBase<T, Construct>::const_reverse_iterator  rend(const ezDequeBase<T, Construct>& container) { return typename ezDequeBase<T, Construct>::const_reverse_iterator(container, (size_t) container.GetCount()); }

template <typename T, bool Construct>
typename ezDequeBase<T, Construct>::const_reverse_iterator crend(const ezDequeBase<T, Construct>& container) { return typename ezDequeBase<T, Construct>::const_reverse_iterator(container, (size_t) container.GetCount()); }

#include <Foundation/Containers/Implementation/Deque_inl.h>

