#pragma once

#include <Foundation/Containers/Deque.h>

/// \brief An associative container. Similar to STL::map
///
/// A map allows to store key/value pairs. This in turn allows to search for values by looking them
/// up with a certain key. Key/Value pairs can also be erased again.
/// All insertion/erasure/lookup functions take O(log n) time. The map is implemented using a balanced tree
/// (a red-black tree), which means the order of insertions/erasures is not important, since it can never
/// create a degenerated tree, and performance will always stay the same.\n
/// \n
/// KeyType is the key type. For example a string.\n
/// ValueType is the value type. For example int.\n
/// Comparer is a helper class that implements a strictly weak-ordering comparison for Key types.
template <typename KeyType, typename ValueType, typename Comparer>
class ezMapBase
{
private:
  struct Node;

  /// \brief Only used by the sentinel node.
  struct NilNode
  {
    NilNode();

    Node* m_pParent;
    Node* m_pLink[2];
    ezUInt8 m_uiLevel;
  };

  /// \brief A node storing the key/value pair.
  struct Node : public NilNode
  {
    KeyType m_Key;
    ValueType m_Value;
  };

public:

  /// \brief Base class for all iterators.
  struct ConstIterator
  {
    typedef std::forward_iterator_tag iterator_category;
    typedef ConstIterator value_type;
    typedef ptrdiff_t difference_type;
    typedef ConstIterator* pointer;
    typedef ConstIterator& reference;

    EZ_DECLARE_POD_TYPE();

    /// \brief Constructs an invalid iterator.
    EZ_FORCE_INLINE ConstIterator() : m_pElement(nullptr) { } // [tested]

    /// \brief Checks whether this iterator points to a valid element.
    EZ_FORCE_INLINE bool IsValid() const { return (m_pElement != nullptr); } // [tested]

    /// \brief Checks whether the two iterators point to the same element.
    EZ_FORCE_INLINE bool operator==(const typename ezMapBase<KeyType, ValueType, Comparer>::ConstIterator& it2) const { return (m_pElement == it2.m_pElement); }

    /// \brief Checks whether the two iterators point to the same element.
    EZ_FORCE_INLINE bool operator!=(const typename ezMapBase<KeyType, ValueType, Comparer>::ConstIterator& it2) const { return (m_pElement != it2.m_pElement); }

    /// \brief Returns the 'key' of the element that this iterator points to.
    EZ_FORCE_INLINE const KeyType&   Key ()  const { EZ_ASSERT_DEV(IsValid(), "Cannot access the 'key' of an invalid iterator."); return m_pElement->m_Key;   } // [tested]

    /// \brief Returns the 'value' of the element that this iterator points to.
    EZ_FORCE_INLINE const ValueType& Value() const { EZ_ASSERT_DEV(IsValid(), "Cannot access the 'value' of an invalid iterator."); return m_pElement->m_Value; } // [tested]

    /// \brief Advances the iterator to the next element in the map. The iterator will not be valid anymore, if the end is reached.
    void Next(); // [tested]

    /// \brief Advances the iterator to the previous element in the map. The iterator will not be valid anymore, if the end is reached.
    void Prev(); // [tested]

    /// \brief Shorthand for 'Next'
    EZ_FORCE_INLINE void operator++() { Next();  } // [tested]

    /// \brief Shorthand for 'Prev'
    EZ_FORCE_INLINE void operator--() { Prev(); } // [tested]

  protected:
    friend class ezMapBase<KeyType, ValueType, Comparer>;

    EZ_FORCE_INLINE explicit ConstIterator(Node* pInit)              : m_pElement(pInit) { }

    Node* m_pElement;
  };

  /// \brief Forward Iterator to iterate over all elements in sorted order.
  struct Iterator : public ConstIterator
  {
    typedef std::forward_iterator_tag iterator_category;
    typedef Iterator value_type;
    typedef ptrdiff_t difference_type;
    typedef Iterator* pointer;
    typedef Iterator& reference;

    // this is required to pull in the const version of this function
    using ConstIterator::Value;

    EZ_DECLARE_POD_TYPE();

    /// \brief Constructs an invalid iterator.
    EZ_FORCE_INLINE Iterator()                   : ConstIterator()      { }

    /// \brief Returns the 'value' of the element that this iterator points to.
    EZ_FORCE_INLINE ValueType& Value() { EZ_ASSERT_DEV(this->IsValid(), "Cannot access the 'value' of an invalid iterator."); return this->m_pElement->m_Value; }

    /// \brief Returns the 'value' of the element that this iterator points to.
    EZ_FORCE_INLINE ValueType& operator*() { return Value(); }

  private:
    friend class ezMapBase<KeyType, ValueType, Comparer>;

    EZ_FORCE_INLINE explicit Iterator(Node* pInit)        : ConstIterator(pInit) { }
  };

protected:

  /// \brief Initializes the map to be empty.
  ezMapBase(const Comparer& comparer, ezAllocatorBase* pAllocator); // [tested]

  /// \brief Copies all key/value pairs from the given map into this one.
  ezMapBase(const ezMapBase<KeyType, ValueType, Comparer>& cc, ezAllocatorBase* pAllocator); // [tested]

  /// \brief Destroys all elements from the map.
  ~ezMapBase(); // [tested]

  /// \brief Copies all key/value pairs from the given map into this one.
  void operator= (const ezMapBase<KeyType, ValueType, Comparer>& rhs);

public:
  /// \brief Returns whether there are no elements in the map. O(1) operation.
  bool IsEmpty() const; // [tested]

  /// \brief Returns the number of elements currently stored in the map. O(1) operation.
  ezUInt32 GetCount() const; // [tested]

  /// \brief Destroys all elements in the map and resets its size to zero.
  void Clear(); // [tested]

  /// \brief Returns an Iterator to the very first element.
  Iterator GetIterator(); // [tested]

  /// \brief Returns a constant Iterator to the very first element.
  ConstIterator GetIterator() const; // [tested]

  /// \brief Returns an Iterator to the very last element. For reverse traversal.
  Iterator GetLastIterator(); // [tested]

  /// \brief Returns a constant Iterator to the very last element. For reverse traversal.
  ConstIterator GetLastIterator() const; // [tested]

  /// \brief Inserts the key/value pair into the tree and returns an Iterator to it. O(log n) operation.
  Iterator Insert(const KeyType& key, const ValueType& value); // [tested]

  /// \brief Erases the key/value pair with the given key, if it exists. O(log n) operation.
  bool Remove(const KeyType& key); // [tested]

  /// \brief Erases the key/value pair at the given Iterator. O(log n) operation. Returns an iterator to the element after the given iterator.
  Iterator Remove(const Iterator& pos); // [tested]

  /// \brief Searches for the given key and returns an iterator to it. If it did not exist yet, it is default-created. \a bExisted is set to true, if the key was found, false if it needed to be created.
  Iterator FindOrAdd(const KeyType& key, bool* bExisted = nullptr); // [tested]

  /// \brief Allows read/write access to the value stored under the given key. If there is no such key, a new element is default-constructed.
  ValueType& operator[](const KeyType& key); // [tested]

  /// \brief Searches for key, returns an Iterator to it or an invalid iterator, if no such key is found. O(log n) operation.
  Iterator Find(const KeyType& key); // [tested]

  /// \brief Returns an Iterator to the element with a key equal or larger than the given key. Returns an invalid iterator, if there is no such element.
  Iterator LowerBound(const KeyType& key); // [tested]

  /// \brief Returns an Iterator to the element with a key that is LARGER than the given key. Returns an invalid iterator, if there is no such element.
  Iterator UpperBound(const KeyType& key); // [tested]

  /// \brief Searches for key, returns an Iterator to it or an invalid iterator, if no such key is found. O(log n) operation.
  ConstIterator Find(const KeyType& key) const; // [tested]

  /// \brief Checks whether the given key is in the container.
  bool Contains(const KeyType& key) const; // [tested]

  /// \brief Returns an Iterator to the element with a key equal or larger than the given key. Returns an invalid iterator, if there is no such element.
  ConstIterator LowerBound(const KeyType& key) const; // [tested]

  /// \brief Returns an Iterator to the element with a key that is LARGER than the given key. Returns an invalid iterator, if there is no such element.
  ConstIterator UpperBound(const KeyType& key) const; // [tested]

  /// \brief Returns the allocator that is used by this instance.
  ezAllocatorBase* GetAllocator() const { return m_Elements.GetAllocator(); }

  /// \brief Comparison operator
  bool operator==(const ezMapBase<KeyType, ValueType, Comparer>& rhs) const; // [tested]

  /// \brief Comparison operator
  bool operator!=(const ezMapBase<KeyType, ValueType, Comparer>& rhs) const; // [tested]

  /// \brief Returns the amount of bytes that are currently allocated on the heap.
  ezUInt64 GetHeapMemoryUsage() const { return m_Elements.GetHeapMemoryUsage(); } // [tested]

private:
  Node* Internal_Find(const KeyType& key) const;
  Node* Internal_LowerBound(const KeyType& key) const;
  Node* Internal_UpperBound(const KeyType& key) const;

private:
  void Constructor();

  /// \brief Creates one new node and initializes it.
  Node* AcquireNode(const KeyType& key, const ValueType& value, int m_uiLevel, Node* pParent);

  /// \brief Destroys the given node.
  void ReleaseNode(Node* pNode);

  // \brief Red-Black Tree stuff(Anderson Tree to be exact).
  // Code taken from here: http://eternallyconfuzzled.com/tuts/datastructures/jsw_tut_andersson.aspx
  Node* SkewNode(Node* root);
  Node* SplitNode(Node* root);
  void Insert(const KeyType& key, const ValueType& value, Node*& pInsertedNode);
  Node* Remove(Node* root, const KeyType& key, bool& bRemoved);

  /// \brief Returns the left-most node of the tree(smallest key).
  Node* GetLeftMost() const;

  /// \brief Returns the right-most node of the tree(largest key).
  Node* GetRightMost() const;

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


/// \brief \see ezMapBase
template <typename KeyType, typename ValueType, typename Comparer = ezCompareHelper<KeyType>, typename AllocatorWrapper = ezDefaultAllocatorWrapper>
class ezMap : public ezMapBase<KeyType, ValueType, Comparer>
{
public:
  ezMap();
  ezMap(const Comparer& comparer, ezAllocatorBase* pAllocator);

  ezMap(const ezMap<KeyType, ValueType, Comparer, AllocatorWrapper>& other);
  ezMap(const ezMapBase<KeyType, ValueType, Comparer>& other);

  void operator=(const ezMap<KeyType, ValueType, Comparer, AllocatorWrapper>& rhs);
  void operator=(const ezMapBase<KeyType, ValueType, Comparer>& rhs);
};

template <typename KeyType, typename ValueType, typename Comparer>
typename ezMapBase<KeyType, ValueType, Comparer>::Iterator begin(ezMapBase<KeyType, ValueType, Comparer>& container) { return container.GetIterator(); }

template <typename KeyType, typename ValueType, typename Comparer>
typename ezMapBase<KeyType, ValueType, Comparer>::ConstIterator begin(const ezMapBase<KeyType, ValueType, Comparer>& container) { return container.GetIterator(); }

template <typename KeyType, typename ValueType, typename Comparer>
typename ezMapBase<KeyType, ValueType, Comparer>::ConstIterator cbegin(const ezMapBase<KeyType, ValueType, Comparer>& container) { return container.GetIterator(); }

template <typename KeyType, typename ValueType, typename Comparer>
typename ezMapBase<KeyType, ValueType, Comparer>::Iterator end(ezMapBase<KeyType, ValueType, Comparer>& container) { return typename ezMapBase<KeyType, ValueType, Comparer>::Iterator(); }

template <typename KeyType, typename ValueType, typename Comparer>
typename ezMapBase<KeyType, ValueType, Comparer>::ConstIterator end(const ezMapBase<KeyType, ValueType, Comparer>& container) { return typename ezMapBase<KeyType, ValueType, Comparer>::ConstIterator(); }

template <typename KeyType, typename ValueType, typename Comparer>
typename ezMapBase<KeyType, ValueType, Comparer>::ConstIterator cend(const ezMapBase<KeyType, ValueType, Comparer>& container) { return typename ezMapBase<KeyType, ValueType, Comparer>::ConstIterator(); }

#include <Foundation/Containers/Implementation/Map_inl.h>

