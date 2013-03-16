#pragma once

#include <Foundation/Math/Math.h>

// ***** Const Iterator *****

template <typename K, typename C>
void ezSetBase<K, C>::Iterator::Next()
{
  const ezInt32 dir0 = 0;
  const ezInt32 dir1 = 1;

  if (m_pElement == NULL)
  {
    EZ_ASSERT(m_pElement != NULL, "The Iterator is invalid (end).");
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
      (m_pElement->m_pParent == NULL))
    {
      m_pElement = NULL;
      return;
    }

    m_pElement = m_pElement->m_pParent;
    return;
  }

  m_pElement = NULL;
  return;
}

template <typename K, typename C>
void ezSetBase<K, C>::Iterator::Prev()
{
  const ezInt32 dir0 = 1;
  const ezInt32 dir1 = 0;

  if (m_pElement == NULL)
  {
    EZ_ASSERT(m_pElement != NULL, "The Iterator is invalid (end).");
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
      (m_pElement->m_pParent == NULL))
    {
      m_pElement = NULL;
      return;
    }

    m_pElement = m_pElement->m_pParent;
    return;
  }

  m_pElement = NULL;
  return;
}

// ***** ezSetBase *****

template <typename K, typename C>
EZ_FORCE_INLINE ezSetBase<K, C>::NilNode::NilNode() : m_uiLevel(0), m_pParent(NULL)
{
}

template <typename K, typename C>
void ezSetBase<K, C>::Constructor()
{
  m_uiCount = 0;

  m_NilNode.m_uiLevel = 0;
  m_NilNode.m_pLink[0] = reinterpret_cast<Node*>(&m_NilNode);
  m_NilNode.m_pLink[1] = reinterpret_cast<Node*>(&m_NilNode);
  m_NilNode.m_pParent  = reinterpret_cast<Node*>(&m_NilNode);

  m_pFreeElementStack = NULL;
  m_pRoot = reinterpret_cast<Node*>(&m_NilNode);
}

template <typename K, typename C>
ezSetBase<K, C>::ezSetBase(ezIAllocator* pAllocator) : m_Elements(pAllocator)
{
  Constructor();
}

template <typename K, typename C>
ezSetBase<K, C>::ezSetBase (const ezSetBase<K, C>& cc, ezIAllocator* pAllocator) : m_Elements(pAllocator)
{
  Constructor();

  operator= (cc);
}

template <typename K, typename C>
ezSetBase<K, C>::~ezSetBase ()
{
  Clear();
}

template <typename K, typename C>
void ezSetBase<K, C>::operator= (const ezSetBase<K, C>& rhs)
{
  Clear();

  for (Iterator it = rhs.GetIterator(); it.IsValid(); ++it)
    Insert (it.Key ());
}

template <typename K, typename C>
void ezSetBase<K, C>::Clear()
{
  for (Iterator it = GetIterator(); it.IsValid(); ++it)
    ezMemoryUtils::Destruct<Node>(it.m_pElement, 1);

  m_pFreeElementStack = NULL;
  m_Elements.Clear();

  m_uiCount = 0;

  m_NilNode.m_uiLevel = 0;
  m_NilNode.m_pLink[0] = reinterpret_cast<Node*>(&m_NilNode);
  m_NilNode.m_pLink[1] = reinterpret_cast<Node*>(&m_NilNode);
  m_NilNode.m_pParent  = reinterpret_cast<Node*>(&m_NilNode);

  m_pRoot = reinterpret_cast<Node*>(&m_NilNode);
}

template <typename K, typename C>
EZ_FORCE_INLINE bool ezSetBase<K, C>::IsEmpty() const
{
  return (m_uiCount == 0);
}

template <typename K, typename C>
EZ_FORCE_INLINE ezUInt32 ezSetBase<K, C>::GetCount() const
{
  return m_uiCount;
}


template <typename K, typename C>
EZ_FORCE_INLINE typename ezSetBase<K, C>::Iterator ezSetBase<K, C>::GetIterator() const
{
  return Iterator(GetLeftMost ());
}

template <typename K, typename C>
EZ_FORCE_INLINE typename ezSetBase<K, C>::Iterator ezSetBase<K, C>::GetLastIterator() const
{
  return Iterator(GetRightMost ());
}

template <typename K, typename C>
typename ezSetBase<K, C>::Node* ezSetBase<K, C>::GetLeftMost() const
{
  if (IsEmpty())
    return NULL;

  Node* pNode = m_pRoot;

  while (pNode->m_pLink[0] != &m_NilNode)
    pNode = pNode->m_pLink[0];

  return pNode;
}

template <typename K, typename C>
typename ezSetBase<K, C>::Node* ezSetBase<K, C>::GetRightMost() const
{
  if (IsEmpty())
    return NULL;

  Node* pNode = m_pRoot;

  while (pNode->m_pLink[1] != &m_NilNode)
    pNode = pNode->m_pLink[1];

  return pNode;
}

template <typename K, typename C>
typename ezSetBase<K, C>::Node* ezSetBase<K, C>::Internal_Find (const K& key) const
{
  Node* pNode = m_pRoot;

  while (pNode != &m_NilNode)// && (pNode->m_Key != key))
  {
    const ezInt32 dir = (ezInt32) C::Less(pNode->m_Key, key);
    const ezInt32 dir2= (ezInt32) C::Less(key, pNode->m_Key);

    if (dir == dir2)
      break;

    pNode = pNode->m_pLink[dir];
  }

  if (pNode == &m_NilNode)
    return NULL;

  return pNode;
}

template <typename K, typename C>
EZ_FORCE_INLINE typename ezSetBase<K, C>::Iterator ezSetBase<K, C>::Find (const K& key)
{
  return Iterator(Internal_Find(key));
}

template <typename K, typename C>
typename ezSetBase<K, C>::Node* ezSetBase<K, C>::Internal_LowerBound (const K& key) const
{
  Node* pNode = m_pRoot;
  Node* pNodeSmaller = NULL;

  while (pNode != &m_NilNode)
  {
    const ezInt32 dir = (ezInt32) C::Less(pNode->m_Key, key);
    const ezInt32 dir2= (ezInt32) C::Less(key, pNode->m_Key);

    if (dir == dir2)
      return pNode;

    if (dir == 0)
      pNodeSmaller = pNode;

    pNode = pNode->m_pLink[dir];
  }

  return pNodeSmaller;
}

template <typename K, typename C>
EZ_FORCE_INLINE typename ezSetBase<K, C>::Iterator ezSetBase<K, C>::LowerBound (const K& key)
{
  return Iterator(Internal_LowerBound(key));
}

template <typename K, typename C>
typename ezSetBase<K, C>::Node* ezSetBase<K, C>::Internal_UpperBound (const K& key) const
{
  Node* pNode = m_pRoot;
  Node* pNodeSmaller = NULL;

  while (pNode != &m_NilNode)
  {
    const ezInt32 dir = (ezInt32) C::Less(pNode->m_Key, key);
    const ezInt32 dir2= (ezInt32) C::Less(key, pNode->m_Key);

    if (dir == dir2)
    {
      Iterator it (pNode);
      ++it;
      return it.m_pElement;
    }

    if (dir == 0)
      pNodeSmaller = pNode;

    pNode = pNode->m_pLink[dir];
  }

  return pNodeSmaller;
}

template <typename K, typename C>
EZ_FORCE_INLINE typename ezSetBase<K, C>::Iterator ezSetBase<K, C>::UpperBound (const K& key)
{
  return Iterator(Internal_UpperBound(key));
}

template <typename K, typename C>
typename ezSetBase<K, C>::Iterator ezSetBase<K, C>::Insert(const K& key)
{
  Node* pInsertedNode = NULL;

  m_pRoot = Insert(m_pRoot, key, pInsertedNode);
  m_pRoot->m_pParent  = reinterpret_cast<Node*>(&m_NilNode);
  m_NilNode.m_pParent = reinterpret_cast<Node*>(&m_NilNode);

  return Iterator(pInsertedNode);
}

template <typename K, typename C>
void ezSetBase<K, C>::Erase (const K& key)
{
  m_pRoot = Erase(m_pRoot, key);
  m_pRoot->m_pParent  = reinterpret_cast<Node*>(&m_NilNode);
  m_NilNode.m_pParent = reinterpret_cast<Node*>(&m_NilNode);
}

template <typename K, typename C>
typename ezSetBase<K, C>::Node* ezSetBase<K, C>::AcquireNode(const K& key, ezInt32 m_uiLevel, Node* pParent)
{
  Node* pNode;

  if (m_pFreeElementStack == NULL)
  {
    m_Elements.PushBack();
    pNode = &m_Elements.PeekBack();
  }
  else
  {
    pNode = m_pFreeElementStack;
    m_pFreeElementStack = m_pFreeElementStack->m_pParent;
  }

  ezMemoryUtils::Construct<Node>(pNode, 1);

  pNode->m_pParent = pParent;
  pNode->m_Key = key;
  pNode->m_uiLevel = m_uiLevel;
  pNode->m_pLink[0] = reinterpret_cast<Node*>(&m_NilNode);
  pNode->m_pLink[1] = reinterpret_cast<Node*>(&m_NilNode);

  ++m_uiCount;

  return pNode;
}

template <typename K, typename C>
void ezSetBase<K, C>::ReleaseNode(Node* pNode)
{
  EZ_ASSERT(pNode != NULL, "pNode is invalid.");

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

template <typename K, typename C>
typename ezSetBase<K, C>::Node* ezSetBase<K, C>::SkewNode(Node* root)
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

template <typename K, typename C>
typename ezSetBase<K, C>::Node* ezSetBase<K, C>::SplitNode(Node* root)
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

template <typename K, typename C>
typename ezSetBase<K, C>::Node* ezSetBase<K, C>::Insert(Node* root, const K& key, Node*& pInsertedNode)
{
  if (root == &m_NilNode)
  {
    pInsertedNode = AcquireNode(key, 1, reinterpret_cast<Node*>(&m_NilNode));
    root = pInsertedNode;
  }
  else 
  {
    Node* it = root;
    Node* up[32];

    ezInt32 top = 0;
    ezInt32 dir = 0;

    while (true) 
    {
      up[top++] = it;
      dir = C::Less(it->m_Key, key) ? 1 : 0;

      // element is identical => do not insert
      if ((ezInt32) C::Less(key, it->m_Key) == dir)
      {
        return root;
      }

      if (it->m_pLink[dir] == &m_NilNode)
        break;

      it = it->m_pLink[dir];
    }

    pInsertedNode = AcquireNode(key, 1, it);
    it->m_pLink[dir] = pInsertedNode;

    while ( --top >= 0 ) 
    {
      if (top != 0)
        dir = up[top - 1]->m_pLink[1] == up[top];

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

  return root;
}

template <typename K, typename C>
typename ezSetBase<K, C>::Node* ezSetBase<K, C>::Erase(Node* root, const K& key)
{
  Node* ToErase    = reinterpret_cast<Node*>(&m_NilNode);
  Node* ToOverride = reinterpret_cast<Node*>(&m_NilNode);

  if (root != &m_NilNode)
  {
    Node* it = root;
    Node* up[32];
    ezInt32 top = 0;
    ezInt32 dir = 0;

    while (true)
    {
      up[top++] = it;

      if (it == &m_NilNode)
        return root;

      ezInt32 newdir = (ezInt32) (C::Less(it->m_Key, key));

      if (newdir == (ezInt32) (C::Less(key, it->m_Key)))
        break;

      dir = newdir;

      it = it->m_pLink[dir];
    }

    ToOverride = it;

    if ((it->m_pLink[0] == &m_NilNode) || (it->m_pLink[1] == &m_NilNode)) 
    {
      ezInt32 dir2 = it->m_pLink[0] == &m_NilNode;

      if ( --top != 0 )
      {
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
        up[top++] = prev = heir;

        heir = heir->m_pLink[0];
      }

      ToErase = heir;
      ToOverride = it;

      prev->m_pLink[prev == it] = heir->m_pLink[1];
      prev->m_pLink[prev == it]->m_pParent = prev;
    }

    while ( --top >= 0 ) 
    {
      if (top != 0)
        dir = up[top - 1]->m_pLink[1] == up[top];

      if ((up[top]->m_pLink[0]->m_uiLevel < up[top]->m_uiLevel - 1) || (up[top]->m_pLink[1]->m_uiLevel < up[top]->m_uiLevel - 1))
      {
        if (up[top]->m_pLink[1]->m_uiLevel > --up[top]->m_uiLevel)
          up[top]->m_pLink[1]->m_uiLevel = up[top]->m_uiLevel;

        up[top] = SkewNode (up[top]);
        up[top]->m_pLink[1] = SkewNode (up[top]->m_pLink[1]);
        up[top]->m_pLink[1]->m_pParent = up[top];

        up[top]->m_pLink[1]->m_pLink[1] = SkewNode (up[top]->m_pLink[1]->m_pLink[1]);
        up[top] = SplitNode (up[top]);
        up[top]->m_pLink[1] = SplitNode (up[top]->m_pLink[1]);
        up[top]->m_pLink[1]->m_pParent = up[top];
      }

      if (top != 0)
      {
        up[top - 1]->m_pLink[dir] = up[top];
        up[top - 1]->m_pLink[dir]->m_pParent = up[top - 1];
      }
      else
        root = up[top];
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
  ReleaseNode(ToOverride);

  return root;
}

template <typename K, typename C>
typename ezSetBase<K, C>::Iterator ezSetBase<K, C>::Erase(const Iterator& pos)
{
  EZ_ASSERT(pos.m_pElement != NULL, "The Iterator (pos) is invalid.");

  Iterator temp(pos);
  ++temp;
  Erase(pos.Key());
  return temp;
}


template <typename K, typename C, typename A>
ezSet<K, C, A>::ezSet() : ezSetBase<K, C>(A::GetAllocator())
{
}

template <typename K, typename C, typename A>
ezSet<K, C, A>::ezSet(ezIAllocator* pAllocator) : ezSetBase<K, C>(pAllocator)
{
}

template <typename K, typename C, typename A>
ezSet<K, C, A>::ezSet(const ezSet<K, C, A>& other) : ezSetBase<K, C>(other, A::GetAllocator())
{
}

template <typename K, typename C, typename A>
ezSet<K, C, A>:: ezSet(const ezSetBase<K, C>& other) : ezSetBase<K, C>(other, A::GetAllocator())
{
}

template <typename K, typename C, typename A>
void ezSet<K, C, A>::operator=(const ezSet<K, C, A>& rhs)
{
  ezSetBase<K, C>::operator=(rhs);
}

template <typename K, typename C, typename A>
void ezSet<K, C, A>::operator=(const ezSetBase<K, C>& rhs)
{
  ezSetBase<K, C>::operator=(rhs);
}
