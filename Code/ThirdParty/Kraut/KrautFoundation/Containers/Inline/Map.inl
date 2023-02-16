#ifndef AE_FOUNDATION_CONTAINERS_MAP_INL
#define AE_FOUNDATION_CONTAINERS_MAP_INL

#include "../../Math/Math.h"
#include "../../Basics/Checks.h"
#include "../../Memory/Memory.h"

namespace AE_NS_FOUNDATION
{
  // ***** base iterator *****

  template < class KEY, class VALUE, class COMPARE, bool NO_DEBUG_ALLOCATOR>
  void aeMap<KEY, VALUE, COMPARE, NO_DEBUG_ALLOCATOR>::iterator_base::forward (void)
  {
    const int dir0 = 0;
    const int dir1 = 1;

    if (m_pElement == nullptr)
    {
      AE_CHECK_DEV (m_pElement != nullptr, "aeMap::iterator_base::forward: The iterator is invalid (end).");
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

  template < class KEY, class VALUE, class COMPARE, bool NO_DEBUG_ALLOCATOR>
  void aeMap<KEY, VALUE, COMPARE, NO_DEBUG_ALLOCATOR>::iterator_base::backward (void)
  {
    const int dir0 = 1;
    const int dir1 = 0;

    if (m_pElement == nullptr)
    {
      AE_CHECK_DEV (m_pElement != nullptr, "aeMap::iterator_base::backward: The iterator is invalid (end).");
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

  // ***** aeMap *****

  template < class KEY, class VALUE, class COMPARE, bool NO_DEBUG_ALLOCATOR>
  aeMap<KEY, VALUE, COMPARE, NO_DEBUG_ALLOCATOR>::aeNode::aeNode (void) : m_uiLevel (0), m_pParent (nullptr)
  {
  }

  template < class KEY, class VALUE, class COMPARE, bool NO_DEBUG_ALLOCATOR>
  aeMap<KEY, VALUE, COMPARE, NO_DEBUG_ALLOCATOR>::aeMap (void)
  {
    m_uiSize = 0;

    m_NilNode.m_uiLevel = 0;
    m_NilNode.m_pLink[0] = &m_NilNode;
    m_NilNode.m_pLink[1] = &m_NilNode;
    m_NilNode.m_pParent = &m_NilNode;

    m_pRoot = &m_NilNode;
  }

  template < class KEY, class VALUE, class COMPARE, bool NO_DEBUG_ALLOCATOR>
  aeMap<KEY, VALUE, COMPARE, NO_DEBUG_ALLOCATOR>::aeMap (const aeMap<KEY, VALUE, COMPARE, NO_DEBUG_ALLOCATOR>& cc)
  {
    m_uiSize = 0;

    m_NilNode.m_uiLevel = 0;
    m_NilNode.m_pLink[0] = &m_NilNode;
    m_NilNode.m_pLink[1] = &m_NilNode;
    m_NilNode.m_pParent = &m_NilNode;

    m_pRoot = &m_NilNode;

    operator= (cc);
  }

  template < class KEY, class VALUE, class COMPARE, bool NO_DEBUG_ALLOCATOR>
  aeMap<KEY, VALUE, COMPARE, NO_DEBUG_ALLOCATOR>::~aeMap ()
  {
    clear ();
  }

  template < class KEY, class VALUE, class COMPARE, bool NO_DEBUG_ALLOCATOR>
  void aeMap<KEY, VALUE, COMPARE, NO_DEBUG_ALLOCATOR>::operator= (const aeMap<KEY, VALUE, COMPARE, NO_DEBUG_ALLOCATOR>& rhs)
  {
    clear ();

    const_iterator itend = rhs.end ();
    for (const_iterator it = rhs.begin (); it != itend; ++it)
      insert (it.key (), it.value ());
  }

  template < class KEY, class VALUE, class COMPARE, bool NO_DEBUG_ALLOCATOR>
  void aeMap<KEY, VALUE, COMPARE, NO_DEBUG_ALLOCATOR>::clear (void)
  {
    iterator it = begin ();
    const iterator itend = end ();

    for ( ; it != itend; ++it)
      aeMemoryManagement::Destruct<aeNode> (it.m_pElement);

    m_FreeElements.clear ();
    m_Elements.clear ();

    m_uiSize = 0;

    m_NilNode.m_uiLevel = 0;
    m_NilNode.m_pLink[0] = &m_NilNode;
    m_NilNode.m_pLink[1] = &m_NilNode;
    m_NilNode.m_pParent = &m_NilNode;

    m_pRoot = &m_NilNode;
  }

  template < class KEY, class VALUE, class COMPARE, bool NO_DEBUG_ALLOCATOR>
  bool aeMap<KEY, VALUE, COMPARE, NO_DEBUG_ALLOCATOR>::empty (void) const
  {
    return (m_uiSize == 0);
  }

  template < class KEY, class VALUE, class COMPARE, bool NO_DEBUG_ALLOCATOR>
  aeUInt32 aeMap<KEY, VALUE, COMPARE, NO_DEBUG_ALLOCATOR>::size (void) const
  {
    return (m_uiSize);
  }


  template < class KEY, class VALUE, class COMPARE, bool NO_DEBUG_ALLOCATOR>
  typename aeMap<KEY, VALUE, COMPARE, NO_DEBUG_ALLOCATOR>::iterator aeMap<KEY, VALUE, COMPARE, NO_DEBUG_ALLOCATOR>::begin (void)
  {
    return (iterator (GetLeftMost ()));
  }

  template < class KEY, class VALUE, class COMPARE, bool NO_DEBUG_ALLOCATOR>
  typename aeMap<KEY, VALUE, COMPARE, NO_DEBUG_ALLOCATOR>::iterator aeMap<KEY, VALUE, COMPARE, NO_DEBUG_ALLOCATOR>::end (void)
  {
    return (iterator (nullptr));
  }

  template < class KEY, class VALUE, class COMPARE, bool NO_DEBUG_ALLOCATOR>
  typename aeMap<KEY, VALUE, COMPARE, NO_DEBUG_ALLOCATOR>::const_iterator aeMap<KEY, VALUE, COMPARE, NO_DEBUG_ALLOCATOR>::begin (void) const
  {
    return (const_iterator (GetLeftMost ()));
  }

  template < class KEY, class VALUE, class COMPARE, bool NO_DEBUG_ALLOCATOR>
  typename aeMap<KEY, VALUE, COMPARE, NO_DEBUG_ALLOCATOR>::const_iterator aeMap<KEY, VALUE, COMPARE, NO_DEBUG_ALLOCATOR>::end (void) const
  {
    return (const_iterator (nullptr));
  }

  template < class KEY, class VALUE, class COMPARE, bool NO_DEBUG_ALLOCATOR>
  typename aeMap<KEY, VALUE, COMPARE, NO_DEBUG_ALLOCATOR>::reverse_iterator aeMap<KEY, VALUE, COMPARE, NO_DEBUG_ALLOCATOR>::rbegin (void)
  {
    return (reverse_iterator (GetRightMost ()));
  }

  template < class KEY, class VALUE, class COMPARE, bool NO_DEBUG_ALLOCATOR>
  typename aeMap<KEY, VALUE, COMPARE, NO_DEBUG_ALLOCATOR>::reverse_iterator aeMap<KEY, VALUE, COMPARE, NO_DEBUG_ALLOCATOR>::rend (void)
  {
    return (reverse_iterator (nullptr));
  }

  template < class KEY, class VALUE, class COMPARE, bool NO_DEBUG_ALLOCATOR>
  typename aeMap<KEY, VALUE, COMPARE, NO_DEBUG_ALLOCATOR>::const_reverse_iterator aeMap<KEY, VALUE, COMPARE, NO_DEBUG_ALLOCATOR>::rbegin (void) const
  {
    return (const_reverse_iterator (GetRightMost ()));
  }

  template < class KEY, class VALUE, class COMPARE, bool NO_DEBUG_ALLOCATOR>
  typename aeMap<KEY, VALUE, COMPARE, NO_DEBUG_ALLOCATOR>::const_reverse_iterator aeMap<KEY, VALUE, COMPARE, NO_DEBUG_ALLOCATOR>::rend (void) const
  {
    return (const_reverse_iterator (nullptr));
  }


  template < class KEY, class VALUE, class COMPARE, bool NO_DEBUG_ALLOCATOR>
  typename aeMap<KEY, VALUE, COMPARE, NO_DEBUG_ALLOCATOR>::aeNode* aeMap<KEY, VALUE, COMPARE, NO_DEBUG_ALLOCATOR>::GetLeftMost (void) const
  {
    if (empty ())
      return (nullptr);

    aeNode* pNode = m_pRoot;

    while (pNode->m_pLink[0] != &m_NilNode)
      pNode = pNode->m_pLink[0];

    return (pNode);
  }

  template < class KEY, class VALUE, class COMPARE, bool NO_DEBUG_ALLOCATOR>
  typename aeMap<KEY, VALUE, COMPARE, NO_DEBUG_ALLOCATOR>::aeNode* aeMap<KEY, VALUE, COMPARE, NO_DEBUG_ALLOCATOR>::GetRightMost (void) const
  {
    if (empty ())
      return (nullptr);

    aeNode* pNode = m_pRoot;

    while (pNode->m_pLink[1] != &m_NilNode)
      pNode = pNode->m_pLink[1];

    return (pNode);
  }

  template < class KEY, class VALUE, class COMPARE, bool NO_DEBUG_ALLOCATOR>
  typename aeMap<KEY, VALUE, COMPARE, NO_DEBUG_ALLOCATOR>::iterator aeMap<KEY, VALUE, COMPARE, NO_DEBUG_ALLOCATOR>::find (const KEY& key)
  {
    aeNode* pNode = m_pRoot;

    while (pNode != &m_NilNode)// && (pNode->m_Key != key))
    {
      const int dir = (int) COMPARE () (pNode->m_Key, key);
      const int dir2= (int) COMPARE () (key, pNode->m_Key);

      if (dir == dir2)
        break;

      pNode = pNode->m_pLink[dir];
    }

    if (pNode == &m_NilNode)
      return (end ());

    return (iterator (pNode));
  }

  template < class KEY, class VALUE, class COMPARE, bool NO_DEBUG_ALLOCATOR>
  typename aeMap<KEY, VALUE, COMPARE, NO_DEBUG_ALLOCATOR>::const_iterator aeMap<KEY, VALUE, COMPARE, NO_DEBUG_ALLOCATOR>::find (const KEY& key) const
  {
    aeNode* pNode = m_pRoot;

    while (pNode != &m_NilNode)// && (pNode->m_Key != key))
    {
      const int dir = (int) COMPARE () (pNode->m_Key, key);
      const int dir2= (int) COMPARE () (key, pNode->m_Key);

      if (dir == dir2)
        break;

      pNode = pNode->m_pLink[dir];
    }

    if (pNode == &m_NilNode)
      return (end ());

    return (const_iterator (pNode));
  }

  template < class KEY, class VALUE, class COMPARE, bool NO_DEBUG_ALLOCATOR>
  typename aeMap<KEY, VALUE, COMPARE, NO_DEBUG_ALLOCATOR>::iterator aeMap<KEY, VALUE, COMPARE, NO_DEBUG_ALLOCATOR>::lower_bound (const KEY& key)
  {
    aeNode* pNode = m_pRoot;
    aeNode* pNodeSmaller = nullptr;

    while (pNode != &m_NilNode)
    {
      const int dir = (int) COMPARE () (pNode->m_Key, key);
      const int dir2= (int) COMPARE () (key, pNode->m_Key);

      if (dir == dir2)
        return (iterator (pNode));

      if (dir == 0)
        pNodeSmaller = pNode;

      pNode = pNode->m_pLink[dir];
    }

    return (iterator (pNodeSmaller));
  }

  template < class KEY, class VALUE, class COMPARE, bool NO_DEBUG_ALLOCATOR>
  typename aeMap<KEY, VALUE, COMPARE, NO_DEBUG_ALLOCATOR>::iterator aeMap<KEY, VALUE, COMPARE, NO_DEBUG_ALLOCATOR>::upper_bound (const KEY& key)
  {
    aeNode* pNode = m_pRoot;
    aeNode* pNodeSmaller = nullptr;

    while (pNode != &m_NilNode)
    {
      const int dir = (int) COMPARE () (pNode->m_Key, key);
      const int dir2= (int) COMPARE () (key, pNode->m_Key);

      if (dir == dir2)
      {
        iterator it (pNode);
        ++it;
        return (it);
      }

      if (dir == 0)
        pNodeSmaller = pNode;

      pNode = pNode->m_pLink[dir];
    }

    return (iterator (pNodeSmaller));
  }


  template < class KEY, class VALUE, class COMPARE, bool NO_DEBUG_ALLOCATOR>
  typename aeMap<KEY, VALUE, COMPARE, NO_DEBUG_ALLOCATOR>::const_iterator aeMap<KEY, VALUE, COMPARE, NO_DEBUG_ALLOCATOR>::lower_bound (const KEY& key) const
  {
    aeNode* pNode = m_pRoot;
    aeNode* pNodeSmaller = nullptr;

    while (pNode != &m_NilNode)
    {
      const int dir = (int) COMPARE () (pNode->m_Key, key);
      const int dir2= (int) COMPARE () (key, pNode->m_Key);

      if (dir == dir2)
        return (const_iterator (pNode));

      if (dir == 0)
        pNodeSmaller = pNode;

      pNode = pNode->m_pLink[dir];
    }

    return (const_iterator (pNodeSmaller));
  }

  template < class KEY, class VALUE, class COMPARE, bool NO_DEBUG_ALLOCATOR>
  typename aeMap<KEY, VALUE, COMPARE, NO_DEBUG_ALLOCATOR>::const_iterator aeMap<KEY, VALUE, COMPARE, NO_DEBUG_ALLOCATOR>::upper_bound (const KEY& key) const
  {
    aeNode* pNode = m_pRoot;
    aeNode* pNodeSmaller = nullptr;

    while (pNode != &m_NilNode)
    {
      const int dir = (int) COMPARE () (pNode->m_Key, key);
      const int dir2= (int) COMPARE () (key, pNode->m_Key);

      if (dir == dir2)
      {
        const_iterator it (pNode);
        ++it;
        return (it);
      }

      if (dir == 0)
        pNodeSmaller = pNode;

      pNode = pNode->m_pLink[dir];
    }

    return (const_iterator (pNodeSmaller));
  }

  template < class KEY, class VALUE, class COMPARE, bool NO_DEBUG_ALLOCATOR>
  VALUE& aeMap<KEY, VALUE, COMPARE, NO_DEBUG_ALLOCATOR>::operator[] (const KEY& key)
  {
    iterator it = find (key);
    if (it != end ())
      return (it.value ());

    return (insert (key, VALUE ()).value ());
  }

  template < class KEY, class VALUE, class COMPARE, bool NO_DEBUG_ALLOCATOR>
  typename aeMap<KEY, VALUE, COMPARE, NO_DEBUG_ALLOCATOR>::iterator aeMap<KEY, VALUE, COMPARE, NO_DEBUG_ALLOCATOR>::insert (const KEY& key, const VALUE& value)
  {
    aeNode* pInsertedNode = nullptr;

    m_pRoot = insert (m_pRoot, key, value, pInsertedNode);
    m_pRoot->m_pParent = &m_NilNode;
    m_NilNode.m_pParent = &m_NilNode;

    return (iterator (pInsertedNode));
  }

  template < class KEY, class VALUE, class COMPARE, bool NO_DEBUG_ALLOCATOR>
  void aeMap<KEY, VALUE, COMPARE, NO_DEBUG_ALLOCATOR>::erase (const KEY& key)
  {
    m_pRoot = erase (m_pRoot, key);
    m_pRoot->m_pParent = &m_NilNode;
    m_NilNode.m_pParent = &m_NilNode;
  }

  template < class KEY, class VALUE, class COMPARE, bool NO_DEBUG_ALLOCATOR>
  typename aeMap<KEY, VALUE, COMPARE, NO_DEBUG_ALLOCATOR>::aeNode* aeMap<KEY, VALUE, COMPARE, NO_DEBUG_ALLOCATOR>::AccquireNode (const KEY& key, const VALUE& value, int m_uiLevel, aeNode* pParent)
  {
    aeNode* pNode;

    if (m_FreeElements.empty ())
    {
      m_Elements.push_back ();
      pNode = &m_Elements.back ();
    }
    else
    {
      pNode = m_FreeElements.top ();
      m_FreeElements.pop ();
    }

    aeMemoryManagement::Construct<aeNode> (pNode);

    pNode->m_pParent = pParent;
    pNode->m_Key = key;
    pNode->m_Value = value;
    pNode->m_uiLevel = m_uiLevel;
    pNode->m_pLink[0] = &m_NilNode;
    pNode->m_pLink[1] = &m_NilNode;

    ++m_uiSize;

    return (pNode);
  }

  template < class KEY, class VALUE, class COMPARE, bool NO_DEBUG_ALLOCATOR>
  void aeMap<KEY, VALUE, COMPARE, NO_DEBUG_ALLOCATOR>::ReleaseNode (aeNode* pNode)
  {
    AE_CHECK_ALWAYS (pNode != nullptr, "aeSet::ReleaseNode: pNode is invalid.");

    aeMemoryManagement::Destruct<aeNode> (pNode);

    m_FreeElements.push (pNode);

    --m_uiSize;
  }

  template < class KEY, class VALUE, class COMPARE, bool NO_DEBUG_ALLOCATOR>
  typename aeMap<KEY, VALUE, COMPARE, NO_DEBUG_ALLOCATOR>::aeNode* aeMap<KEY, VALUE, COMPARE, NO_DEBUG_ALLOCATOR>::SkewNode (aeNode* root)
  {
    if ((root->m_pLink[0]->m_uiLevel == root->m_uiLevel) && (root->m_uiLevel != 0)) 
    {
      aeNode* save = root->m_pLink[0];
      root->m_pLink[0] = save->m_pLink[1];
      root->m_pLink[0]->m_pParent = root;
      save->m_pLink[1] = root;
      save->m_pLink[1]->m_pParent = save;
      root = save;
    }

    return root;
  }

  template < class KEY, class VALUE, class COMPARE, bool NO_DEBUG_ALLOCATOR>
  typename aeMap<KEY, VALUE, COMPARE, NO_DEBUG_ALLOCATOR>::aeNode* aeMap<KEY, VALUE, COMPARE, NO_DEBUG_ALLOCATOR>::SplitNode (aeNode* root)
  {
    if ((root->m_pLink[1]->m_pLink[1]->m_uiLevel == root->m_uiLevel) && (root->m_uiLevel != 0)) 
    {
      aeNode* save = root->m_pLink[1];
      root->m_pLink[1] = save->m_pLink[0];
      root->m_pLink[1]->m_pParent = root;
      save->m_pLink[0] = root;
      save->m_pLink[0]->m_pParent = save;
      root = save;
      ++root->m_uiLevel;
    }

    return root;
  }

  template < class KEY, class VALUE, class COMPARE, bool NO_DEBUG_ALLOCATOR>
  typename aeMap<KEY, VALUE, COMPARE, NO_DEBUG_ALLOCATOR>::aeNode* aeMap<KEY, VALUE, COMPARE, NO_DEBUG_ALLOCATOR>::insert (aeNode* root, const KEY& key, const VALUE& value, aeNode*& pInsertedNode)
  {
    if (root == &m_NilNode)
    {
      pInsertedNode = AccquireNode (key, value, 1, &m_NilNode);
      root = pInsertedNode;
    }
    else 
    {
      aeNode* it = root;
      aeNode* up[32];

      int top = 0;
      int dir = 0;

      while (true) 
      {
        up[top++] = it;
        dir = (int) COMPARE ()(it->m_Key, key);

        // element is identical => do not insert
        if ((int) COMPARE ()(key, it->m_Key) == dir)
        {
          it->m_Value = value;
          return (root);
        }

        if (it->m_pLink[dir] == &m_NilNode)
          break;

        it = it->m_pLink[dir];
      }

      pInsertedNode = AccquireNode (key, value, 1, it);
      it->m_pLink[dir] = pInsertedNode;

      while ( --top >= 0 ) 
      {
        if (top != 0)
          dir = up[top - 1]->m_pLink[1] == up[top];

        up[top] = SkewNode (up[top]);
        up[top] = SplitNode (up[top]);

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

  template < class KEY, class VALUE, class COMPARE, bool NO_DEBUG_ALLOCATOR>
  typename aeMap<KEY, VALUE, COMPARE, NO_DEBUG_ALLOCATOR>::aeNode* aeMap<KEY, VALUE, COMPARE, NO_DEBUG_ALLOCATOR>::erase (aeNode* root, const KEY& key)
  {
    aeNode* ToErase = &m_NilNode;
    aeNode* ToOverride = &m_NilNode;

    if (root != &m_NilNode)
    {
      aeNode* it = root;
      aeNode* up[32];
      int top = 0;
      int dir = 0;

      while (true)
      {
        up[top++] = it;

        if (it == &m_NilNode)
          return root;

		int newdir = (int) (COMPARE () (it->m_Key, key));

        if (newdir == (int) (COMPARE () (key, it->m_Key)))
          break;

        dir = newdir;

        it = it->m_pLink[dir];
      }

      ToOverride = it;

      if ((it->m_pLink[0] == &m_NilNode) || (it->m_pLink[1] == &m_NilNode)) 
      {
        int dir2 = it->m_pLink[0] == &m_NilNode;

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
        aeNode* heir = it->m_pLink[1];
        aeNode* prev = it;

        while (heir->m_pLink[0] != &m_NilNode)
        {
          up[top++] = prev = heir;

          heir = heir->m_pLink[0];
        }

        ToErase = heir;
        ToOverride = it;

        prev->m_pLink[prev == it] = heir->m_pLink[1];
        prev->m_pLink[prev == it]->m_pParent = prev;

        //ToOverride->m_Key = ToErase->m_Key;
        //ToOverride->m_Value = ToErase->m_Value;
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

    root->m_pParent = &m_NilNode;


    // if necessary, swap nodes
    if (ToErase != &m_NilNode)
    {
      aeNode* parent = ToOverride->m_pParent;

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
    ReleaseNode (ToOverride);

    return root;
  }

  template < class KEY, class VALUE, class COMPARE, bool NO_DEBUG_ALLOCATOR>
  typename aeMap<KEY, VALUE, COMPARE, NO_DEBUG_ALLOCATOR>::iterator aeMap<KEY, VALUE, COMPARE, NO_DEBUG_ALLOCATOR>::erase (const iterator& pos)
  {
    AE_CHECK_DEV (pos.m_pElement != nullptr, "aeMap::erase: The iterator (pos) is invalid.");

    iterator temp (pos);
    ++temp;
    erase (pos.key ());
    return temp;
  }

  template < class KEY, class VALUE, class COMPARE, bool NO_DEBUG_ALLOCATOR>
  typename aeMap<KEY, VALUE, COMPARE, NO_DEBUG_ALLOCATOR>::reverse_iterator aeMap<KEY, VALUE, COMPARE, NO_DEBUG_ALLOCATOR>::erase (const reverse_iterator& pos)
  {
    AE_CHECK_DEV (pos.m_pElement != nullptr, "aeMap::erase: The reverse_iterator (pos) is invalid.");

    reverse_iterator temp (pos);
    ++temp;
    erase (pos.key ());
    return temp;
  }
}

#endif

