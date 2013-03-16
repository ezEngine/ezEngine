#pragma once

#include <Foundation/Containers/Deque.h>

/// A List-class, similar to STL::list
/*! This container class allows fast insertion and erasure of elements.
    Access is limited to iteration from front-to-back or back-to-front, there is no random-access.
    Define the type of object to store in the list via the template argument T.
*/
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

  /// A list-node, containing data and prev/next pointers
  struct ListElement : public ListElementBase
  {
    ListElement() : ListElementBase() { } 
    explicit ListElement(const T& data);

    T m_Data;
  };

  /// base-class for all iterators
  struct ConstIterator
  {
    EZ_DECLARE_POD_TYPE();
    ConstIterator() : m_pElement (NULL)  {}

    /// Equality comparison operator.
    bool operator== (typename ezListBase<T>::ConstIterator it2) const { return (m_pElement == it2.m_pElement); }

    /// Inequality comparison operator.
    bool operator!= (typename ezListBase<T>::ConstIterator it2) const { return (m_pElement != it2.m_pElement); }

    /// Grants access to the node-data.
    const T& operator* () const { return (m_pElement->m_Data);  }

    /// Grants access to the node-data.
    const T* operator->() const { return (&m_pElement->m_Data); }

    /// Moves the iterator to the next node.
    void Next() { m_pElement = m_pElement->m_pNext; }

    /// Moves the iterator to the previous node.
    void Prev() { m_pElement = m_pElement->m_pPrev; }

    /// Checks whether this iterator points to a valid element (and not the start/end of the list)
    bool IsValid() const { return ((m_pElement != NULL) && (m_pElement->m_pPrev != NULL) && (m_pElement->m_pNext != NULL)); }

    void operator++ () { Next();  }
    void operator-- () { Prev();  }

  private:
    friend class ezListBase<T>;

    ConstIterator(ListElement* pInit) : m_pElement (pInit) {}

    ListElement* m_pElement;
  };

public:
  /// A forward-iterator. Allows sequential access from front-to-back.
  struct Iterator : public ConstIterator
  {
    EZ_DECLARE_POD_TYPE();
    Iterator() : ConstIterator () {}

    T& operator* () { return (this->m_pElement->m_Data);  }
    T* operator->() { return (&this->m_pElement->m_Data); }

  private:
    friend class ezListBase<T>;

    explicit Iterator(ListElement* pInit) : ConstIterator (pInit) {}
  };

protected:
  /// Initializes the list to be empty.
  ezListBase(ezIAllocator* pAllocator); // [tested]

  /// Initializes the list with a copy from another list.
  ezListBase(const ezListBase<T>& cc, ezIAllocator* pAllocator); // [tested]

  /// Destroys the list and all its content.
  ~ezListBase(); // [tested]

  /// Copies the list cc into this list.
  void operator=(const ezListBase<T>& cc); // [tested]

public:
  /// Clears the list, afterwards it is empty.
  void Clear(); // [tested]

  /// Returns the number of elements in the list. O(1) operation.
  ezUInt32 GetCount() const; // [tested]

  /// Returns whether size == 0. O(1) operation.
  bool IsEmpty() const; // [tested]

  /// Returns the very first element in the list.
  const T& PeekFront() const; // [tested]

  /// Returns the very last element in the list.
  const T& PeekBack() const; // [tested]

  /// Returns the very first element in the list.
  T& PeekFront(); // [tested]

  /// Returns the very last element in the list.
  T& PeekBack(); // [tested]

  /// Appends a default-constructed element to the list.
  void PushBack(); // [tested]

  /// Appands a copy of the given element to the list.
  void PushBack(const T& element); // [tested]

  /// Removes the very last element from the list.
  void PopBack(); // [tested]

  /// Appends a default-constructed element to the front of the list.
  void PushFront(); // [tested]

  /// Appands a copy of the given element to the front of the list.
  void PushFront (const T& element); // [tested]

  /// Removes the very first element from the list.
  void PopFront(); // [tested]

  /// Sets the number of elements that are in the list.
  void SetCount(ezUInt32 uiNewSize); // [tested]

  /// Inserts one element before the position defined by the iterator.
  Iterator Insert(const Iterator& pos, const T& data); // [tested]

  /// Inserts the range defined by [first;last) after pos.
  void Insert(const Iterator& pos, ConstIterator first, const ConstIterator& last);

  /// Erases the element pointed to by the iterator.
  Iterator Erase(const Iterator& pos); // [tested]

  /// Erases range [first; last).
  Iterator Erase(Iterator first, const Iterator& last);

  /// Returns an iterator to the first list-element.
  Iterator GetIterator(); // [tested]

  /// Returns an iterator to the last list-element. Can be used for reverse iteration.
  Iterator GetLastIterator(); // [tested]

  /// Returns an iterator pointing behind the last element. Necessary if one wants to insert elements at the end of a list.
  Iterator GetEndIterator(); // [tested]

  /// Returns a const-iterator to the first list-element.
  ConstIterator GetIterator() const; // [tested]

  /// Returns a const-iterator to the last list-element. Can be used for reverse iteration.
  ConstIterator GetLastIterator() const; // [tested]

  /// Returns a const-iterator pointing behind the last element. Necessary if one wants to insert elements at the end of a list.
  ConstIterator GetEndIterator() const; // [tested]

  /// Returns the allocator that is used by this instance.
  ezIAllocator* GetAllocator() const { return m_Elements.GetAllocator(); }

private:
  /// Sentinel node before the first element.
  ListElementBase m_First;

  /// Sentinel node after the last element.
  ListElementBase m_Last;

  // small hack to get around const problems
  Iterator m_End;

  /// The number of active elements in the list.
  ezUInt32 m_uiCount;

  /// Acquires and initializes one node.
  ListElement* AcquireNode(const T& data);

  /// Destructs one node and puts it into the free-list.
  void ReleaseNode(ListElement* pNode);

  /// Data-Store. Contains all the elements.
  ezDeque<ListElement, ezNullAllocatorWrapper, false> m_Elements;

  /// Stack that holds recently freed nodes, that can be quickly reused.
  ListElement* m_pFreeElementStack;
};


template <typename T, typename AllocatorWrapper = ezDefaultAllocatorWrapper>
class ezList : public ezListBase<T>
{
public:
  ezList();
  ezList(ezIAllocator* pAllocator);

  ezList(const ezList<T, AllocatorWrapper>& other);
  ezList(const ezListBase<T>& other);

  void operator=(const ezList<T, AllocatorWrapper>& rhs);
  void operator=(const ezListBase<T>& rhs);
};

#include <Foundation/Containers/Implementation/List_inl.h>

