#pragma once

#include <Foundation/Math/Math.h>

// ***** Const Iterator *****

#define STACK_SIZE 48

template <typename KeyType, typename ValueType, typename Comparer>
void ezMapBase<KeyType, ValueType, Comparer>::ConstIterator::Next()
{
  const ezInt32 dir0 = 0;
  const ezInt32 dir1 = 1;

  if (m_pElement == nullptr)
  {
    EZ_ASSERT_DEV(m_pElement != nullptr, "The Iterator is invalid (end).");
    return;
  }

  // if this element has a right child, go there and then search for the left most child of that
  if (m_pElement->m_pLink[dir1] != m_pElement->m_pLink[dir1]->m_pLink[dir1])
  {
    m_pElement = m_pElement->m_pLink[dir1];

    while (m_pElement->m_pLink[dir0] != m_pElement->m_pLink[dir0]->m_pLink[dir0])
      m_pElement = m_pElement->m_pLink[dir0];

    return;
  }

  // if this element has a parent and this element is that parents left child, go directly to the parent
  if ((m_pElement->m_pParent != m_pElement->m_pParent->m_pParent) &&
      (m_pElement->m_pParent->m_pLink[dir0] == m_pElement))
  {
    m_pElement = m_pElement->m_pParent;
    return;
  }

  // if this element has a parent and this element is that parents right child, search for the next parent, whose left child this is
  if ((m_pElement->m_pParent != m_pElement->m_pParent->m_pParent) &&
      (m_pElement->m_pParent->m_pLink[dir1] == m_pElement))
  {
    while (m_pElement->m_pParent->m_pLink[dir1] == m_pElement)
      m_pElement = m_pElement->m_pParent;

    // if we are at the root node..
    if ((m_pElement->m_pParent == m_pElement->m_pParent->m_pParent) ||
        (m_pElement->m_pParent == nullptr))
    {
      m_pElement = nullptr;
      return;
    }

    m_pElement = m_pElement->m_pParent;
    return;
  }

  m_pElement = nullptr;
  return;
}

template <typename KeyType, typename ValueType, typename Comparer>
void ezMapBase<KeyType, ValueType, Comparer>::ConstIterator::Prev()
{
  const ezInt32 dir0 = 1;
  const ezInt32 dir1 = 0;

  if (m_pElement == nullptr)
  {
    EZ_ASSERT_DEV(m_pElement != nullptr, "The Iterator is invalid (end).");
    return;
  }

  // if this element has a right child, go there and then search for the left most child of that
  if (m_pElement->m_pLink[dir1] != m_pElement->m_pLink[dir1]->m_pLink[dir1])
  {
    m_pElement = m_pElement->m_pLink[dir1];

    while (m_pElement->m_pLink[dir0] != m_pElement->m_pLink[dir0]->m_pLink[dir0])
      m_pElement = m_pElement->m_pLink[dir0];

    return;
  }

  // if this element has a parent and this element is that parents left child, go directly to the parent
  if ((m_pElement->m_pParent != m_pElement->m_pParent->m_pParent) &&
      (m_pElement->m_pParent->m_pLink[dir0] == m_pElement))
  {
    m_pElement = m_pElement->m_pParent;
    return;
  }

  // if this element has a parent and this element is that parents right child, search for the next parent, whose left child this is
  if ((m_pElement->m_pParent != m_pElement->m_pParent->m_pParent) &&
      (m_pElement->m_pParent->m_pLink[dir1] == m_pElement))
  {
    while (m_pElement->m_pParent->m_pLink[dir1] == m_pElement)
      m_pElement = m_pElement->m_pParent;

    // if we are at the root node..
    if ((m_pElement->m_pParent == m_pElement->m_pParent->m_pParent) ||
        (m_pElement->m_pParent == nullptr))
    {
      m_pElement = nullptr;
      return;
    }

    m_pElement = m_pElement->m_pParent;
    return;
  }

  m_pElement = nullptr;
  return;
}

// ***** ezMapBase *****

template <typename KeyType, typename ValueType, typename Comparer>
EZ_FORCE_INLINE ezMapBase<KeyType, ValueType, Comparer>::NilNode::NilNode() : m_pParent(nullptr), m_uiLevel(0)
{
}

template <typename KeyType, typename ValueType, typename Comparer>
void ezMapBase<KeyType, ValueType, Comparer>::Constructor()
{
  m_uiCount = 0;

  m_NilNode.m_uiLevel = 0;
  m_NilNode.m_pLink[0] = reinterpret_cast<Node*>(&m_NilNode);
  m_NilNode.m_pLink[1] = reinterpret_cast<Node*>(&m_NilNode);
  m_NilNode.m_pParent  = reinterpret_cast<Node*>(&m_NilNode);

  m_pFreeElementStack = nullptr;
  m_pRoot = reinterpret_cast<Node*>(&m_NilNode);
}

template <typename KeyType, typename ValueType, typename Comparer>
ezMapBase<KeyType, ValueType, Comparer>::ezMapBase(const Comparer& comparer, ezAllocatorBase* pAllocator) : m_Elements(pAllocator), m_Comparer(comparer)
{
  Constructor();
}

template <typename KeyType, typename ValueType, typename Comparer>
ezMapBase<KeyType, ValueType, Comparer>::ezMapBase(const ezMapBase<KeyType, ValueType, Comparer>& cc, ezAllocatorBase* pAllocator) : m_Elements(pAllocator)
{
  Constructor();

  operator=(cc);
}

template <typename KeyType, typename ValueType, typename Comparer>
ezMapBase<KeyType, ValueType, Comparer>::~ezMapBase()
{
  Clear();
}

template <typename KeyType, typename ValueType, typename Comparer>
void ezMapBase<KeyType, ValueType, Comparer>::operator=(const ezMapBase<KeyType, ValueType, Comparer>& rhs)
{
  Clear();

  for (ConstIterator it = rhs.GetIterator(); it.IsValid(); ++it)
    Insert(it.Key(), it.Value());
}

template <typename KeyType, typename ValueType, typename Comparer>
void ezMapBase<KeyType, ValueType, Comparer>::Clear()
{
  for (Iterator it = GetIterator(); it.IsValid(); ++it)
    ezMemoryUtils::Destruct<Node>(it.m_pElement, 1);

  m_pFreeElementStack = nullptr;
  m_Elements.Clear();

  m_uiCount = 0;

  m_NilNode.m_uiLevel = 0;
  m_NilNode.m_pLink[0] = reinterpret_cast<Node*>(&m_NilNode);
  m_NilNode.m_pLink[1] = reinterpret_cast<Node*>(&m_NilNode);
  m_NilNode.m_pParent  = reinterpret_cast<Node*>(&m_NilNode);

  m_pRoot = reinterpret_cast<Node*>(&m_NilNode);
}

template <typename KeyType, typename ValueType, typename Comparer>
EZ_FORCE_INLINE bool ezMapBase<KeyType, ValueType, Comparer>::IsEmpty() const
{
  return (m_uiCount == 0);
}

template <typename KeyType, typename ValueType, typename Comparer>
EZ_FORCE_INLINE ezUInt32 ezMapBase<KeyType, ValueType, Comparer>::GetCount() const
{
  return m_uiCount;
}


template <typename KeyType, typename ValueType, typename Comparer>
EZ_FORCE_INLINE typename ezMapBase<KeyType, ValueType, Comparer>::Iterator ezMapBase<KeyType, ValueType, Comparer>::GetIterator()
{
  return Iterator(GetLeftMost());
}

template <typename KeyType, typename ValueType, typename Comparer>
EZ_FORCE_INLINE typename ezMapBase<KeyType, ValueType, Comparer>::ConstIterator ezMapBase<KeyType, ValueType, Comparer>::GetIterator() const
{
  return ConstIterator(GetLeftMost());
}

template <typename KeyType, typename ValueType, typename Comparer>
EZ_FORCE_INLINE typename ezMapBase<KeyType, ValueType, Comparer>::Iterator ezMapBase<KeyType, ValueType, Comparer>::GetLastIterator()
{
  return Iterator(GetRightMost());
}

template <typename KeyType, typename ValueType, typename Comparer>
EZ_FORCE_INLINE typename ezMapBase<KeyType, ValueType, Comparer>::ConstIterator ezMapBase<KeyType, ValueType, Comparer>::GetLastIterator() const
{
  return ConstIterator(GetRightMost());
}

template <typename KeyType, typename ValueType, typename Comparer>
typename ezMapBase<KeyType, ValueType, Comparer>::Node* ezMapBase<KeyType, ValueType, Comparer>::GetLeftMost() const
{
  if (IsEmpty())
    return nullptr;

  Node* pNode = m_pRoot;

  while (pNode->m_pLink[0] != &m_NilNode)
    pNode = pNode->m_pLink[0];

  return pNode;
}

template <typename KeyType, typename ValueType, typename Comparer>
typename ezMapBase<KeyType, ValueType, Comparer>::Node* ezMapBase<KeyType, ValueType, Comparer>::GetRightMost() const
{
  if (IsEmpty())
    return nullptr;

  Node* pNode = m_pRoot;

  while (pNode->m_pLink[1] != &m_NilNode)
    pNode = pNode->m_pLink[1];

  return pNode;
}

template <typename KeyType, typename ValueType, typename Comparer>
typename ezMapBase<KeyType, ValueType, Comparer>::Node* ezMapBase<KeyType, ValueType, Comparer>::Internal_Find(const KeyType& key) const
{
  Node* pNode = m_pRoot;

  while (pNode != &m_NilNode)
  {
    const ezInt32 dir = (ezInt32) m_Comparer.Less(pNode->m_Key, key);
    const ezInt32 dir2= (ezInt32) m_Comparer.Less(key, pNode->m_Key);

    if (dir == dir2)
      break;

    pNode = pNode->m_pLink[dir];
  }

  if (pNode == &m_NilNode)
    return nullptr;

  return pNode;
}

template <typename KeyType, typename ValueType, typename Comparer>
EZ_FORCE_INLINE typename ezMapBase<KeyType, ValueType, Comparer>::Iterator ezMapBase<KeyType, ValueType, Comparer>::Find(const KeyType& key)
{
  return Iterator(Internal_Find(key));
}

template <typename KeyType, typename ValueType, typename Comparer>
EZ_FORCE_INLINE typename ezMapBase<KeyType, ValueType, Comparer>::ConstIterator ezMapBase<KeyType, ValueType, Comparer>::Find(const KeyType& key) const
{
  return ConstIterator(Internal_Find(key));
}

template <typename KeyType, typename ValueType, typename Comparer>
EZ_FORCE_INLINE bool ezMapBase<KeyType, ValueType, Comparer>::Contains(const KeyType& key) const
{
  return Internal_Find(key) != nullptr;
}

template <typename KeyType, typename ValueType, typename Comparer>
typename ezMapBase<KeyType, ValueType, Comparer>::Node* ezMapBase<KeyType, ValueType, Comparer>::Internal_LowerBound(const KeyType& key) const
{
  Node* pNode = m_pRoot;
  Node* pNodeSmaller = nullptr;

  while (pNode != &m_NilNode)
  {
    const ezInt32 dir = (ezInt32) m_Comparer.Less(pNode->m_Key, key);
    const ezInt32 dir2= (ezInt32) m_Comparer.Less(key, pNode->m_Key);

    if (dir == dir2)
      return pNode;

    if (dir == 0)
      pNodeSmaller = pNode;

    pNode = pNode->m_pLink[dir];
  }

  return pNodeSmaller;
}

template <typename KeyType, typename ValueType, typename Comparer>
EZ_FORCE_INLINE typename ezMapBase<KeyType, ValueType, Comparer>::Iterator ezMapBase<KeyType, ValueType, Comparer>::LowerBound(const KeyType& key)
{
  return Iterator(Internal_LowerBound(key));
}

template <typename KeyType, typename ValueType, typename Comparer>
EZ_FORCE_INLINE typename ezMapBase<KeyType, ValueType, Comparer>::ConstIterator ezMapBase<KeyType, ValueType, Comparer>::LowerBound(const KeyType& key) const
{
  return ConstIterator(Internal_LowerBound(key));
}

template <typename KeyType, typename ValueType, typename Comparer>
typename ezMapBase<KeyType, ValueType, Comparer>::Node* ezMapBase<KeyType, ValueType, Comparer>::Internal_UpperBound(const KeyType& key) const
{
  Node* pNode = m_pRoot;
  Node* pNodeSmaller = nullptr;

  while (pNode != &m_NilNode)
  {
    const ezInt32 dir = (ezInt32) m_Comparer.Less(pNode->m_Key, key);
    const ezInt32 dir2= (ezInt32) m_Comparer.Less(key, pNode->m_Key);

    if (dir == dir2)
    {
      ConstIterator it(pNode);
      ++it;
      return it.m_pElement;
    }

    if (dir == 0)
      pNodeSmaller = pNode;

    pNode = pNode->m_pLink[dir];
  }

  return pNodeSmaller;
}

template <typename KeyType, typename ValueType, typename Comparer>
EZ_FORCE_INLINE typename ezMapBase<KeyType, ValueType, Comparer>::Iterator ezMapBase<KeyType, ValueType, Comparer>::UpperBound(const KeyType& key)
{
  return Iterator(Internal_UpperBound(key));
}

template <typename KeyType, typename ValueType, typename Comparer>
EZ_FORCE_INLINE typename ezMapBase<KeyType, ValueType, Comparer>::ConstIterator ezMapBase<KeyType, ValueType, Comparer>::UpperBound(const KeyType& key) const
{
  return ConstIterator(Internal_UpperBound(key));
}

template <typename KeyType, typename ValueType, typename Comparer>
ValueType& ezMapBase<KeyType, ValueType, Comparer>::operator[](const KeyType& key)
{
  return FindOrAdd(key).Value();
}

template <typename KeyType, typename ValueType, typename Comparer>
typename ezMapBase<KeyType, ValueType, Comparer>::Iterator ezMapBase<KeyType, ValueType, Comparer>::FindOrAdd(const KeyType& key, bool* bExisted)
{
  Node* pNilNode = reinterpret_cast<Node*>(&m_NilNode);
  Node* pInsertedNode = nullptr;

  {
    Node* root = m_pRoot;

    if (m_pRoot != pNilNode)
    {
      Node* it = m_pRoot;
      Node* up[STACK_SIZE];

      ezInt32 top = 0;
      ezUInt32 dir = 0;

      while (true)
      {
        if (m_Comparer.Equal(it->m_Key, key))
        {
          if (bExisted)
            *bExisted = true;

          return Iterator(it);
        }

        dir = m_Comparer.Less(it->m_Key, key) ? 1 : 0;

        EZ_ASSERT_DEBUG(top < STACK_SIZE, "ezMapBase's internal stack is not large enough to be able to sort %i elements.", GetCount());
        up[top++] = it;

        if (it->m_pLink[dir] == pNilNode)
          break;

        it = it->m_pLink[dir];
      }

      pInsertedNode = AcquireNode(key, ValueType(), 1, it);
      it->m_pLink[dir] = pInsertedNode;

      while (--top >= 0)
      {
        if (top != 0)
          dir = (up[top - 1]->m_pLink[1] == up[top]) ? 1 : 0;

        up[top] = SkewNode(up[top]);
        up[top] = SplitNode(up[top]);

        if (top != 0)
        {
          up[top - 1]->m_pLink[dir] = up[top];
          up[top - 1]->m_pLink[dir]->m_pParent = up[top - 1];
        }
        else
          root = up[top];
      }
    }
    else
    {
      pInsertedNode = AcquireNode(key, ValueType(), 1, pNilNode);
      root = pInsertedNode;
    }

    m_pRoot = root;
    m_pRoot->m_pParent  = pNilNode;
    m_NilNode.m_pParent = pNilNode;
  }

  EZ_ASSERT_DEBUG(pInsertedNode != nullptr, "Implementation Error.");

  if (bExisted)
    *bExisted = false;

  return Iterator(pInsertedNode);
}

template <typename KeyType, typename ValueType, typename Comparer>
typename ezMapBase<KeyType, ValueType, Comparer>::Iterator ezMapBase<KeyType, ValueType, Comparer>::Insert(const KeyType& key, const ValueType& value)
{
  auto it = FindOrAdd(key);
  it.Value() = value;

  return it;
}

template <typename KeyType, typename ValueType, typename Comparer>
bool ezMapBase<KeyType, ValueType, Comparer>::Remove(const KeyType& key)
{
  bool bRemoved = true;
  m_pRoot = Remove(m_pRoot, key, bRemoved);
  m_pRoot->m_pParent  = reinterpret_cast<Node*>(&m_NilNode);
  m_NilNode.m_pParent = reinterpret_cast<Node*>(&m_NilNode);

  return bRemoved;
}

template <typename KeyType, typename ValueType, typename Comparer>
typename ezMapBase<KeyType, ValueType, Comparer>::Node* ezMapBase<KeyType, ValueType, Comparer>::AcquireNode(const KeyType& key, const ValueType& value, ezInt32 uiLevel, Node* pParent)
{
  Node* pNode;

  if (m_pFreeElementStack == nullptr)
  {
    m_Elements.PushBack();
    pNode = &m_Elements.PeekBack();
  }
  else
  {
    pNode = m_pFreeElementStack;
    m_pFreeElementStack = m_pFreeElementStack->m_pParent;
  }

  ezMemoryUtils::Construct(pNode, 1);

  pNode->m_pParent = pParent;
  pNode->m_Key = key;
  pNode->m_Value = value;
  pNode->m_uiLevel = uiLevel;
  pNode->m_pLink[0] = reinterpret_cast<Node*>(&m_NilNode);
  pNode->m_pLink[1] = reinterpret_cast<Node*>(&m_NilNode);

  ++m_uiCount;

  return pNode;
}

template <typename KeyType, typename ValueType, typename Comparer>
void ezMapBase<KeyType, ValueType, Comparer>::ReleaseNode(Node* pNode)
{
  EZ_ASSERT_DEV(pNode != nullptr, "pNode is invalid.");
  EZ_ASSERT_DEV(pNode != &m_NilNode, "pNode is invalid.");

  ezMemoryUtils::Destruct<Node>(pNode, 1);

  // try to reduce the element array, if possible
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
    pNode->m_pParent = m_pFreeElementStack;
    m_pFreeElementStack = pNode;
  }

  --m_uiCount;
}

template <typename KeyType, typename ValueType, typename Comparer>
EZ_FORCE_INLINE typename ezMapBase<KeyType, ValueType, Comparer>::Node* ezMapBase<KeyType, ValueType, Comparer>::SkewNode(Node* root)
{
  if ((root->m_pLink[0]->m_uiLevel == root->m_uiLevel) && (root->m_uiLevel != 0))
  {
    Node* save = root->m_pLink[0];
    root->m_pLink[0] = save->m_pLink[1];
    root->m_pLink[0]->m_pParent = root;
    save->m_pLink[1] = root;
    save->m_pLink[1]->m_pParent = save;
    root = save;
  }

  return root;
}

template <typename KeyType, typename ValueType, typename Comparer>
EZ_FORCE_INLINE typename ezMapBase<KeyType, ValueType, Comparer>::Node* ezMapBase<KeyType, ValueType, Comparer>::SplitNode(Node* root)
{
  if ((root->m_pLink[1]->m_pLink[1]->m_uiLevel == root->m_uiLevel) && (root->m_uiLevel != 0))
  {
    Node* save = root->m_pLink[1];
    root->m_pLink[1] = save->m_pLink[0];
    root->m_pLink[1]->m_pParent = root;
    save->m_pLink[0] = root;
    save->m_pLink[0]->m_pParent = save;
    root = save;
    ++root->m_uiLevel;
  }

  return root;
}

template <typename KeyType, typename ValueType, typename Comparer>
typename ezMapBase<KeyType, ValueType, Comparer>::Node* ezMapBase<KeyType, ValueType, Comparer>::Remove(Node* root, const KeyType& key, bool& bRemoved)
{
  bRemoved = false;

  Node* ToErase    = reinterpret_cast<Node*>(&m_NilNode);
  Node* ToOverride = reinterpret_cast<Node*>(&m_NilNode);

  if (root != &m_NilNode)
  {
    Node* it = root;
    Node* up[STACK_SIZE];
    ezInt32 top = 0;
    ezInt32 dir = 0;

    while (true)
    {
      EZ_ASSERT_DEBUG(top >= 0 && top < STACK_SIZE, "Implementation error");
      up[top++] = it;

      if (it == &m_NilNode)
        return root;

      if (m_Comparer.Equal(it->m_Key, key))
        break;

      dir = m_Comparer.Less(it->m_Key, key) ? 1 : 0;

      it = it->m_pLink[dir];
    }

    ToOverride = it;

    if ((it->m_pLink[0] == &m_NilNode) || (it->m_pLink[1] == &m_NilNode))
    {
      ezInt32 dir2 = it->m_pLink[0] == &m_NilNode;

      if (--top != 0)
      {
        EZ_ASSERT_DEBUG(top >= 1 && top < STACK_SIZE, "Implementation error");
        up[top - 1]->m_pLink[dir] = it->m_pLink[dir2];
        up[top - 1]->m_pLink[dir]->m_pParent = up[top - 1];
      }
      else
        root = it->m_pLink[1];
    }
    else
    {
      Node* heir = it->m_pLink[1];
      Node* prev = it;

      while (heir->m_pLink[0] != &m_NilNode)
      {
        EZ_ASSERT_DEBUG(top >= 0 && top < STACK_SIZE, "Implementation error");
        up[top++] = prev = heir;

        heir = heir->m_pLink[0];
      }

      ToErase = heir;
      ToOverride = it;

      prev->m_pLink[prev == it] = heir->m_pLink[1];
      prev->m_pLink[prev == it]->m_pParent = prev;
    }

    while (--top >= 0)
    {
      if (top != 0)
      {
        EZ_ASSERT_DEBUG(top >= 1 && top < STACK_SIZE, "Implementation error");
        dir = up[top - 1]->m_pLink[1] == up[top];
      }

      EZ_ASSERT_DEBUG(top >= 0 && top < STACK_SIZE, "Implementation error");

      if ((up[top]->m_pLink[0]->m_uiLevel < up[top]->m_uiLevel - 1) || (up[top]->m_pLink[1]->m_uiLevel < up[top]->m_uiLevel - 1))
      {
        if (up[top]->m_pLink[1]->m_uiLevel > --up[top]->m_uiLevel)
          up[top]->m_pLink[1]->m_uiLevel = up[top]->m_uiLevel;

        up[top] = SkewNode(up[top]);
        up[top]->m_pLink[1] = SkewNode(up[top]->m_pLink[1]);
        up[top]->m_pLink[1]->m_pParent = up[top];

        up[top]->m_pLink[1]->m_pLink[1] = SkewNode(up[top]->m_pLink[1]->m_pLink[1]);
        up[top] = SplitNode(up[top]);
        up[top]->m_pLink[1] = SplitNode(up[top]->m_pLink[1]);
        up[top]->m_pLink[1]->m_pParent = up[top];
      }

      if (top != 0)
      {
        EZ_ASSERT_DEBUG(top >= 1 && top < STACK_SIZE, "Implementation error");

        up[top - 1]->m_pLink[dir] = up[top];
        up[top - 1]->m_pLink[dir]->m_pParent = up[top - 1];
      }
      else
      {
        EZ_ASSERT_DEBUG(top >= 0 && top < STACK_SIZE, "Implementation error");
        root = up[top];
      }
    }
  }

  root->m_pParent = reinterpret_cast<Node*>(&m_NilNode);


  // if necessary, swap nodes
  if (ToErase != &m_NilNode)
  {
    Node* parent = ToOverride->m_pParent;

    if (parent != &m_NilNode)
    {
      if (parent->m_pLink[0] == ToOverride)
      {
        parent->m_pLink[0] = ToErase;
        parent->m_pLink[0]->m_pParent = parent;
      }
      if (parent->m_pLink[1] == ToOverride)
      {
        parent->m_pLink[1] = ToErase;
        parent->m_pLink[1]->m_pParent = parent;
      }
    }
    else
      root = ToErase;

    ToErase->m_uiLevel = ToOverride->m_uiLevel;
    ToErase->m_pLink[0] = ToOverride->m_pLink[0];
    ToErase->m_pLink[0]->m_pParent = ToErase;
    ToErase->m_pLink[1] = ToOverride->m_pLink[1];
    ToErase->m_pLink[1]->m_pParent = ToErase;
  }

  // remove the erased node
  if (ToOverride != &m_NilNode)
  {
    bRemoved = true;
    ReleaseNode(ToOverride);
  }

  return root;
}

template <typename KeyType, typename ValueType, typename Comparer>
typename ezMapBase<KeyType, ValueType, Comparer>::Iterator ezMapBase<KeyType, ValueType, Comparer>::Remove(const Iterator& pos)
{
  EZ_ASSERT_DEV(pos.m_pElement != nullptr, "The Iterator(pos) is invalid.");

  Iterator temp(pos);
  ++temp;
  Remove(pos.Key());
  return temp;
}

template <typename KeyType, typename ValueType, typename Comparer>
bool ezMapBase<KeyType, ValueType, Comparer>::operator==(const ezMapBase<KeyType, ValueType, Comparer>& rhs) const
{
  if (GetCount() != rhs.GetCount())
    return false;

  auto itLhs = GetIterator();
  auto itRhs = rhs.GetIterator();

  while (itLhs.IsValid())
  {
    if (!m_Comparer.Equal(itLhs.Key(), itRhs.Key()))
      return false;

    if (itLhs.Value() != itRhs.Value())
      return false;

    ++itLhs;
    ++itRhs;
  }

  return true;
}

template <typename KeyType, typename ValueType, typename Comparer>
bool ezMapBase<KeyType, ValueType, Comparer>::operator!=(const ezMapBase<KeyType, ValueType, Comparer>& rhs) const
{
  return !operator==(rhs);
}

#undef STACK_SIZE


template <typename KeyType, typename ValueType, typename Comparer, typename AllocatorWrapper>
ezMap<KeyType, ValueType, Comparer, AllocatorWrapper>::ezMap() : ezMapBase<KeyType, ValueType, Comparer>(Comparer(), AllocatorWrapper::GetAllocator())
{
}

template <typename KeyType, typename ValueType, typename Comparer, typename AllocatorWrapper>
ezMap<KeyType, ValueType, Comparer, AllocatorWrapper>::ezMap(const Comparer& comparer, ezAllocatorBase* pAllocator) : ezMapBase<KeyType, ValueType, Comparer>(comparer, pAllocator)
{
}

template <typename KeyType, typename ValueType, typename Comparer, typename AllocatorWrapper>
ezMap<KeyType, ValueType, Comparer, AllocatorWrapper>::ezMap(const ezMap<KeyType, ValueType, Comparer, AllocatorWrapper>& other) : ezMapBase<KeyType, ValueType, Comparer>(other, AllocatorWrapper::GetAllocator())
{
}

template <typename KeyType, typename ValueType, typename Comparer, typename AllocatorWrapper>
ezMap<KeyType, ValueType, Comparer, AllocatorWrapper>::ezMap(const ezMapBase<KeyType, ValueType, Comparer>& other) : ezMapBase<KeyType, ValueType, Comparer>(other, AllocatorWrapper::GetAllocator())
{
}

template <typename KeyType, typename ValueType, typename Comparer, typename AllocatorWrapper>
void ezMap<KeyType, ValueType, Comparer, AllocatorWrapper>::operator=(const ezMap<KeyType, ValueType, Comparer, AllocatorWrapper>& rhs)
{
  ezMapBase<KeyType, ValueType, Comparer>::operator=(rhs);
}

template <typename KeyType, typename ValueType, typename Comparer, typename AllocatorWrapper>
void ezMap<KeyType, ValueType, Comparer, AllocatorWrapper>::operator=(const ezMapBase<KeyType, ValueType, Comparer>& rhs)
{
  ezMapBase<KeyType, ValueType, Comparer>::operator=(rhs);
}

