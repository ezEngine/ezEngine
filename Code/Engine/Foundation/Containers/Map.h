#pragma once

#include <Foundation/Containers/Deque.h>

template <typename KeyType, typename ValueType, typename Comparer>
class ezMapBase;

/// \brief Base class for all iterators.
template <typename KeyType, typename ValueType, typename Comparer, bool REVERSE>
struct ezMapBaseConstIteratorBase
{
  using iterator_category = std::forward_iterator_tag;
  using value_type = ezMapBaseConstIteratorBase<KeyType, ValueType, Comparer, false>;
  using difference_type = std::ptrdiff_t;
  using pointer = ezMapBaseConstIteratorBase<KeyType, ValueType, Comparer, false>*;
  using reference = ezMapBaseConstIteratorBase<KeyType, ValueType, Comparer, false>&;

  EZ_DECLARE_POD_TYPE();

  /// \brief Constructs an invalid iterator.
  EZ_ALWAYS_INLINE ezMapBaseConstIteratorBase()
    : m_pElement(nullptr)
  {
  } // [tested]

  /// \brief Checks whether this iterator points to a valid element.
  EZ_ALWAYS_INLINE bool IsValid() const { return (m_pElement != nullptr); } // [tested]

  /// \brief Checks whether the two iterators point to the same element.
  EZ_ALWAYS_INLINE bool operator==(const ezMapBaseConstIteratorBase& it2) const { return (m_pElement == it2.m_pElement); }
  EZ_ADD_DEFAULT_OPERATOR_NOTEQUAL(const ezMapBaseConstIteratorBase&);

  /// \brief Returns the 'key' of the element that this iterator points to.
  EZ_FORCE_INLINE const KeyType& Key() const
  {
    EZ_ASSERT_DEBUG(IsValid(), "Cannot access the 'key' of an invalid iterator.");
    return m_pElement->m_Key;
  } // [tested]

  /// \brief Returns the 'value' of the element that this iterator points to.
  EZ_FORCE_INLINE const ValueType& Value() const
  {
    EZ_ASSERT_DEBUG(IsValid(), "Cannot access the 'value' of an invalid iterator.");
    return m_pElement->m_Value;
  } // [tested]

  /// \brief Returns '*this' to enable foreach
  EZ_ALWAYS_INLINE ezMapBaseConstIteratorBase& operator*() { return *this; } // [tested]

  /// \brief Advances the iterator to the next element in the map. The iterator will not be valid anymore, if the end is reached.
  void Next(); // [tested]

  /// \brief Advances the iterator to the previous element in the map. The iterator will not be valid anymore, if the end is reached.
  void Prev(); // [tested]

  /// \brief Shorthand for 'Next'
  EZ_ALWAYS_INLINE void operator++() { Next(); } // [tested]

  /// \brief Shorthand for 'Prev'
  EZ_ALWAYS_INLINE void operator--() { Prev(); } // [tested]

protected:
  void Advance(const ezInt32 dir0, const ezInt32 dir1);

  friend class ezMapBase<KeyType, ValueType, Comparer>;

  EZ_ALWAYS_INLINE explicit ezMapBaseConstIteratorBase(typename ezMapBase<KeyType, ValueType, Comparer>::Node* pInit)
    : m_pElement(pInit)
  {
  }

  typename ezMapBase<KeyType, ValueType, Comparer>::Node* m_pElement;

#if EZ_ENABLED(EZ_USE_CPP20_OPERATORS)
public:
  struct Pointer
  {
    std::pair<const KeyType&, const ValueType&> value;
    const std::pair<const KeyType&, const ValueType&>* operator->() const { return &value; }
  };

  EZ_ALWAYS_INLINE Pointer operator->() const
  {
    return Pointer{.value = {Key(), Value()}};
  }

  // This function is used to return the values for structured bindings.
  // The number and type of each slot are defined in the inl file.
  template <std::size_t Index>
  std::tuple_element_t<Index, ezMapBaseConstIteratorBase>& get() const
  {
    if constexpr (Index == 0)
      return Key();
    if constexpr (Index == 1)
      return Value();
  }
#endif
};

/// \brief Forward Iterator to iterate over all elements in sorted order.
template <typename KeyType, typename ValueType, typename Comparer, bool REVERSE>
struct ezMapBaseIteratorBase : public ezMapBaseConstIteratorBase<KeyType, ValueType, Comparer, REVERSE>
{
  using iterator_category = std::forward_iterator_tag;
  using value_type = ezMapBaseIteratorBase<KeyType, ValueType, Comparer, REVERSE>;
  using difference_type = std::ptrdiff_t;
  using pointer = ezMapBaseIteratorBase<KeyType, ValueType, Comparer, REVERSE>*;
  using reference = ezMapBaseIteratorBase<KeyType, ValueType, Comparer, REVERSE>&;

  EZ_DECLARE_POD_TYPE();

  /// \brief Constructs an invalid iterator.
  EZ_ALWAYS_INLINE ezMapBaseIteratorBase()
    : ezMapBaseConstIteratorBase<KeyType, ValueType, Comparer, REVERSE>()
  {
  }

  /// \brief Returns the 'value' of the element that this iterator points to.
  EZ_FORCE_INLINE ValueType& Value()
  {
    EZ_ASSERT_DEBUG(this->IsValid(), "Cannot access the 'value' of an invalid iterator.");
    return this->m_pElement->m_Value;
  }

  /// \brief Returns the 'value' of the element that this iterator points to.
  EZ_FORCE_INLINE ValueType& Value() const
  {
    EZ_ASSERT_DEBUG(this->IsValid(), "Cannot access the 'value' of an invalid iterator.");
    return this->m_pElement->m_Value;
  }

  /// \brief Returns '*this' to enable foreach
  EZ_ALWAYS_INLINE ezMapBaseIteratorBase& operator*() { return *this; } // [tested]

private:
  friend class ezMapBase<KeyType, ValueType, Comparer>;

  EZ_ALWAYS_INLINE explicit ezMapBaseIteratorBase(typename ezMapBase<KeyType, ValueType, Comparer>::Node* pInit)
    : ezMapBaseConstIteratorBase<KeyType, ValueType, Comparer, REVERSE>(pInit)
  {
  }

#if EZ_ENABLED(EZ_USE_CPP20_OPERATORS)
public:
  struct Pointer
  {
    std::pair<const KeyType&, ValueType&> value;
    const std::pair<const KeyType&, ValueType&>* operator->() const { return &value; }
  };

  EZ_ALWAYS_INLINE Pointer operator->() const
  {
    return Pointer{.value = {ezMapBaseConstIteratorBase<KeyType, ValueType, Comparer, REVERSE>::Key(), Value()}};
  }


  // These functions are used to return the values for structured bindings.
  // The number and type of type of each slot are defined in the inl file.

  template <std::size_t Index>
  std::tuple_element_t<Index, ezMapBaseIteratorBase>& get()
  {
    if constexpr (Index == 0)
      return ezMapBaseConstIteratorBase<KeyType, ValueType, Comparer, REVERSE>::Key();
    if constexpr (Index == 1)
      return Value();
  }

  template <std::size_t Index>
  std::tuple_element_t<Index, ezMapBaseIteratorBase>& get() const
  {
    if constexpr (Index == 0)
      return ezMapBaseConstIteratorBase<KeyType, ValueType, Comparer, REVERSE>::Key();
    if constexpr (Index == 1)
      return Value();
  }
#endif
};

/// \brief An associative container. Similar to STL::map
///
/// A map allows to store key/value pairs. This in turn allows to search for values by looking them
/// up with a certain key. Key/Value pairs can also be erased again.
/// This container is implemented using a balanced tree (red-black tree, Anderson tree variant), which means
/// the order of insertions/erasures is not important, since it can never create a degenerated tree.
///
/// Performance characteristics:
/// - All operations: O(log n) - insertion, erasure, lookup, bounds checking
/// - Memory usage: One node per key/value pair plus tree overhead
/// - Iteration: O(n) in sorted key order
/// - No reallocation of existing elements (stable pointers/references)
///
/// Use when:
/// - You need sorted iteration by key
/// - Stable element addresses are important
/// - Range queries (lower_bound, upper_bound) are needed
/// - Predictable O(log n) performance is required
///
/// Consider ezHashTable instead when:
/// - You don't need sorted iteration
/// - You want O(1) average case performance
/// - Memory usage is more critical
///
/// KeyType is the key type. For example a string.
/// ValueType is the value type. For example int.
/// Comparer is a helper class that implements a strictly weak-ordering comparison for Key types.
template <typename KeyType, typename ValueType, typename Comparer>
class ezMapBase
{

public:
  using ConstIterator = ezMapBaseConstIteratorBase<KeyType, ValueType, Comparer, false>;
  using ConstReverseIterator = ezMapBaseConstIteratorBase<KeyType, ValueType, Comparer, true>;

  using Iterator = ezMapBaseIteratorBase<KeyType, ValueType, Comparer, false>;
  using ReverseIterator = ezMapBaseIteratorBase<KeyType, ValueType, Comparer, true>;

private:
  friend ConstIterator;
  friend ConstReverseIterator;
  friend Iterator;
  friend ReverseIterator;
  struct Node;

  /// \brief Only used by the sentinel node.
  struct NilNode
  {
    Node* m_pParent = nullptr;
    Node* m_pLink[2] = {nullptr, nullptr};
    ezUInt8 m_uiLevel = 0;
  };

  /// \brief A node storing the key/value pair.
  struct Node : public NilNode
  {
    KeyType m_Key;
    ValueType m_Value;
  };

protected:
  /// \brief Initializes the map to be empty.
  ezMapBase(const Comparer& comparer, ezAllocator* pAllocator); // [tested]

  /// \brief Copies all key/value pairs from the given map into this one.
  ezMapBase(const ezMapBase<KeyType, ValueType, Comparer>& cc, ezAllocator* pAllocator); // [tested]

  /// \brief Destroys all elements from the map.
  ~ezMapBase(); // [tested]

  /// \brief Copies all key/value pairs from the given map into this one.
  void operator=(const ezMapBase<KeyType, ValueType, Comparer>& rhs);

public:
  /// \brief Returns whether there are no elements in the map. O(1) operation.
  bool IsEmpty() const; // [tested]

  /// \brief Returns the number of elements currently stored in the map. O(1) operation.
  ezUInt32 GetCount() const; // [tested]

  /// \brief Destroys all elements in the map and resets its size to zero.
  void Clear(); // [tested]

  /// \brief Returns an Iterator to the very first element.
  Iterator GetIterator(); // [tested]

  /// \brief Returns a ReverseIterator to the very last element.
  ReverseIterator GetReverseIterator(); // [tested]

  /// \brief Returns a constant Iterator to the very first element.
  ConstIterator GetIterator() const; // [tested]

  /// \brief Returns a constant ReverseIterator to the very last element.
  ConstReverseIterator GetReverseIterator() const; // [tested]

  /// \brief Inserts the key/value pair into the tree and returns an Iterator to it. O(log n) operation.
  template <typename CompatibleKeyType, typename CompatibleValueType>
  Iterator Insert(CompatibleKeyType&& key, CompatibleValueType&& value); // [tested]

  /// \brief Erases the key/value pair with the given key, if it exists. O(log n) operation.
  template <typename CompatibleKeyType>
  bool Remove(const CompatibleKeyType& key); // [tested]

  /// \brief Erases the key/value pair at the given Iterator. O(log n) operation. Returns an iterator to the element after the given
  /// iterator.
  Iterator Remove(const Iterator& pos); // [tested]

  /// \brief Searches for the given key and returns an iterator to it. If it did not exist yet, it is default-created. \a bExisted is set to
  /// true, if the key was found, false if it needed to be created.
  template <typename CompatibleKeyType>
  Iterator FindOrAdd(CompatibleKeyType&& key, bool* out_pExisted = nullptr); // [tested]

  /// \brief Allows read/write access to the value stored under the given key. If there is no such key, a new element is
  /// default-constructed.
  template <typename CompatibleKeyType>
  ValueType& operator[](const CompatibleKeyType& key); // [tested]

  /// \brief Returns whether an entry with the given key was found and if found writes out the corresponding value to out_value.
  template <typename CompatibleKeyType>
  bool TryGetValue(const CompatibleKeyType& key, ValueType& out_value) const; // [tested]

  /// \brief Returns whether an entry with the given key was found and if found writes out the pointer to the corresponding value to out_pValue.
  template <typename CompatibleKeyType>
  bool TryGetValue(const CompatibleKeyType& key, const ValueType*& out_pValue) const; // [tested]

  /// \brief Returns whether an entry with the given key was found and if found writes out the pointer to the corresponding value to out_pValue.
  template <typename CompatibleKeyType>
  bool TryGetValue(const CompatibleKeyType& key, ValueType*& out_pValue) const; // [tested]

  /// \brief Returns a pointer to the value of the entry with the given key if found, otherwise returns nullptr.
  template <typename CompatibleKeyType>
  const ValueType* GetValue(const CompatibleKeyType& key) const; // [tested]

  /// \brief Returns a pointer to the value of the entry with the given key if found, otherwise returns nullptr.
  template <typename CompatibleKeyType>
  ValueType* GetValue(const CompatibleKeyType& key); // [tested]

  /// \brief Either returns the value of the entry with the given key, if found, or the provided default value.
  template <typename CompatibleKeyType>
  const ValueType& GetValueOrDefault(const CompatibleKeyType& key, const ValueType& defaultValue) const; // [tested]

  /// \brief Searches for key, returns an Iterator to it or an invalid iterator, if no such key is found. O(log n) operation.
  template <typename CompatibleKeyType>
  Iterator Find(const CompatibleKeyType& key); // [tested]

  /// \brief Returns an Iterator to the element with a key equal or larger than the given key. Returns an invalid iterator, if there is no
  /// such element.
  template <typename CompatibleKeyType>
  Iterator LowerBound(const CompatibleKeyType& key); // [tested]

  /// \brief Returns an Iterator to the element with a key that is LARGER than the given key. Returns an invalid iterator, if there is no
  /// such element.
  template <typename CompatibleKeyType>
  Iterator UpperBound(const CompatibleKeyType& key); // [tested]

  /// \brief Searches for key, returns an Iterator to it or an invalid iterator, if no such key is found. O(log n) operation.
  template <typename CompatibleKeyType>
  ConstIterator Find(const CompatibleKeyType& key) const; // [tested]

  /// \brief Checks whether the given key is in the container.
  template <typename CompatibleKeyType>
  bool Contains(const CompatibleKeyType& key) const; // [tested]

  /// \brief Returns an Iterator to the element with a key equal or larger than the given key. Returns an invalid iterator, if there is no
  /// such element.
  template <typename CompatibleKeyType>
  ConstIterator LowerBound(const CompatibleKeyType& key) const; // [tested]

  /// \brief Returns an Iterator to the element with a key that is LARGER than the given key. Returns an invalid iterator, if there is no
  /// such element.
  template <typename CompatibleKeyType>
  ConstIterator UpperBound(const CompatibleKeyType& key) const; // [tested]

  /// \brief Returns the allocator that is used by this instance.
  ezAllocator* GetAllocator() const { return m_Elements.GetAllocator(); }

  /// \brief Comparison operator
  bool operator==(const ezMapBase<KeyType, ValueType, Comparer>& rhs) const; // [tested]
  EZ_ADD_DEFAULT_OPERATOR_NOTEQUAL(const ezMapBase<KeyType, ValueType, Comparer>&);

  /// \brief Returns the amount of bytes that are currently allocated on the heap.
  ezUInt64 GetHeapMemoryUsage() const { return m_Elements.GetHeapMemoryUsage(); } // [tested]

  /// \brief Swaps this map with the other one.
  void Swap(ezMapBase<KeyType, ValueType, Comparer>& other); // [tested]

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
  Node* AcquireNode(CompatibleKeyType&& key, ValueType&& value, ezUInt8 uiLevel, Node* pParent);

  /// \brief Destroys the given node.
  void ReleaseNode(Node* pNode);

  // \brief Red-Black Tree stuff(Anderson Tree to be exact).
  // Code taken from here: http://eternallyconfuzzled.com/tuts/datastructures/jsw_tut_andersson.aspx
  Node* SkewNode(Node* root);
  Node* SplitNode(Node* root);
  void Insert(const KeyType& key, const ValueType& value, Node*& pInsertedNode);
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

  /// \brief Stack of recently discarded nodes to quickly acquire new nodes.
  Node* m_pFreeElementStack;

  /// \brief Sentinel node.
  NilNode m_NilNode;

  /// \brief Data store. Keeps all the nodes.
  ezDeque<Node, ezNullAllocatorWrapper, false> m_Elements;

  /// \brief Number of active nodes in the tree.
  ezUInt32 m_uiCount;

  /// \brief Comparer object
  Comparer m_Comparer;
};


/// \brief \see ezMapBase
template <typename KeyType, typename ValueType, typename Comparer = ezCompareHelper<KeyType>, typename AllocatorWrapper = ezDefaultAllocatorWrapper>
class ezMap : public ezMapBase<KeyType, ValueType, Comparer>
{
public:
  ezMap();
  explicit ezMap(ezAllocator* pAllocator);
  ezMap(const Comparer& comparer, ezAllocator* pAllocator);

  ezMap(const ezMap<KeyType, ValueType, Comparer, AllocatorWrapper>& other);
  ezMap(const ezMapBase<KeyType, ValueType, Comparer>& other);

  void operator=(const ezMap<KeyType, ValueType, Comparer, AllocatorWrapper>& rhs);
  void operator=(const ezMapBase<KeyType, ValueType, Comparer>& rhs);
};

template <typename KeyType, typename ValueType, typename Comparer>
typename ezMapBase<KeyType, ValueType, Comparer>::Iterator begin(ezMapBase<KeyType, ValueType, Comparer>& ref_container)
{
  return ref_container.GetIterator();
}

template <typename KeyType, typename ValueType, typename Comparer>
typename ezMapBase<KeyType, ValueType, Comparer>::ConstIterator begin(const ezMapBase<KeyType, ValueType, Comparer>& container)
{
  return container.GetIterator();
}

template <typename KeyType, typename ValueType, typename Comparer>
typename ezMapBase<KeyType, ValueType, Comparer>::ConstIterator cbegin(const ezMapBase<KeyType, ValueType, Comparer>& container)
{
  return container.GetIterator();
}

template <typename KeyType, typename ValueType, typename Comparer>
typename ezMapBase<KeyType, ValueType, Comparer>::Iterator end(ezMapBase<KeyType, ValueType, Comparer>& ref_container)
{
  EZ_IGNORE_UNUSED(ref_container);
  return typename ezMapBase<KeyType, ValueType, Comparer>::Iterator();
}

template <typename KeyType, typename ValueType, typename Comparer>
typename ezMapBase<KeyType, ValueType, Comparer>::ConstIterator end(const ezMapBase<KeyType, ValueType, Comparer>& container)
{
  EZ_IGNORE_UNUSED(container);
  return typename ezMapBase<KeyType, ValueType, Comparer>::ConstIterator();
}

template <typename KeyType, typename ValueType, typename Comparer>
typename ezMapBase<KeyType, ValueType, Comparer>::ConstIterator cend(const ezMapBase<KeyType, ValueType, Comparer>& container)
{
  EZ_IGNORE_UNUSED(container);
  return typename ezMapBase<KeyType, ValueType, Comparer>::ConstIterator();
}

#include <Foundation/Containers/Implementation/Map_inl.h>
