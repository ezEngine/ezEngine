#pragma once

#include <Foundation/Containers/Deque.h>

/// \brief A set container that only stores whether an element resides in it or not. Similar to STL::set
///
/// Sets are similar to maps that do not store a value (or only a bool that is always true).
/// Sets can be used to reduce an unordered number of elements to only those that are unique.
/// This container is implemented with a red-black tree (Anderson tree variant), ensuring balanced operations.
///
/// Performance characteristics:
/// - All operations: O(log n) - insertion, erasure, lookup, bounds checking
/// - Memory usage: One node per element plus tree overhead
/// - Iteration: O(n) in sorted order
/// - Set operations (union, intersection, difference): O(n + m) where n, m are set sizes
///
/// Use when:
/// - You need sorted iteration over unique elements
/// - Fast lookup/insertion/removal is important
/// - Set operations (union, intersection) are needed
/// - Memory usage is not the primary concern
///
/// Consider ezHashSet instead when:
/// - You don't need sorted iteration
/// - You want O(1) average case performance
/// - Memory usage is more critical
template <typename KeyType, typename Comparer>
class ezSetBase
{
private:
  struct Node;

  /// \brief Only used by the sentinel node.
  struct NilNode
  {
    ezUInt16 m_uiLevel = 0;
    Node* m_pParent = nullptr;
    Node* m_pLink[2] = {nullptr, nullptr};
  };

  /// \brief A node storing the key
  struct Node : public NilNode
  {
    KeyType m_Key;
  };

public:
  /// \brief Base class for all iterators.
  template <bool REVERSE>
  struct IteratorBase
  {
    using iterator_category = std::forward_iterator_tag;
    using value_type = IteratorBase<REVERSE>;
    using difference_type = std::ptrdiff_t;
    using pointer = IteratorBase<REVERSE>*;
    using reference = IteratorBase<REVERSE>&;

    EZ_DECLARE_POD_TYPE();

    /// \brief Constructs an invalid iterator.
    EZ_ALWAYS_INLINE IteratorBase()
      : m_pElement(nullptr)
    {
    } // [tested]

    /// \brief Checks whether this iterator points to a valid element.
    EZ_ALWAYS_INLINE bool IsValid() const { return (m_pElement != nullptr); } // [tested]

    /// \brief Checks whether the two iterators point to the same element.
    EZ_ALWAYS_INLINE bool operator==(const typename ezSetBase<KeyType, Comparer>::IteratorBase<REVERSE>& it2) const { return (m_pElement == it2.m_pElement); }
    EZ_ADD_DEFAULT_OPERATOR_NOTEQUAL(const typename ezSetBase<KeyType, Comparer>::IteratorBase<REVERSE>&);

    /// \brief Returns the 'key' of the element that this iterator points to.
    EZ_FORCE_INLINE const KeyType& Key() const
    {
      EZ_ASSERT_DEBUG(IsValid(), "Cannot access the 'key' of an invalid iterator.");
      return m_pElement->m_Key;
    } // [tested]

    /// \brief Returns the 'key' of the element that this iterator points to.
    EZ_ALWAYS_INLINE const KeyType& operator*() const { return Key(); }

    /// \brief Advances the iterator to the next element in the set. The iterator will not be valid anymore, if the end is reached.
    void Next(); // [tested]

    /// \brief Advances the iterator to the previous element in the set. The iterator will not be valid anymore, if the end is reached.
    void Prev(); // [tested]

    /// \brief Shorthand for 'Next'
    EZ_ALWAYS_INLINE void operator++() { Next(); } // [tested]

    /// \brief Shorthand for 'Prev'
    EZ_ALWAYS_INLINE void operator--() { Prev(); } // [tested]

  protected:
    void Advance(ezInt32 dir0, ezInt32 dir1);

    friend class ezSetBase<KeyType, Comparer>;

    EZ_ALWAYS_INLINE explicit IteratorBase(Node* pInit)
      : m_pElement(pInit)
    {
    }

    Node* m_pElement;
  };

  using Iterator = IteratorBase<false>;
  using ReverseIterator = IteratorBase<true>;

protected:
  /// \brief Initializes the set to be empty.
  ezSetBase(const Comparer& comparer, ezAllocator* pAllocator); // [tested]

  /// \brief Copies all keys from the given set into this one.
  ezSetBase(const ezSetBase<KeyType, Comparer>& cc, ezAllocator* pAllocator); // [tested]

  /// \brief Destroys all elements in the set.
  ~ezSetBase(); // [tested]

  /// \brief Copies all keys from the given set into this one.
  void operator=(const ezSetBase<KeyType, Comparer>& rhs); // [tested]

public:
  /// \brief Returns whether there are no elements in the set. O(1) operation.
  bool IsEmpty() const; // [tested]

  /// \brief Returns the number of elements currently stored in the set. O(1) operation.
  ezUInt32 GetCount() const; // [tested]

  /// \brief Destroys all elements in the set and resets its size to zero.
  void Clear(); // [tested]

  /// \brief Returns a constant Iterator to the very first element.
  Iterator GetIterator() const; // [tested]

  /// \brief Returns a constant ReverseIterator to the very last element.
  ReverseIterator GetReverseIterator() const; // [tested]

  /// \brief Inserts the key into the tree and returns an Iterator to it. O(log n) operation.
  template <typename CompatibleKeyType>
  Iterator Insert(CompatibleKeyType&& key); // [tested]

  /// \brief Erases the element with the given key, if it exists. O(log n) operation.
  template <typename CompatibleKeyType>
  bool Remove(const CompatibleKeyType& key); // [tested]

  /// \brief Erases the element at the given Iterator. O(log n) operation.
  Iterator Remove(const Iterator& pos); // [tested]

  /// \brief Searches for key, returns an Iterator to it or an invalid iterator, if no such key is found. O(log n) operation.
  template <typename CompatibleKeyType>
  Iterator Find(const CompatibleKeyType& key) const; // [tested]

  /// \brief Checks whether the given key is in the container.
  template <typename CompatibleKeyType>
  bool Contains(const CompatibleKeyType& key) const; // [tested]

  /// \brief Checks whether all keys of the given set are in the container.
  bool ContainsSet(const ezSetBase<KeyType, Comparer>& operand) const; // [tested]

  /// \brief Returns an Iterator to the element with a key equal or larger than the given key. Returns an invalid iterator, if there is no such
  /// element.
  template <typename CompatibleKeyType>
  Iterator LowerBound(const CompatibleKeyType& key) const; // [tested]

  /// \brief Returns an Iterator to the element with a key that is LARGER than the given key. Returns an invalid iterator, if there is no such
  /// element.
  template <typename CompatibleKeyType>
  Iterator UpperBound(const CompatibleKeyType& key) const; // [tested]

  /// \brief Makes this set the union of itself and the operand.
  void Union(const ezSetBase<KeyType, Comparer>& operand); // [tested]

  /// \brief Makes this set the difference of itself and the operand, i.e. subtracts operand.
  void Difference(const ezSetBase<KeyType, Comparer>& operand); // [tested]

  /// \brief Makes this set the intersection of itself and the operand.
  void Intersection(const ezSetBase<KeyType, Comparer>& operand); // [tested]

  /// \brief Returns the allocator that is used by this instance.
  ezAllocator* GetAllocator() const { return m_Elements.GetAllocator(); }

  /// \brief Comparison operator
  bool operator==(const ezSetBase<KeyType, Comparer>& rhs) const; // [tested]
  EZ_ADD_DEFAULT_OPERATOR_NOTEQUAL(const ezSetBase<KeyType, Comparer>&);

  /// \brief Returns the amount of bytes that are currently allocated on the heap.
  ezUInt64 GetHeapMemoryUsage() const { return m_Elements.GetHeapMemoryUsage(); } // [tested]

  /// \brief Swaps this map with the other one.
  void Swap(ezSetBase<KeyType, Comparer>& other); // [tested]

private:
  template <typename CompatibleKeyType>
  Node* Internal_Find(const CompatibleKeyType& key) const;
  template <typename CompatibleKeyType>
  Node* Internal_LowerBound(const CompatibleKeyType& key) const;
  template <typename CompatibleKeyType>
  Node* Internal_UpperBound(const CompatibleKeyType& key) const;

private:
  void Constructor();

  /// \brief Creates one new node and initializes it.
  template <typename CompatibleKeyType>
  Node* AcquireNode(CompatibleKeyType&& key, ezUInt16 uiLevel, Node* pParent);

  /// \brief Destroys the given node.
  void ReleaseNode(Node* pNode);

  /// \brief Red-Black Tree stuff(Anderson Tree to be exact).
  ///
  /// Code taken from here: http://eternallyconfuzzled.com/tuts/datastructures/jsw_tut_andersson.aspx
  Node* SkewNode(Node* root);
  Node* SplitNode(Node* root);

  template <typename CompatibleKeyType>
  Node* Insert(Node* root, CompatibleKeyType&& key, Node*& pInsertedNode);
  template <typename CompatibleKeyType>
  Node* Remove(Node* root, const CompatibleKeyType& key, bool& bRemoved);

  /// \brief Returns the left-most node of the tree(smallest key).
  Node* GetLeftMost() const;

  /// \brief Returns the right-most node of the tree(largest key).
  Node* GetRightMost() const;

  /// \brief Needed during Swap() to fix up the NilNode pointers from one container to the other
  void SwapNilNode(Node*& pCurNode, NilNode* pOld, NilNode* pNew);

  /// \brief Root node of the tree.
  Node* m_pRoot;

  /// \brief Sentinel node.
  NilNode m_NilNode;

  /// \brief Number of active nodes in the tree.
  ezUInt32 m_uiCount;

  /// \brief Data store. Keeps all the nodes.
  ezDeque<Node, ezNullAllocatorWrapper, false> m_Elements;

  /// \brief Stack of recently discarded nodes to quickly acquire new nodes.
  Node* m_pFreeElementStack;

  /// \brief Comparer object
  Comparer m_Comparer;
};

/// \brief \see ezSetBase
template <typename KeyType, typename Comparer = ezCompareHelper<KeyType>, typename AllocatorWrapper = ezDefaultAllocatorWrapper>
class ezSet : public ezSetBase<KeyType, Comparer>
{
public:
  ezSet();
  explicit ezSet(ezAllocator* pAllocator);
  ezSet(const Comparer& comparer, ezAllocator* pAllocator);

  ezSet(const ezSet<KeyType, Comparer, AllocatorWrapper>& other);
  ezSet(const ezSetBase<KeyType, Comparer>& other);

  void operator=(const ezSet<KeyType, Comparer, AllocatorWrapper>& rhs);
  void operator=(const ezSetBase<KeyType, Comparer>& rhs);
};


template <typename KeyType, typename Comparer>
typename ezSetBase<KeyType, Comparer>::Iterator begin(ezSetBase<KeyType, Comparer>& ref_container)
{
  return ref_container.GetIterator();
}

template <typename KeyType, typename Comparer>
typename ezSetBase<KeyType, Comparer>::Iterator begin(const ezSetBase<KeyType, Comparer>& container)
{
  return container.GetIterator();
}

template <typename KeyType, typename Comparer>
typename ezSetBase<KeyType, Comparer>::Iterator cbegin(const ezSetBase<KeyType, Comparer>& container)
{
  return container.GetIterator();
}

template <typename KeyType, typename Comparer>
typename ezSetBase<KeyType, Comparer>::Iterator end(ezSetBase<KeyType, Comparer>& ref_container)
{
  EZ_IGNORE_UNUSED(ref_container);
  return typename ezSetBase<KeyType, Comparer>::Iterator();
}

template <typename KeyType, typename Comparer>
typename ezSetBase<KeyType, Comparer>::Iterator end(const ezSetBase<KeyType, Comparer>& container)
{
  EZ_IGNORE_UNUSED(container);
  return typename ezSetBase<KeyType, Comparer>::Iterator();
}

template <typename KeyType, typename Comparer>
typename ezSetBase<KeyType, Comparer>::Iterator cend(const ezSetBase<KeyType, Comparer>& container)
{
  EZ_IGNORE_UNUSED(container);
  return typename ezSetBase<KeyType, Comparer>::Iterator();
}


#include <Foundation/Containers/Implementation/Set_inl.h>
