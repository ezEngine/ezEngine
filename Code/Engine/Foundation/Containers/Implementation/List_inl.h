#pragma once

#include <Foundation/Math/Math.h>

// **** ListElement ****

template <typename T>
ezList<T>::ListElementBase::ListElementBase() : m_pPrev(NULL), m_pNext(NULL)
{
}

template <typename T>
ezList<T>::ListElement::ListElement(const T& data) : m_Data(data)
{
}

// **** ezList ****

template <typename T>
ezList<T>::ezList(ezIAllocator* pAllocator /* = ezFoundation::GetDefaultAllocator() */) : m_uiCount(0), m_End(reinterpret_cast<ListElement*>(&m_Last)), m_Elements(pAllocator), m_pFreeElementStack(NULL)
{
  m_First.m_pNext = reinterpret_cast<ListElement*>(&m_Last);
  m_Last.m_pPrev  = reinterpret_cast<ListElement*>(&m_First);
}

template <typename T>
ezList<T>::ezList(const ezList<T>& cc) : m_uiCount(0), m_End(reinterpret_cast<ListElement*>(&m_Last)), m_Elements(cc.m_Elements.GetAllocator()), m_pFreeElementStack(NULL)
{
  m_First.m_pNext = reinterpret_cast<ListElement*>(&m_Last);
  m_Last.m_pPrev  = reinterpret_cast<ListElement*>(&m_First);

  operator=(cc);
}

template <typename T>
ezList<T>::~ezList()
{
  Clear();
}

template <typename T>
void ezList<T>::operator=(const ezList<T>& cc)
{
  Clear();
  Insert(GetIterator(), cc.GetIterator(), cc.GetEndIterator());
}

template <typename T>
typename ezList<T>::ListElement* ezList<T>::AcquireNode(const T& data)
{
  ListElement* pNode;

  if (m_pFreeElementStack == NULL)
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
void ezList<T>::ReleaseNode(ListElement* pNode)
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
EZ_FORCE_INLINE typename ezList<T>::Iterator ezList<T>::GetIterator()
{
  return Iterator(m_First.m_pNext);
}

template <typename T>
EZ_FORCE_INLINE typename ezList<T>::Iterator ezList<T>::GetLastIterator()
{
  return Iterator(m_Last.m_pPrev);
}

template <typename T>
EZ_FORCE_INLINE typename ezList<T>::Iterator ezList<T>::GetEndIterator()
{
  return m_End;
}

template <typename T>
EZ_FORCE_INLINE typename ezList<T>::ConstIterator ezList<T>::GetIterator() const
{
  return ConstIterator(m_First.m_pNext);
}

template <typename T>
EZ_FORCE_INLINE typename ezList<T>::ConstIterator ezList<T>::GetLastIterator() const
{
  return ConstIterator(m_Last.m_pPrev);
}

template <typename T>
EZ_FORCE_INLINE typename ezList<T>::ConstIterator ezList<T>::GetEndIterator() const
{
  return m_End;
}

template <typename T>
EZ_FORCE_INLINE ezUInt32 ezList<T>::GetCount() const
{
  return m_uiCount;
}

template <typename T>
EZ_FORCE_INLINE bool ezList<T>::IsEmpty() const
{
  return (m_uiCount == 0);
}

template <typename T>
void ezList<T>::Clear()
{
  if (!IsEmpty())
    Erase(GetIterator(), GetEndIterator());

  m_pFreeElementStack = NULL;
  m_Elements.Clear();
}

template <typename T>
EZ_FORCE_INLINE T& ezList<T>::PeekFront()
{
  EZ_ASSERT(!IsEmpty(), "The container is empty.");

  return m_First.m_pNext->m_Data;
}

template <typename T>
EZ_FORCE_INLINE T& ezList<T>::PeekBack()
{
  EZ_ASSERT(!IsEmpty(), "The container is empty.");

  return m_Last.m_pPrev->m_Data;
}

template <typename T>
EZ_FORCE_INLINE const T& ezList<T>::PeekFront() const
{
  EZ_ASSERT(!IsEmpty(), "The container is empty.");

  return m_First.m_pNext->m_Data;
}

template <typename T>
EZ_FORCE_INLINE const T& ezList<T>::PeekBack() const
{
  EZ_ASSERT(!IsEmpty(), "The container is empty.");

  return m_Last.m_pPrev->m_Data;
}


template <typename T>
EZ_FORCE_INLINE void ezList<T>::PushBack()
{
  PushBack(T());
}

template <typename T>
EZ_FORCE_INLINE void ezList<T>::PushBack(const T& element)
{
  Insert(GetEndIterator(), element);
}

template <typename T>
EZ_FORCE_INLINE void ezList<T>::PushFront()
{
  PushFront(T());
}

template <typename T>
EZ_FORCE_INLINE void ezList<T>::PushFront(const T& element)
{
  Insert(GetIterator(), element);
}

template <typename T>
EZ_FORCE_INLINE void ezList<T>::PopBack()
{
  EZ_ASSERT(!IsEmpty(), "The container is empty.");

  Erase(Iterator(m_Last.m_pPrev));
}

template <typename T>
void ezList<T>::PopFront()
{
  EZ_ASSERT(!IsEmpty(), "The container is empty.");

  Erase (Iterator(m_First.m_pNext));
}

template <typename T>
typename ezList<T>::Iterator ezList<T>::Insert(const Iterator& pos, const T& data)
{
  EZ_ASSERT(pos.m_pElement != NULL, "The iterator (pos) is invalid.");

  ++m_uiCount;
  ListElement* elem = AcquireNode(data);

  elem->m_pNext = pos.m_pElement;
  elem->m_pPrev = pos.m_pElement->m_pPrev;

  pos.m_pElement->m_pPrev->m_pNext = elem;
  pos.m_pElement->m_pPrev = elem;

  return Iterator(elem);
}

template <typename T>
void ezList<T>::Insert(const Iterator& pos, ConstIterator first, const ConstIterator& last)
{
  EZ_ASSERT(pos.m_pElement != NULL, "The iterator (pos) is invalid.");
  EZ_ASSERT(first.m_pElement != NULL, "The iterator (first) is invalid.");
  EZ_ASSERT(last.m_pElement != NULL, "The iterator (last) is invalid.");

  while (first != last)
  {
    Insert(pos, *first);
    ++first;
  }
}

template <typename T>
typename ezList<T>::Iterator ezList<T>::Erase(const Iterator& pos)
{
  EZ_ASSERT(!IsEmpty(), "The container is empty.");
  EZ_ASSERT(pos.m_pElement != NULL, "The iterator (pos) is invalid.");

  ListElement* pPrev = pos.m_pElement->m_pPrev;
  ListElement* pNext = pos.m_pElement->m_pNext;

  pPrev->m_pNext = pNext;
  pNext->m_pPrev = pPrev;

  ReleaseNode(pos.m_pElement);

  return Iterator(pNext);
}

template <typename T>
typename ezList<T>::Iterator ezList<T>::Erase(Iterator first, const Iterator& last)
{
  EZ_ASSERT(!IsEmpty(), "The container is empty.");
  EZ_ASSERT(first.m_pElement != NULL, "The iterator (first) is invalid.");
  EZ_ASSERT(last.m_pElement != NULL, "The iterator (last) is invalid.");

  while (first != last)
    first = Erase(first);

  return last;
}

/*! If uiNewSize is smaller than the size of the list, elements are popped from the back, until the desired size is reached.
    If uiNewSize is larger than the size of the list, default-constructed elements are appended to the list, until the desired size is reached.
*/
template <typename T>
void ezList<T>::SetCount(ezUInt32 uiNewSize)
{
  while (m_uiCount > uiNewSize)
    PopBack();

  while (m_uiCount < uiNewSize)
    PushBack();
}

