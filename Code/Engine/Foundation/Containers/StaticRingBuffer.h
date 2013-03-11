#pragma once

#include <Foundation/Containers/StaticArray.h>

/// A ring-buffer container that will use a static array of a given capacity to cycle through elements.
/// If you need a dynamic ring-buffer, use an ezDeque.
template <typename T, ezUInt32 Capacity>
class ezStaticRingBuffer
{
public:
  EZ_CHECK_AT_COMPILETIME_MSG(Capacity > 1, "ORLY?");

  /// Constructs an empty ring-buffer.
  ezStaticRingBuffer(); // [tested]

  /// Copies the content from rhs into this ring-buffer.
  ezStaticRingBuffer(const ezStaticRingBuffer<T, Capacity>& rhs); // [tested]

  /// Destructs all remaining elements.
  ~ezStaticRingBuffer(); // [tested]

  /// Copies the content from rhs into this ring-buffer.
  void operator=(const ezStaticRingBuffer<T, Capacity>& rhs); // [tested]

  /// Compares two ring-buffers for equality.
  bool operator==(const ezStaticRingBuffer<T, Capacity>& rhs) const; // [tested]

  /// Compares two ring-buffers for inequality.
  bool operator!=(const ezStaticRingBuffer<T, Capacity>& rhs) const; // [tested]

  /// Appends an element at the end of the ring-buffer.
  void PushBack(const T& element); // [tested]

  /// Removes the oldest element from the ring-buffer.
  void PopFront(ezUInt32 uiElements = 1); // [tested]

  /// Accesses the oldest element in the ring-buffer.
  const T& PeekFront() const; // [tested]

  /// Accesses the oldest element in the ring-buffer.
  T& PeekFront(); // [tested]

  /// Accesses the n-th element in the ring-buffer.
  const T& operator[](ezUInt32 uiIndex) const; // [tested]
  
  /// Accesses the n-th element in the ring-buffer.
  T& operator[](ezUInt32 uiIndex); // [tested]

  /// Returns the number of elements that are currently in the ring-buffer.
  ezUInt32 GetCount() const; // [tested]

  /// Returns true if the ring-buffer currently contains no elements.
  bool IsEmpty() const; // [tested]

  /// Returns true, if there the ring-buffer can store at least uiElements additional elements.
  bool CanAppend(ezUInt32 uiElements = 1); // [tested]

  /// Destructs all elements in the ring-buffer.
  void Clear(); // [tested]

private:
  T* GetStaticArray();

  /// The fixed size array.
  struct : ezAligned<EZ_ALIGNMENT_OF(T)>
  {
    ezUInt8 m_Data[Capacity * sizeof(T)];
  };

  T* m_pElements;
  ezUInt32 m_uiCount;
  ezUInt32 m_uiFirstElement;
};

#include <Foundation/Containers/Implementation/StaticRingBuffer_inl.h>


