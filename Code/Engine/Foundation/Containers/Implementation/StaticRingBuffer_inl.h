#pragma once

template <typename T, ezUInt32 C>
ezStaticRingBuffer<T, C>::ezStaticRingBuffer()
{
  m_pElements = GetStaticArray();
  m_uiFirstElement = 0;
  m_uiCount = 0;
}

template <typename T, ezUInt32 C>
ezStaticRingBuffer<T, C>::ezStaticRingBuffer(const ezStaticRingBuffer<T, C>& rhs)
{
  m_pElements = GetStaticArray();
  m_uiFirstElement = 0;
  m_uiCount = 0;

  *this = rhs;
}

template <typename T, ezUInt32 C>
ezStaticRingBuffer<T, C>::~ezStaticRingBuffer()
{
  Clear();
}

template <typename T, ezUInt32 C>
void ezStaticRingBuffer<T, C>::operator=(const ezStaticRingBuffer<T, C>& rhs)
{
  Clear();

  for (ezUInt32 i = 0; i < rhs.GetCount(); ++i)
    PushBack(rhs[i]);
}

template <typename T, ezUInt32 C>
bool ezStaticRingBuffer<T, C>::operator==(const ezStaticRingBuffer<T, C>& rhs) const
{
  if (GetCount() != rhs.GetCount())
    return false;

  for (ezUInt32 i = 0; i < m_uiCount; ++i)
  {
    if ((*this)[i] != rhs[i])
      return false;
  }

  return true;
}

template <typename T, ezUInt32 C>
EZ_FORCE_INLINE bool ezStaticRingBuffer<T, C>::operator!=(const ezStaticRingBuffer<T, C>& rhs) const
{
  return !(*this == rhs);
}

template <typename T, ezUInt32 C>
void ezStaticRingBuffer<T, C>::PushBack(const T& element)
{
  EZ_ASSERT_DEV(CanAppend(), "The ring-buffer is full, no elements can be appended before removing one.");

  const ezUInt32 uiLastElement = (m_uiFirstElement + m_uiCount) % C;

  ezMemoryUtils::Construct(&m_pElements[uiLastElement], element, 1);
  ++m_uiCount;
}

template <typename T, ezUInt32 C>
void ezStaticRingBuffer<T, C>::PopFront(ezUInt32 uiElements)
{
  EZ_ASSERT_DEV(m_uiCount >= uiElements, "The ring-buffer contains %i elements, cannot remove %i elements from it.", m_uiCount, uiElements);

  while (uiElements > 0)
  {
    ezMemoryUtils::Destruct(&m_pElements[m_uiFirstElement], 1);
    ++m_uiFirstElement;
    m_uiFirstElement %= C;
    --m_uiCount;

    --uiElements;
  }
}

template <typename T, ezUInt32 C>
EZ_FORCE_INLINE const T& ezStaticRingBuffer<T, C>::PeekFront() const
{
  EZ_ASSERT_DEV(!IsEmpty(), "The ring-buffer is empty, cannot peek at the first element.");

  return m_pElements[m_uiFirstElement];
}

template <typename T, ezUInt32 C>
EZ_FORCE_INLINE T& ezStaticRingBuffer<T, C>::PeekFront()
{
  EZ_ASSERT_DEV(!IsEmpty(), "The ring-buffer is empty, cannot peek at the first element.");

  return m_pElements[m_uiFirstElement];
}

template <typename T, ezUInt32 C>
EZ_FORCE_INLINE const T& ezStaticRingBuffer<T, C>::operator[](ezUInt32 uiIndex) const
{
  EZ_ASSERT_DEV(uiIndex < m_uiCount, "The ring-buffer only has %i elements, cannot access element %i.", m_uiCount, uiIndex);

  return m_pElements[(m_uiFirstElement + uiIndex) % C];
}

template <typename T, ezUInt32 C>
EZ_FORCE_INLINE T& ezStaticRingBuffer<T, C>::operator[](ezUInt32 uiIndex)
{
  EZ_ASSERT_DEV(uiIndex < m_uiCount, "The ring-buffer only has %i elements, cannot access element %i.", m_uiCount, uiIndex);

  return m_pElements[(m_uiFirstElement + uiIndex) % C];
}

template <typename T, ezUInt32 C>
EZ_FORCE_INLINE ezUInt32 ezStaticRingBuffer<T, C>::GetCount() const
{
  return m_uiCount;
}

template <typename T, ezUInt32 C>
EZ_FORCE_INLINE bool ezStaticRingBuffer<T, C>::IsEmpty() const
{
  return m_uiCount == 0;
}

template <typename T, ezUInt32 C>
EZ_FORCE_INLINE bool ezStaticRingBuffer<T, C>::CanAppend(ezUInt32 uiElements)
{
  return (m_uiCount + uiElements) <= C;
}

template <typename T, ezUInt32 C>
void ezStaticRingBuffer<T, C>::Clear()
{
  while (!IsEmpty())
    PopFront();
}

template <typename T, ezUInt32 C>
EZ_FORCE_INLINE T* ezStaticRingBuffer<T, C>::GetStaticArray()
{
  return reinterpret_cast<T*>(m_Data);
}

