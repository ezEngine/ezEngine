#pragma once

#include <Foundation/Containers/StaticArray.h>

/// \brief A ring-buffer container that will use a static array of a given capacity to cycle through elements.
///
/// If you need a dynamic ring-buffer, use an ezDeque.
template <typename T, ezUInt32 Capacity>
class ezStaticRingBuffer
{
public:
  EZ_CHECK_AT_COMPILETIME_MSG(Capacity > 1, "ORLY?");

  /// \brief Constructs an empty ring-buffer.
  ezStaticRingBuffer(); // [tested]

  /// \brief Copies the content from rhs into this ring-buffer.
  ezStaticRingBuffer(const ezStaticRingBuffer<T, Capacity>& rhs); // [tested]

  /// \brief Destructs all remaining elements.
  ~ezStaticRingBuffer(); // [tested]

  /// \brief Copies the content from rhs into this ring-buffer.
  void operator=(const ezStaticRingBuffer<T, Capacity>& rhs); // [tested]

  /// \brief Compares two ring-buffers for equality.
  bool operator==(const ezStaticRingBuffer<T, Capacity>& rhs) const; // [tested]

  /// \brief Compares two ring-buffers for inequality.
  bool operator!=(const ezStaticRingBuffer<T, Capacity>& rhs) const; // [tested]

  /// \brief Appends an element at the end of the ring-buffer.
  void PushBack(const T& element); // [tested]

  /// \brief Removes the oldest element from the ring-buffer.
  void PopFront(ezUInt32 uiElements = 1); // [tested]

  /// \brief Accesses the oldest element in the ring-buffer.
  const T& PeekFront() const; // [tested]

  /// \brief Accesses the oldest element in the ring-buffer.
  T& PeekFront(); // [tested]

  /// \brief Accesses the n-th element in the ring-buffer.
  const T& operator[](ezUInt32 uiIndex) const; // [tested]
  
  /// \brief Accesses the n-th element in the ring-buffer.
  T& operator[](ezUInt32 uiIndex); // [tested]

  /// \brief Returns the number of elements that are currently in the ring-buffer.
  ezUInt32 GetCount() const; // [tested]

  /// \brief Returns true if the ring-buffer currently contains no elements.
  bool IsEmpty() const; // [tested]

  /// \brief Returns true, if there the ring-buffer can store at least uiElements additional elements.
  bool CanAppend(ezUInt32 uiElements = 1); // [tested]

  /// \brief Destructs all elements in the ring-buffer.
  void Clear(); // [tested]

private:
  T* GetStaticArray();

  /// \brief The fixed size array.
  struct : ezAligned<EZ_ALIGNMENT_OF(T)>
  {
    ezUInt8 m_Data[Capacity * sizeof(T)];
  };

  T* m_pElements;
  ezUInt32 m_uiCount;
  ezUInt32 m_uiFirstElement;
};

#include <Foundation/Containers/Implementation/StaticRingBuffer_inl.h>


