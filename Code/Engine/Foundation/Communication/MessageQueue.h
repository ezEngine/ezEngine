
#pragma once

#include <Foundation/Communication/Message.h>
#include <Foundation/Containers/Deque.h>
#include <Foundation/Threading/Lock.h>

/// \todo document and test
template <typename MetaDataType, typename MutexType>
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
  ezMessageQueueBase(ezAllocatorBase* pAllocator);

  /// \brief No memory is allocated during construction.
  ezMessageQueueBase(const ezMessageQueueBase& rhs, ezAllocatorBase* pAllocator);

  /// \brief Destructor.
  ~ezMessageQueueBase();

  /// \brief Assignment operator.
  void operator=(const ezMessageQueueBase& rhs);

public:
  Entry& operator[](ezUInt32 uiIndex);
  const Entry& operator[](ezUInt32 uiIndex) const;

  ezUInt32 GetCount() const;
  bool IsEmpty() const;

  void Clear();
  void Reserve(ezUInt32 uiCount);
  void Compact();

  // thread safe
  void Enqueue(ezMessage* pMessage, const MetaDataType& metaData);

  // thread safe
  bool TryDequeue(ezMessage*& out_pMessage, MetaDataType& out_metaData);
  bool TryPeek(ezMessage*& out_pMessage, MetaDataType& out_metaData);

  Entry& Peek();
  void Dequeue();

  template <typename C>
  void Sort();

private:
  ezDeque<Entry, ezNullAllocatorWrapper> m_Queue;
  MutexType m_mutex;
};

template <typename MetaDataType, typename MutexType, 
  typename AllocatorWrapper = ezDefaultAllocatorWrapper>
class ezMessageQueue : public ezMessageQueueBase<MetaDataType, MutexType>
{
public:
  ezMessageQueue();
  ezMessageQueue(ezAllocatorBase* pAllocator);

  ezMessageQueue(const ezMessageQueue<MetaDataType, MutexType, AllocatorWrapper>& rhs);
  ezMessageQueue(const ezMessageQueueBase<MetaDataType, MutexType>& rhs);

  void operator=(const ezMessageQueue<MetaDataType, MutexType, AllocatorWrapper>& rhs);
  void operator=(const ezMessageQueueBase<MetaDataType, MutexType>& rhs);
};

#include <Foundation/Communication/Implementation/MessageQueue_inl.h>

