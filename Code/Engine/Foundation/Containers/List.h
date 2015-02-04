#pragma once

#include <Foundation/Containers/Deque.h>

/// \brief A List-class, similar to STL::list
///
/// This container class allows fast insertion and erasure of elements.
/// Access is limited to iteration from front-to-back or back-to-front, there is no random-access.
/// Define the type of object to store in the list via the template argument T.
template <typename T>
class ezListBase
{
private:

  struct ListElement;

  struct ListElementBase
  {
    ListElementBase();

    ListElement* m_pPrev;
    ListElement* m_pNext;
  };

  /// \brief A list-node, containing data and prev/next pointers
  struct ListElement : public ListElementBase
  {
    ListElement() : ListElementBase() { } 
    explicit ListElement(const T& data);

    T m_Data;
  };

  /// \brief base-class for all iterators
  struct ConstIterator
  {
    EZ_DECLARE_POD_TYPE();

    /// \brief Constructor.
    ConstIterator() : m_pElement (nullptr)  {} // [tested]

    /// \brief Equality comparison operator.
    bool operator== (typename ezListBase<T>::ConstIterator it2) const { return (m_pElement == it2.m_pElement); } // [tested]

    /// \brief Inequality comparison operator.
    bool operator!= (typename ezListBase<T>::ConstIterator it2) const { return (m_pElement != it2.m_pElement); } // [tested]

    /// \brief Grants access to the node-data.
    const T& operator* () const { return (m_pElement->m_Data);  } // [tested]

    /// \brief Grants access to the node-data.
    const T* operator->() const { return (&m_pElement->m_Data); } // [tested]

    /// \brief Moves the iterator to the next node.
    void Next() { m_pElement = m_pElement->m_pNext; } // [tested]

    /// \brief Moves the iterator to the previous node.
    void Prev() { m_pElement = m_pElement->m_pPrev; } // [tested]

    /// \brief Checks whether this iterator points to a valid element (and not the start/end of the list)
    bool IsValid() const { return ((m_pElement != nullptr) && (m_pElement->m_pPrev != nullptr) && (m_pElement->m_pNext != nullptr)); } // [tested]

    /// \brief Moves the iterator to the next element in the list.
    void operator++ () { Next();  } // [tested]

    /// \brief Moves the iterator to the previous element in the list.
    void operator-- () { Prev();  } // [tested]

  private:
    friend class ezListBase<T>;

    ConstIterator(ListElement* pInit) : m_pElement (pInit) {}

    ListElement* m_pElement;
  };

public:
  /// \brief A forward-iterator. Allows sequential access from front-to-back.
  struct Iterator : public ConstIterator
  {
    // this is required to pull in the const version of this function
    using ConstIterator::operator*;
    using ConstIterator::operator->;

    EZ_DECLARE_POD_TYPE();

    /// \brief Constructor.
    Iterator() : ConstIterator () {} // [tested]

    /// \brief Accesses the element stored in the node.
    T& operator* () { return (this->m_pElement->m_Data);  } // [tested]

    /// \brief Accesses the element stored in the node.
    T* operator->() { return (&this->m_pElement->m_Data); } // [tested]

  private:
    friend class ezListBase<T>;

    explicit Iterator(ListElement* pInit) : ConstIterator (pInit) {}
  };

protected:
  /// \brief Initializes the list to be empty.
  ezListBase(ezAllocatorBase* pAllocator); // [tested]

  /// \brief Initializes the list with a copy from another list.
  ezListBase(const ezListBase<T>& cc, ezAllocatorBase* pAllocator); // [tested]

  /// \brief Destroys the list and all its content.
  ~ezListBase(); // [tested]

  /// \brief Copies the list cc into this list.
  void operator=(const ezListBase<T>& cc); // [tested]

public:
  /// \brief Clears the list, afterwards it is empty.
  void Clear(); // [tested]

  /// \brief Returns the number of elements in the list. O(1) operation.
  ezUInt32 GetCount() const; // [tested]

  /// \brief Returns whether size == 0. O(1) operation.
  bool IsEmpty() const; // [tested]

  /// \brief Returns the very first element in the list.
  const T& PeekFront() const; // [tested]

  /// \brief Returns the very last element in the list.
  const T& PeekBack() const; // [tested]

  /// \brief Returns the very first element in the list.
  T& PeekFront(); // [tested]

  /// \brief Returns the very last element in the list.
  T& PeekBack(); // [tested]

  /// \brief Appends a default-constructed element to the list.
  void PushBack(); // [tested]

  /// \brief Appends a copy of the given element to the list.
  void PushBack(const T& element); // [tested]

  /// \brief Removes the very last element from the list.
  void PopBack(); // [tested]

  /// \brief Appends a default-constructed element to the front of the list.
  void PushFront(); // [tested]

  /// \brief Appends a copy of the given element to the front of the list.
  void PushFront (const T& element); // [tested]

  /// \brief Removes the very first element from the list.
  void PopFront(); // [tested]

  /// \brief Sets the number of elements that are in the list.
  void SetCount(ezUInt32 uiNewSize); // [tested]

  /// \brief Inserts one element before the position defined by the iterator.
  Iterator Insert(const Iterator& pos, const T& data); // [tested]

  /// \brief Inserts the range defined by [first;last) after pos.
  void Insert(const Iterator& pos, ConstIterator first, const ConstIterator& last);

  /// \brief Erases the element pointed to by the iterator.
  Iterator Remove(const Iterator& pos); // [tested]

  /// \brief Erases range [first; last).
  Iterator Remove(Iterator first, const Iterator& last);

  /// \brief Returns an iterator to the first list-element.
  Iterator GetIterator(); // [tested]

  /// \brief Returns an iterator to the last list-element. Can be used for reverse iteration.
  Iterator GetLastIterator(); // [tested]

  /// \brief Returns an iterator pointing behind the last element. Necessary if one wants to insert elements at the end of a list.
  Iterator GetEndIterator(); // [tested]

  /// \brief Returns a const-iterator to the first list-element.
  ConstIterator GetIterator() const; // [tested]

  /// \brief Returns a const-iterator to the last list-element. Can be used for reverse iteration.
  ConstIterator GetLastIterator() const; // [tested]

  /// \brief Returns a const-iterator pointing behind the last element. Necessary if one wants to insert elements at the end of a list.
  ConstIterator GetEndIterator() const; // [tested]

  /// \brief Returns the allocator that is used by this instance.
  ezAllocatorBase* GetAllocator() const { return m_Elements.GetAllocator(); }

  /// \brief Comparison operator
  bool operator==(const ezListBase<T>& rhs) const; // [tested]

  /// \brief Comparison operator
  bool operator!=(const ezListBase<T>& rhs) const; // [tested]

  /// \brief Returns the amount of bytes that are currently allocated on the heap.
  ezUInt64 GetHeapMemoryUsage() const { return m_Elements.GetHeapMemoryUsage(); } // [tested]

private:
  /// \brief Sentinel node before the first element.
  ListElementBase m_First;

  /// \brief Sentinel node after the last element.
  ListElementBase m_Last;

  // \brief Small hack to get around const problems.
  Iterator m_End;

  /// \brief The number of active elements in the list.
  ezUInt32 m_uiCount;

  /// \brief Acquires and initializes one node.
  ListElement* AcquireNode(const T& data);

  /// \brief Destructs one node and puts it into the free-list.
  void ReleaseNode(ListElement* pNode);

  /// \brief Data-Store. Contains all the elements.
  ezDeque<ListElement, ezNullAllocatorWrapper, false> m_Elements;

  /// \brief Stack that holds recently freed nodes, that can be quickly reused.
  ListElement* m_pFreeElementStack;
};


/// \brief \see ezListBase
template <typename T, typename AllocatorWrapper = ezDefaultAllocatorWrapper>
class ezList : public ezListBase<T>
{
public:
  ezList();
  ezList(ezAllocatorBase* pAllocator);

  ezList(const ezList<T, AllocatorWrapper>& other);
  ezList(const ezListBase<T>& other);

  void operator=(const ezList<T, AllocatorWrapper>& rhs);
  void operator=(const ezListBase<T>& rhs);
};

#include <Foundation/Containers/Implementation/List_inl.h>

