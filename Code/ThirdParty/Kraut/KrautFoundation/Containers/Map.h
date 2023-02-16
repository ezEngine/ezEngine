#ifndef AE_FOUNDATION_CONTAINERS_MAP_H
#define AE_FOUNDATION_CONTAINERS_MAP_H

#include "Deque.h"
#include "Stack.h"

namespace AE_NS_FOUNDATION
{
  /*
    TODO: erase (iterator, reverse_iterator)
  */
  
  //! An associative container. Similar to STL::map
  /*! A map allows to store key/value pairs. This in turn allows to search for values by looking them
      up with a certain key. Key/Value pairs can also be erased again.
      All insertion/erasure/lookup functions take O(log n) time. The map is implemented using a balanced tree
      (a red-black tree), which means the order of insertions/erasures is not important, since it can never
      create a degenerated tree, and performance will always stay the same.\n
      \n
      KEY is the key type. For example aeString.\n
      VALUE is the value type. For example int.\n
      COMPARE is a functor class that implements a strictly weak-ordering comparison for KEY types.
  */
  template < class KEY, class VALUE, class COMPARE = aeCompareLess<KEY>, bool NO_DEBUG_ALLOCATOR = false>
  class aeMap
  {
  private:
    //! A node storing the key/value pair.
    struct aeNode 
    {
      aeNode (void);

      KEY m_Key;
      VALUE m_Value;

      aeUInt16 m_uiLevel;
      aeNode* m_pParent;
      aeNode* m_pLink[2];
    };

    //! Base class for all iterators.
    struct iterator_base
    {
      iterator_base (void)                    : m_pElement (nullptr) {}
      iterator_base (const iterator_base& cc) : m_pElement (cc.m_pElement) {}
      iterator_base (aeNode* pInit)           : m_pElement (pInit) {}

      bool operator== (const typename aeMap<KEY, VALUE, COMPARE, NO_DEBUG_ALLOCATOR>::iterator_base& it2) const { return (m_pElement == it2.m_pElement); }
      bool operator!= (const typename aeMap<KEY, VALUE, COMPARE, NO_DEBUG_ALLOCATOR>::iterator_base& it2) const { return (m_pElement != it2.m_pElement); }

      const KEY&   key   (void) const { return (m_pElement->m_Key);   }
      const VALUE& value (void) const { return (m_pElement->m_Value); }

    protected:
      friend class aeMap<KEY, VALUE, COMPARE, NO_DEBUG_ALLOCATOR>;
      aeNode* m_pElement;

      void forward (void);
      void backward (void);
    };

  public:

    //! Forward iterator to iterate over all elements in sorted order.
    struct iterator : public iterator_base
    {
      iterator (void)               : iterator_base () {}
      iterator (const iterator& cc) : iterator_base (cc.m_pElement) {}
      iterator (aeNode* pInit)      : iterator_base (pInit) {}
      
      // prefix
      void operator++ () { forward ();  }
      void operator-- () { backward (); }

      VALUE& value (void) { return (m_pElement->m_Value);}
    };

    //! Forward iterator to iterate over all elements in reversed sorted order.
    struct reverse_iterator : public iterator_base
    {
      reverse_iterator (void)                       : iterator_base () {}
      reverse_iterator (const reverse_iterator& cc) : iterator_base (cc.m_pElement) {}
      reverse_iterator (aeNode* pInit)              : iterator_base (pInit) {}
      
      // prefix
      void operator++ () {backward ();}
      void operator-- () {forward ();}

      VALUE& value (void) { return (m_pElement->m_Value);}
    };

    //! Forward iterator to iterate over all elements in sorted order. Only allows read-access.
    struct const_iterator : public iterator_base
    {
      const_iterator (void)                     : iterator_base () {}
      const_iterator (const const_iterator& cc) : iterator_base (cc.m_pElement) {}
      const_iterator (const iterator& cc)       : iterator_base (cc.m_pElement) {}
      const_iterator (aeNode* pInit)            : iterator_base (pInit) {}
      
      // prefix
      void operator++ () { forward ();  }
      void operator-- () { backward (); }
    };

    //! Forward iterator to iterate over all elements in reversed sorted order. Only allows read-access.
    struct const_reverse_iterator : public iterator_base
    {
      const_reverse_iterator (void)                             : iterator_base () {}
      const_reverse_iterator (const const_reverse_iterator& cc) : iterator_base (cc.m_pElement) {}
      const_reverse_iterator (const reverse_iterator& cc)       : iterator_base (cc.m_pElement) {}
      const_reverse_iterator (aeNode* pInit)                    : iterator_base (pInit) {}
      
      // prefix
      void operator++ () { backward (); }
      void operator-- () { forward ();  }
    };

  public:
    //! Initializes the map to be empty.
    aeMap (void);
    //! Copies all key/value pairs from the given map into this one.
    aeMap (const aeMap<KEY, VALUE, COMPARE, NO_DEBUG_ALLOCATOR>& cc);
    //! Destroys all elements from the map.
    ~aeMap ();

    //! Copies all key/value pairs from the given map into this one.
    void operator= (const aeMap<KEY, VALUE, COMPARE, NO_DEBUG_ALLOCATOR>& rhs);

    //! Returns whether there are no elements in the map. O(1) operation.
    bool empty (void) const;
    //! Returns the number of elements currently stored in the map. O(1) operation.
    aeUInt32 size (void) const;

    //! Destroys all elements in the map and resets its size to zero.
    void clear (void);

    //! Returns a forward iterator to the very first element.
    iterator begin (void);
    //! Returns a forward iterator to the element after the last element.
    iterator end (void);
    //! Returns a constant forward iterator to the very first element.
    const_iterator begin (void) const;
    //! Returns a constant forward iterator to the element after the last element.
    const_iterator end (void) const;
    //! Returns an iterator to the very last element. For reverse traversal.
    reverse_iterator rbegin (void);
    //! Returns an iterator to the element before the first element. For reverse traversal.
    reverse_iterator rend (void);
    //! Returns a constant iterator to the very last element. For reverse traversal.
    const_reverse_iterator rbegin (void) const;
    //! Returns a constant iterator to the element before the first element. For reverse traversal.
    const_reverse_iterator rend (void) const;

    //! Inserts the key/value pair into the tree and returns an iterator to it. O(log n) operation.
    iterator insert (const KEY& key, const VALUE& value);

    //! Erases the key/value pair with the given key, if it exists. O(log n) operation.
    void erase (const KEY& key);
    //! Erases the key/value pair at the given iterator. O(1) operation (nearly).
    iterator erase (const iterator& pos);
    //! Erases the key/value pair at the given iterator. O(1) operation (nearly).
    reverse_iterator erase (const reverse_iterator& pos);

    //! Allows read/write access to the value stored under the given key. If there is no such key, a new element is default-constructed.
    VALUE& operator[] (const KEY& key);

    //! Searches for key, returns an iterator to it or "end ()", if no such key is found. O(log n) operation.
    iterator find (const KEY& key);
    //! Returns an iterator to the element with a key equal or larger than the given key.
    iterator lower_bound (const KEY& key);
    //! Returns an iterator to the element with a key that is LARGER than the given key.
    iterator upper_bound (const KEY& key);

    //! Searches for key, returns an iterator to it or "end ()", if no such key is found. O(log n) operation.
    const_iterator find (const KEY& key) const;
    //! Returns an iterator to the element with a key equal or larger than the given key.
    const_iterator lower_bound (const KEY& key) const;
    //! Returns an iterator to the element with a key that is LARGER than the given key.
    const_iterator upper_bound (const KEY& key) const;

  private:
    //! Creates one new node and initializes it.
    aeNode* AccquireNode (const KEY& key, const VALUE& value, int m_uiLevel, aeNode* pParent);
    //! Destroys the given node.
    void ReleaseNode (aeNode* pNode);

    // Red-Black Tree stuff (Anderson Tree to be exact).
    // Code taken from here: http://eternallyconfuzzled.com/tuts/datastructures/jsw_tut_andersson.aspx
    aeNode* SkewNode (aeNode* root);
    aeNode* SplitNode (aeNode* root);
    aeNode* insert (aeNode* root, const KEY& key, const VALUE& value, aeNode*& pInsertedNode);
    aeNode* erase (aeNode* root, const KEY& key);

    //! Returns the left-most node of the tree (smallest key).
    aeNode* GetLeftMost (void) const;
    //! Returns the right-most node of the tree (largest key).
    aeNode* GetRightMost (void) const;

    //! Root node of the tree.
    aeNode* m_pRoot;
    //! Sentinel node.
    aeNode m_NilNode;

    //! Number of active nodes in the tree.
    aeUInt32 m_uiSize;

    //! Data store. Keeps all the nodes.
    aeDeque<aeNode, ((4096 / sizeof (aeNode)) < 32) ? 32 : (4096 / sizeof (aeNode)), false, NO_DEBUG_ALLOCATOR> m_Elements;
    //! Stack of recently discarded nodes to quickly accquire new nodes.
    aeStack<aeNode*, ((4096 / sizeof (aeNode)) < 32) ? 32 : (4096 / sizeof (aeNode)), NO_DEBUG_ALLOCATOR> m_FreeElements;

  };

}

#include "Inline/Map.inl"

#endif


