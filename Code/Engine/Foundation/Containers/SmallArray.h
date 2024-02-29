#pragma once

#include <Foundation/Algorithm/Sorting.h>
#include <Foundation/Math/Math.h>
#include <Foundation/Memory/AllocatorWrapper.h>
#include <Foundation/Types/ArrayPtr.h>

constexpr ezUInt32 ezSmallInvalidIndex = 0xFFFF;

/// \brief Implementation of a dynamically growing array with in-place storage and small memory overhead.
///
/// Best-case performance for the PushBack operation is in O(1) if the ezHybridArray does not need to be expanded.
/// In the worst case, PushBack is in O(n).
/// Look-up is guaranteed to always be in O(1).
template <typename T, ezUInt16 Size>
class ezSmallArrayBase
{
public:
  // Only if the stored type is either POD or relocatable the hybrid array itself is also relocatable.
  EZ_DECLARE_MEM_RELOCATABLE_TYPE_CONDITIONAL(T);

  ezSmallArrayBase();                                                                // [tested]
  ezSmallArrayBase(const ezSmallArrayBase<T, Size>& other, ezAllocator* pAllocator); // [tested]
  ezSmallArrayBase(const ezArrayPtr<const T>& other, ezAllocator* pAllocator);       // [tested]
  ezSmallArrayBase(ezSmallArrayBase<T, Size>&& other, ezAllocator* pAllocator);      // [tested]

  ~ezSmallArrayBase();                                                               // [tested]

  // Can't use regular assignment operators since we need to pass an allocator. Use CopyFrom or MoveFrom methods instead.
  void operator=(const ezSmallArrayBase<T, Size>& rhs) = delete;
  void operator=(ezSmallArrayBase<T, Size>&& rhs) = delete;

  /// \brief Copies the data from some other array into this one.
  void CopyFrom(const ezArrayPtr<const T>& other, ezAllocator* pAllocator); // [tested]

  /// \brief Moves the data from some other array into this one.
  void MoveFrom(ezSmallArrayBase<T, Size>&& other, ezAllocator* pAllocator); // [tested]

  /// \brief Conversion to const ezArrayPtr.
  operator ezArrayPtr<const T>() const; // [tested]

  /// \brief Conversion to ezArrayPtr.
  operator ezArrayPtr<T>(); // [tested]

  /// \brief Compares this array to another contiguous array type.
  bool operator==(const ezSmallArrayBase<T, Size>& rhs) const; // [tested]
  EZ_ADD_DEFAULT_OPERATOR_NOTEQUAL(const ezSmallArrayBase<T, Size>&);

#if EZ_DISABLED(EZ_USE_CPP20_OPERATORS)
  bool operator==(const ezArrayPtr<const T>& rhs) const; // [tested]
  EZ_ADD_DEFAULT_OPERATOR_NOTEQUAL(const ezArrayPtr<const T>&);
#endif

  /// \brief Compares this array to another contiguous array type.
  bool operator<(const ezSmallArrayBase<T, Size>& rhs) const; // [tested]
  bool operator<(const ezArrayPtr<const T>& rhs) const; // [tested]

  /// \brief Returns the element at the given index. Does bounds checks in debug builds.
  const T& operator[](ezUInt32 uiIndex) const; // [tested]

  /// \brief Returns the element at the given index. Does bounds checks in debug builds.
  T& operator[](ezUInt32 uiIndex); // [tested]

  /// \brief Resizes the array to have exactly uiCount elements. Default constructs extra elements if the array is grown.
  void SetCount(ezUInt16 uiCount, ezAllocator* pAllocator); // [tested]

  /// \brief Resizes the array to have exactly uiCount elements. Constructs all new elements by copying the FillValue.
  void SetCount(ezUInt16 uiCount, const T& fillValue, ezAllocator* pAllocator); // [tested]

  /// \brief Resizes the array to have exactly uiCount elements. Extra elements might be uninitialized.
  template <typename = void>                                             // Template is used to only conditionally compile this function in when it is actually used.
  void SetCountUninitialized(ezUInt16 uiCount, ezAllocator* pAllocator); // [tested]

  /// \brief Ensures the container has at least \a uiCount elements. Ie. calls SetCount() if the container has fewer elements, does nothing
  /// otherwise.
  void EnsureCount(ezUInt16 uiCount, ezAllocator* pAllocator); // [tested]

  /// \brief Returns the number of active elements in the array.
  ezUInt32 GetCount() const; // [tested]

  /// \brief Returns true, if the array does not contain any elements.
  bool IsEmpty() const; // [tested]

  /// \brief Clears the array.
  void Clear(); // [tested]

  /// \brief Checks whether the given value can be found in the array. O(n) complexity.
  bool Contains(const T& value) const; // [tested]

  /// \brief Inserts value at index by shifting all following elements.
  void Insert(const T& value, ezUInt32 uiIndex, ezAllocator* pAllocator); // [tested]

  /// \brief Inserts value at index by shifting all following elements.
  void Insert(T&& value, ezUInt32 uiIndex, ezAllocator* pAllocator); // [tested]

  /// \brief Removes the first occurrence of value and fills the gap by shifting all following elements
  bool RemoveAndCopy(const T& value); // [tested]

  /// \brief Removes the first occurrence of value and fills the gap by swapping in the last element
  bool RemoveAndSwap(const T& value); // [tested]

  /// \brief Removes the element at index and fills the gap by shifting all following elements
  void RemoveAtAndCopy(ezUInt32 uiIndex, ezUInt16 uiNumElements = 1); // [tested]

  /// \brief Removes the element at index and fills the gap by swapping in the last element
  void RemoveAtAndSwap(ezUInt32 uiIndex, ezUInt16 uiNumElements = 1); // [tested]

  /// \brief Searches for the first occurrence of the given value and returns its index or ezInvalidIndex if not found.
  ezUInt32 IndexOf(const T& value, ezUInt32 uiStartIndex = 0) const; // [tested]

  /// \brief Searches for the last occurrence of the given value and returns its index or ezInvalidIndex if not found.
  ezUInt32 LastIndexOf(const T& value, ezUInt32 uiStartIndex = ezSmallInvalidIndex) const; // [tested]

  /// \brief Grows the array by one element and returns a reference to the newly created element.
  T& ExpandAndGetRef(ezAllocator* pAllocator); // [tested]

  /// \brief Pushes value at the end of the array.
  void PushBack(const T& value, ezAllocator* pAllocator); // [tested]

  /// \brief Pushes value at the end of the array.
  void PushBack(T&& value, ezAllocator* pAllocator); // [tested]

  /// \brief Pushes value at the end of the array. Does NOT ensure capacity.
  void PushBackUnchecked(const T& value); // [tested]

  /// \brief Pushes value at the end of the array. Does NOT ensure capacity.
  void PushBackUnchecked(T&& value); // [tested]

  /// \brief Pushes all elements in range at the end of the array. Increases the capacity if necessary.
  void PushBackRange(const ezArrayPtr<const T>& range, ezAllocator* pAllocator); // [tested]

  /// \brief Removes count elements from the end of the array.
  void PopBack(ezUInt32 uiCountToRemove = 1); // [tested]

  /// \brief Returns the last element of the array.
  T& PeekBack(); // [tested]

  /// \brief Returns the last element of the array.
  const T& PeekBack() const; // [tested]

  /// \brief Sort with explicit comparer
  template <typename Comparer>
  void Sort(const Comparer& comparer); // [tested]

  /// \brief Sort with default comparer
  void Sort(); // [tested]

  /// \brief Returns a pointer to the array data, or nullptr if the array is empty.
  T* GetData();

  /// \brief Returns a pointer to the array data, or nullptr if the array is empty.
  const T* GetData() const;

  /// \brief Returns an array pointer to the array data, or an empty array pointer if the array is empty.
  ezArrayPtr<T> GetArrayPtr(); // [tested]

  /// \brief Returns an array pointer to the array data, or an empty array pointer if the array is empty.
  ezArrayPtr<const T> GetArrayPtr() const; // [tested]

  /// \brief Returns a byte array pointer to the array data, or an empty array pointer if the array is empty.
  ezArrayPtr<typename ezArrayPtr<T>::ByteType> GetByteArrayPtr(); // [tested]

  /// \brief Returns a byte array pointer to the array data, or an empty array pointer if the array is empty.
  ezArrayPtr<typename ezArrayPtr<const T>::ByteType> GetByteArrayPtr() const; // [tested]

  /// \brief Expands the array so it can at least store the given capacity.
  void Reserve(ezUInt16 uiCapacity, ezAllocator* pAllocator); // [tested]

  /// \brief Tries to compact the array to avoid wasting memory. The resulting capacity is at least 'GetCount' (no elements get removed). Will
  /// deallocate all data, if the array is empty.
  void Compact(ezAllocator* pAllocator); // [tested]

  /// \brief Returns the reserved number of elements that the array can hold without reallocating.
  ezUInt32 GetCapacity() const { return m_uiCapacity; }

  /// \brief Returns the amount of bytes that are currently allocated on the heap.
  ezUInt64 GetHeapMemoryUsage() const; // [tested]

  using value_type = T;
  using const_reference = const T&;
  using const_iterator = const T*;
  using const_reverse_iterator = const_reverse_pointer_iterator<T>;
  using iterator = T*;
  using reverse_iterator = reverse_pointer_iterator<T>;

  template <typename U>
  const U& GetUserData() const; // [tested]

  template <typename U>
  U& GetUserData();             // [tested]

protected:
  enum
  {
    CAPACITY_ALIGNMENT = 4
  };

  void SetCapacity(ezUInt16 uiCapacity, ezAllocator* pAllocator);

  T* GetElementsPtr();
  const T* GetElementsPtr() const;

  ezUInt16 m_uiCount = 0;
  ezUInt16 m_uiCapacity = Size;

  ezUInt32 m_uiUserData = 0;

  union
  {
    struct alignas(EZ_ALIGNMENT_OF(T))
    {
      ezUInt8 m_StaticData[Size * sizeof(T)];
    };

    T* m_pElements = nullptr;
  };
};

//////////////////////////////////////////////////////////////////////////

/// \brief \see ezSmallArrayBase
template <typename T, ezUInt16 Size, typename AllocatorWrapper = ezDefaultAllocatorWrapper>
class ezSmallArray : public ezSmallArrayBase<T, Size>
{
  using SUPER = ezSmallArrayBase<T, Size>;

public:
  // Only if the stored type is either POD or relocatable the hybrid array itself is also relocatable.
  EZ_DECLARE_MEM_RELOCATABLE_TYPE_CONDITIONAL(T);

  ezSmallArray();

  ezSmallArray(const ezSmallArray<T, Size, AllocatorWrapper>& other);
  explicit ezSmallArray(const ezArrayPtr<const T>& other);
  ezSmallArray(ezSmallArray<T, Size, AllocatorWrapper>&& other);

  ~ezSmallArray();

  void operator=(const ezSmallArray<T, Size, AllocatorWrapper>& rhs);
  void operator=(const ezArrayPtr<const T>& rhs);
  void operator=(ezSmallArray<T, Size, AllocatorWrapper>&& rhs) noexcept;

  void SetCount(ezUInt16 uiCount);                      // [tested]
  void SetCount(ezUInt16 uiCount, const T& fillValue);  // [tested]
  void EnsureCount(ezUInt16 uiCount);                   // [tested]

  template <typename = void>
  void SetCountUninitialized(ezUInt16 uiCount);         // [tested]

  void InsertAt(ezUInt32 uiIndex, const T& value);      // [tested]
  void InsertAt(ezUInt32 uiIndex, T&& value);           // [tested]

  T& ExpandAndGetRef();                                 // [tested]
  void PushBack(const T& value);                        // [tested]
  void PushBack(T&& value);                             // [tested]
  void PushBackRange(const ezArrayPtr<const T>& range); // [tested]

  void Reserve(ezUInt16 uiCapacity);
  void Compact();
};

#include <Foundation/Containers/Implementation/SmallArray_inl.h>
