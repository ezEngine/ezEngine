
#pragma once

#include <Foundation/Communication/Message.h>
#include <Foundation/Containers/Deque.h>
#include <Foundation/Threading/Lock.h>
#include <Foundation/Threading/Mutex.h>

/// \brief Implementation of a message queue on top of a deque.
///
/// Enqueue and TryDequeue/TryPeek methods are thread safe all the others are not. To ensure 
/// thread safety for all methods the queue can be locked using ezLock like a mutex.
/// Every entry consists of a pointer to a message and some meta data. 
/// Lifetime of the enqueued messages needs to be managed by the user.
/// \see ezMessage
template <typename MetaDataType>
class ezMessageQueueBase
{
public:
  struct Entry
  {
    EZ_DECLARE_POD_TYPE();

    ezMessage* m_pMessage;
    MetaDataType m_MetaData;
  };

protected:

  /// \brief No memory is allocated during construction.
  ezMessageQueueBase(ezAllocatorBase* pAllocator); // [tested]

  /// \brief No memory is allocated during construction.
  ezMessageQueueBase(const ezMessageQueueBase& rhs, ezAllocatorBase* pAllocator);

  /// \brief Destructor.
  ~ezMessageQueueBase(); // [tested]

  /// \brief Assignment operator.
  void operator=(const ezMessageQueueBase& rhs);

public:
  /// \brief Returns the element at the given index. Not thread safe.
  Entry& operator[](ezUInt32 uiIndex); // [tested]

  /// \brief Returns the element at the given index. Not thread safe.
  const Entry& operator[](ezUInt32 uiIndex) const; // [tested]

  /// \brief Returns the number of active elements in the queue.
  ezUInt32 GetCount() const;

  /// \brief Returns true, if the queue does not contain any elements.
  bool IsEmpty() const;

  /// \brief Destructs all elements and sets the count to zero. Does not deallocate any data.
  void Clear();

  /// \brief Expands the queue so it can at least store the given capacity.
  void Reserve(ezUInt32 uiCount);

  /// \brief Tries to compact the array to avoid wasting memory.The resulting capacity is at least 'GetCount' (no elements get removed).
  void Compact();

  /// \brief Enqueues the given message and meta-data. This method is thread safe.
  void Enqueue(ezMessage* pMessage, const MetaDataType& metaData); // [tested]

  /// \brief Dequeues the first element if the queue is not empty and returns true. Returns false if the queue is empty. This method is thread safe.
  bool TryDequeue(ezMessage*& out_pMessage, MetaDataType& out_metaData); // [tested]

  /// \brief Gives the first element if the queue is not empty and returns true. Returns false if the queue is empty. This method is thread safe.
  bool TryPeek(ezMessage*& out_pMessage, MetaDataType& out_metaData); // [tested]

  /// \brief Returns the first element in the queue. Not thread safe.
  Entry& Peek();

  /// \brief Removes the first element from the queue. Not thread safe.
  void Dequeue();

  /// \brief Sort with explicit comparer. Not thread safe.
  template <typename Comparer>
  void Sort(const Comparer& comparer); // [tested]

  /// \brief Attempts to acquire an exclusive lock on the queue. Do not use this method directly but use ezLock instead.
  void Acquire(); // [tested]

  /// \brief Releases a lock that has been previously acquired. Do not use this method directly but use ezLock instead.
  void Release(); // [tested]

private:
  ezDeque<Entry, ezNullAllocatorWrapper> m_Queue;
  ezMutex m_Mutex;
};

/// \brief \see ezMessageQueueBase
template <typename MetaDataType, typename AllocatorWrapper = ezDefaultAllocatorWrapper>
class ezMessageQueue : public ezMessageQueueBase<MetaDataType>
{
public:
  ezMessageQueue();
  ezMessageQueue(ezAllocatorBase* pAllocator);

  ezMessageQueue(const ezMessageQueue<MetaDataType, AllocatorWrapper>& rhs);
  ezMessageQueue(const ezMessageQueueBase<MetaDataType>& rhs);

  void operator=(const ezMessageQueue<MetaDataType, AllocatorWrapper>& rhs);
  void operator=(const ezMessageQueueBase<MetaDataType>& rhs);
};

#include <Foundation/Communication/Implementation/MessageQueue_inl.h>

