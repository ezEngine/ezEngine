#pragma once

#include <Foundation/Math/Math.h>

// **** ListElement ****

template <typename T>
ezListBase<T>::ListElementBase::ListElementBase() : m_pPrev(nullptr), m_pNext(nullptr)
{
}

template <typename T>
ezListBase<T>::ListElement::ListElement(const T& data) : m_Data(data)
{
}

// **** ezListBase ****

template <typename T>
ezListBase<T>::ezListBase(ezAllocatorBase* pAllocator) : 
  m_End(reinterpret_cast<ListElement*>(&m_Last)),
  m_uiCount(0),
  m_Elements(pAllocator),
  m_pFreeElementStack(nullptr)
{
  m_First.m_pNext = reinterpret_cast<ListElement*>(&m_Last);
  m_Last.m_pPrev  = reinterpret_cast<ListElement*>(&m_First);
}

template <typename T>
ezListBase<T>::ezListBase(const ezListBase<T>& cc, ezAllocatorBase* pAllocator) : 
  m_End(reinterpret_cast<ListElement*>(&m_Last)),
  m_uiCount(0),
  m_Elements(pAllocator),
  m_pFreeElementStack(nullptr)
{
  m_First.m_pNext = reinterpret_cast<ListElement*>(&m_Last);
  m_Last.m_pPrev  = reinterpret_cast<ListElement*>(&m_First);

  operator=(cc);
}

template <typename T>
ezListBase<T>::~ezListBase()
{
  Clear();
}

template <typename T>
void ezListBase<T>::operator=(const ezListBase<T>& cc)
{
  Clear();
  Insert(GetIterator(), cc.GetIterator(), cc.GetEndIterator());
}

template <typename T>
typename ezListBase<T>::ListElement* ezListBase<T>::AcquireNode(const T& data)
{
  ListElement* pNode;

  if (m_pFreeElementStack == nullptr)
  {
    m_Elements.PushBack();
    pNode = &m_Elements.PeekBack();
  }
  else
  {
    pNode = m_pFreeElementStack;
    m_pFreeElementStack = m_pFreeElementStack->m_pNext;
  }

  ezMemoryUtils::Construct<ListElement>(pNode, 1);
  pNode->m_Data = data;
  return pNode;
}

template <typename T>
void ezListBase<T>::ReleaseNode(ListElement* pNode)
{
  ezMemoryUtils::Destruct<ListElement>(pNode, 1);

  if (pNode == &m_Elements.PeekBack())
  {
    m_Elements.PopBack();
  }
  else
  if (pNode == &m_Elements.PeekFront())
  {
    m_Elements.PopFront();
  }
  else
  {
    pNode->m_pNext = m_pFreeElementStack;
    m_pFreeElementStack = pNode;
  }

  --m_uiCount;
}


template <typename T>
EZ_FORCE_INLINE typename ezListBase<T>::Iterator ezListBase<T>::GetIterator()
{
  return Iterator(m_First.m_pNext);
}

template <typename T>
EZ_FORCE_INLINE typename ezListBase<T>::Iterator ezListBase<T>::GetLastIterator()
{
  return Iterator(m_Last.m_pPrev);
}

template <typename T>
EZ_FORCE_INLINE typename ezListBase<T>::Iterator ezListBase<T>::GetEndIterator()
{
  return m_End;
}

template <typename T>
EZ_FORCE_INLINE typename ezListBase<T>::ConstIterator ezListBase<T>::GetIterator() const
{
  return ConstIterator(m_First.m_pNext);
}

template <typename T>
EZ_FORCE_INLINE typename ezListBase<T>::ConstIterator ezListBase<T>::GetLastIterator() const
{
  return ConstIterator(m_Last.m_pPrev);
}

template <typename T>
EZ_FORCE_INLINE typename ezListBase<T>::ConstIterator ezListBase<T>::GetEndIterator() const
{
  return m_End;
}

template <typename T>
EZ_FORCE_INLINE ezUInt32 ezListBase<T>::GetCount() const
{
  return m_uiCount;
}

template <typename T>
EZ_FORCE_INLINE bool ezListBase<T>::IsEmpty() const
{
  return (m_uiCount == 0);
}

template <typename T>
void ezListBase<T>::Clear()
{
  if (!IsEmpty())
    Remove(GetIterator(), GetEndIterator());

  m_pFreeElementStack = nullptr;
  m_Elements.Clear();
}

template <typename T>
EZ_FORCE_INLINE T& ezListBase<T>::PeekFront()
{
  EZ_ASSERT_DEV(!IsEmpty(), "The container is empty.");

  return m_First.m_pNext->m_Data;
}

template <typename T>
EZ_FORCE_INLINE T& ezListBase<T>::PeekBack()
{
  EZ_ASSERT_DEV(!IsEmpty(), "The container is empty.");

  return m_Last.m_pPrev->m_Data;
}

template <typename T>
EZ_FORCE_INLINE const T& ezListBase<T>::PeekFront() const
{
  EZ_ASSERT_DEV(!IsEmpty(), "The container is empty.");

  return m_First.m_pNext->m_Data;
}

template <typename T>
EZ_FORCE_INLINE const T& ezListBase<T>::PeekBack() const
{
  EZ_ASSERT_DEV(!IsEmpty(), "The container is empty.");

  return m_Last.m_pPrev->m_Data;
}


template <typename T>
EZ_FORCE_INLINE void ezListBase<T>::PushBack()
{
  PushBack(T());
}

template <typename T>
EZ_FORCE_INLINE void ezListBase<T>::PushBack(const T& element)
{
  Insert(GetEndIterator(), element);
}

template <typename T>
EZ_FORCE_INLINE void ezListBase<T>::PushFront()
{
  PushFront(T());
}

template <typename T>
EZ_FORCE_INLINE void ezListBase<T>::PushFront(const T& element)
{
  Insert(GetIterator(), element);
}

template <typename T>
EZ_FORCE_INLINE void ezListBase<T>::PopBack()
{
  EZ_ASSERT_DEV(!IsEmpty(), "The container is empty.");

  Remove(Iterator(m_Last.m_pPrev));
}

template <typename T>
void ezListBase<T>::PopFront()
{
  EZ_ASSERT_DEV(!IsEmpty(), "The container is empty.");

  Remove(Iterator(m_First.m_pNext));
}

template <typename T>
typename ezListBase<T>::Iterator ezListBase<T>::Insert(const Iterator& pos, const T& data)
{
  EZ_ASSERT_DEV(pos.m_pElement != nullptr, "The iterator (pos) is invalid.");

  ++m_uiCount;
  ListElement* elem = AcquireNode(data);

  elem->m_pNext = pos.m_pElement;
  elem->m_pPrev = pos.m_pElement->m_pPrev;

  pos.m_pElement->m_pPrev->m_pNext = elem;
  pos.m_pElement->m_pPrev = elem;

  return Iterator(elem);
}

template <typename T>
void ezListBase<T>::Insert(const Iterator& pos, ConstIterator first, const ConstIterator& last)
{
  EZ_ASSERT_DEV(pos.m_pElement != nullptr, "The iterator (pos) is invalid.");
  EZ_ASSERT_DEV(first.m_pElement != nullptr, "The iterator (first) is invalid.");
  EZ_ASSERT_DEV(last.m_pElement != nullptr, "The iterator (last) is invalid.");

  while (first != last)
  {
    Insert(pos, *first);
    ++first;
  }
}

template <typename T>
typename ezListBase<T>::Iterator ezListBase<T>::Remove(const Iterator& pos)
{
  EZ_ASSERT_DEV(!IsEmpty(), "The container is empty.");
  EZ_ASSERT_DEV(pos.m_pElement != nullptr, "The iterator (pos) is invalid.");

  ListElement* pPrev = pos.m_pElement->m_pPrev;
  ListElement* pNext = pos.m_pElement->m_pNext;

  pPrev->m_pNext = pNext;
  pNext->m_pPrev = pPrev;

  ReleaseNode(pos.m_pElement);

  return Iterator(pNext);
}

template <typename T>
typename ezListBase<T>::Iterator ezListBase<T>::Remove(Iterator first, const Iterator& last)
{
  EZ_ASSERT_DEV(!IsEmpty(), "The container is empty.");
  EZ_ASSERT_DEV(first.m_pElement != nullptr, "The iterator (first) is invalid.");
  EZ_ASSERT_DEV(last.m_pElement != nullptr, "The iterator (last) is invalid.");

  while (first != last)
    first = Remove(first);

  return last;
}

/*! If uiNewSize is smaller than the size of the list, elements are popped from the back, until the desired size is reached.
    If uiNewSize is larger than the size of the list, default-constructed elements are appended to the list, until the desired size is reached.
*/
template <typename T>
void ezListBase<T>::SetCount(ezUInt32 uiNewSize)
{
  while (m_uiCount > uiNewSize)
    PopBack();

  while (m_uiCount < uiNewSize)
    PushBack();
}

template <typename T>
bool ezListBase<T>::operator==(const ezListBase<T>& rhs) const
{
  if (GetCount() != rhs.GetCount())
    return false;

  auto itLhs = GetIterator();
  auto itRhs = rhs.GetIterator();

  while (itLhs.IsValid())
  {
    if (*itLhs != *itRhs)
      return false;

    ++itLhs;
    ++itRhs;
  }

  return true;
}

template <typename T>
bool ezListBase<T>::operator!=(const ezListBase<T>& rhs) const
{
  return !operator==(rhs);
}

template <typename T, typename A>
ezList<T, A>::ezList() : ezListBase<T>(A::GetAllocator())
{
}

template <typename T, typename A>
ezList<T, A>::ezList(ezAllocatorBase* pAllocator) : ezListBase<T>(pAllocator)
{
}

template <typename T, typename A>
ezList<T, A>::ezList(const ezList<T, A>& other) : ezListBase<T>(other, A::GetAllocator())
{
}

template <typename T, typename A>
ezList<T, A>::ezList(const ezListBase<T>& other) : ezListBase<T>(other, A::GetAllocator())
{
}

template <typename T, typename A>
void ezList<T, A>::operator=(const ezList<T, A>& rhs)
{
  ezListBase<T>::operator=(rhs);
}

template <typename T, typename A>
void ezList<T, A>::operator=(const ezListBase<T>& rhs)
{
  ezListBase<T>::operator=(rhs);
}

