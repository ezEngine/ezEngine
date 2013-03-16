#pragma once

#include <Foundation/Containers/Deque.h>

/// A set container that only stores whether an element resides in it or not. Similar to STL::set
/*! Sets are similar to maps that do not store a value (or only a bool that is always true).
    Sets can be used to reduce an unordered number of elements to only those that are unique.
    Insertion/erasure/lookup in sets is quite fast (O (log n)).
    This container is implemented with a red-black tree, so it will always be a balanced tree.
*/
template <typename KeyType, typename Comparer>
class ezSetBase
{
private:
  struct Node;

  /// Only used by the sentinel node.
  struct NilNode
  {
    NilNode();

    ezUInt16 m_uiLevel;
    Node* m_pParent;
    Node* m_pLink[2];
  };

  /// A node storing the key
  struct Node : public NilNode
  {
    KeyType m_Key;
  };

public:

  /// Base class for all iterators.
  struct Iterator
  {
    EZ_DECLARE_POD_TYPE();

    /// Constructs an invalid iterator.
    EZ_FORCE_INLINE Iterator()                         : m_pElement(NULL) { }

    /// Checks whether this iterator points to a valid element.
    EZ_FORCE_INLINE bool IsValid() const { return (m_pElement != NULL); }

    /// Checks whether the two iterators point to the same element.
    EZ_FORCE_INLINE bool operator==(const typename ezSetBase<KeyType, Comparer>::Iterator& it2) const { return (m_pElement == it2.m_pElement); }

    /// Checks whether the two iterators point to the same element.
    EZ_FORCE_INLINE bool operator!=(const typename ezSetBase<KeyType, Comparer>::Iterator& it2) const { return (m_pElement != it2.m_pElement); }

    /// Returns the 'key' of the element that this iterator points to.
    EZ_FORCE_INLINE const KeyType&   Key ()  const { EZ_ASSERT(IsValid(), "Cannot access the 'key' of an invalid iterator."); return m_pElement->m_Key;   }

    /// Advances the iterator to the next element in the set. The iterator will not be valid anymore, if the end is reached.
    void Next();

    /// Advances the iterator to the previous element in the set. The iterator will not be valid anymore, if the end is reached.
    void Prev();

    /// Shorthand for 'Next'
    EZ_FORCE_INLINE void operator++() { Next();  }

    /// Shorthand for 'Prev'
    EZ_FORCE_INLINE void operator--() { Prev(); }

  protected:
    friend class ezSetBase<KeyType, Comparer>;

    EZ_FORCE_INLINE explicit Iterator(Node* pInit)              : m_pElement(pInit) { }

    Node* m_pElement;
  };

protected:

  /// Initializes the set to be empty.
  ezSetBase(ezIAllocator* pAllocator); // [tested]

  /// Copies all keys from the given set into this one.
  ezSetBase(const ezSetBase<KeyType, Comparer>& cc, ezIAllocator* pAllocator); // [tested]

  /// Destroys all elements in the set.
  ~ezSetBase(); // [tested]

  /// Copies all keys from the given set into this one.
  void operator= (const ezSetBase<KeyType, Comparer>& rhs);

public:
  /// Returns whether there are no elements in the set. O(1) operation.
  bool IsEmpty() const; // [tested]

  /// Returns the number of elements currently stored in the set. O(1) operation.
  ezUInt32 GetCount() const; // [tested]

  /// Destroys all elements in the set and resets its size to zero.
  void Clear(); // [tested]

  /// Returns a constant Iterator to the very first element.
  Iterator GetIterator() const; // [tested]

  /// Returns a constant Iterator to the very last element. For reverse traversal.
  Iterator GetLastIterator() const; // [tested]

  /// Inserts the key into the tree and returns an Iterator to it. O(log n) operation.
  Iterator Insert(const KeyType& key); // [tested]

  /// Erases the element with the given key, if it exists. O(log n) operation.
  void Erase(const KeyType& key); // [tested]

  /// Erases the element at the given Iterator. O(1) operation(nearly).
  Iterator Erase(const Iterator& pos); // [tested]

  /// Searches for key, returns an Iterator to it or an invalid iterator, if no such key is found. O(log n) operation.
  Iterator Find(const KeyType& key); // [tested]

  /// Returns an Iterator to the element with a key equal or larger than the given key. Returns an invalid iterator, if there is no such element.
  Iterator LowerBound(const KeyType& key); // [tested]

  /// Returns an Iterator to the element with a key that is LARGER than the given key. Returns an invalid iterator, if there is no such element.
  Iterator UpperBound(const KeyType& key); // [tested]

  /// Returns the allocator that is used by this instance.
  ezIAllocator* GetAllocator() const { return m_Elements.GetAllocator(); }

private:
  Node* Internal_Find(const KeyType& key) const;
  Node* Internal_LowerBound(const KeyType& key) const;
  Node* Internal_UpperBound(const KeyType& key) const;

private:
  void Constructor();

  /// Creates one new node and initializes it.
  Node* AcquireNode(const KeyType& key, int m_uiLevel, Node* pParent);

  /// Destroys the given node.
  void ReleaseNode(Node* pNode);

  // Red-Black Tree stuff(Anderson Tree to be exact).
  // Code taken from here: http://eternallyconfuzzled.com/tuts/datastructures/jsw_tut_andersson.aspx
  Node* SkewNode(Node* root);
  Node* SplitNode(Node* root);
  Node* Insert(Node* root, const KeyType& key, Node*& pInsertedNode);
  Node* Erase(Node* root, const KeyType& key);

  /// Returns the left-most node of the tree(smallest key).
  Node* GetLeftMost() const;

  /// Returns the right-most node of the tree(largest key).
  Node* GetRightMost() const;

  /// Root node of the tree.
  Node* m_pRoot;

  /// Sentinel node.
  NilNode m_NilNode;

  /// Number of active nodes in the tree.
  ezUInt32 m_uiCount;

  /// Data store. Keeps all the nodes.
  ezDeque<Node, ezNullAllocatorWrapper, false> m_Elements;

  /// Stack of recently discarded nodes to quickly acquire new nodes.
  Node* m_pFreeElementStack;
};


template <typename KeyType, typename Comparer = ezCompareHelper<KeyType>, typename AllocatorWrapper = ezDefaultAllocatorWrapper>
class ezSet : public ezSetBase<KeyType, Comparer>
{
public:
  ezSet();
  ezSet(ezIAllocator* pAllocator);

  ezSet(const ezSet<KeyType, Comparer, AllocatorWrapper>& other);
  ezSet(const ezSetBase<KeyType, Comparer>& other);

  void operator=(const ezSet<KeyType, Comparer, AllocatorWrapper>& rhs);
  void operator=(const ezSetBase<KeyType, Comparer>& rhs);
};

#include <Foundation/Containers/Implementation/Set_inl.h>

