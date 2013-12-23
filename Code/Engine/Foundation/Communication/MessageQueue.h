
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
  ezMessageQueueBase(ezIAllocator* pStorageAllocator, ezIAllocator* pQueueAllocator);

  /// \brief No memory is allocated during construction.
  ezMessageQueueBase(const ezMessageQueueBase& rhs, ezIAllocator* pStorageAllocator, ezIAllocator* pQueueAllocator);

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

  void Enqueue(const ezMessage& message, const MetaDataType& metaData);

  bool TryDequeue(ezMessage*& out_pMessage, MetaDataType& out_metaData);
  bool TryPeek(ezMessage*& out_pMessage, MetaDataType& out_metaData);

  template <typename C>
  void Sort();

  ezIAllocator* GetStorageAllocator() const;

private:
  ezDeque<Entry, ezNullAllocatorWrapper> m_Queue;
  ezIAllocator* m_pStorageAllocator;
  MutexType m_mutex;
};

template <typename MetaDataType, typename MutexType, 
  typename StorageAllocatorWrapper = ezDefaultAllocatorWrapper, 
  typename QueueAllocatorWrapper = ezDefaultAllocatorWrapper>
class ezMessageQueue : public ezMessageQueueBase<MetaDataType, MutexType>
{
public:
  ezMessageQueue();
  ezMessageQueue(ezIAllocator* pStorageAllocator, ezIAllocator* pQueueAllocator);

  ezMessageQueue(const ezMessageQueue<MetaDataType, MutexType, StorageAllocatorWrapper, QueueAllocatorWrapper>& rhs);
  ezMessageQueue(const ezMessageQueueBase<MetaDataType, MutexType>& rhs);

  void operator=(const ezMessageQueue<MetaDataType, MutexType, StorageAllocatorWrapper, QueueAllocatorWrapper>& rhs);
  void operator=(const ezMessageQueueBase<MetaDataType, MutexType>& rhs);
};

#include <Foundation/Communication/Implementation/MessageQueue_inl.h>

