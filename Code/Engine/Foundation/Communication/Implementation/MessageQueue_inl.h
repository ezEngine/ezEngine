
template <typename MetaDataType, typename MutexType>
ezMessageQueueBase<MetaDataType, MutexType>::ezMessageQueueBase(ezAllocatorBase* pStorageAllocator, ezAllocatorBase* pQueueAllocator) :
  m_Queue(pQueueAllocator)
{
  m_pStorageAllocator = pStorageAllocator;
}

template <typename MetaDataType, typename MutexType>
ezMessageQueueBase<MetaDataType, MutexType>::ezMessageQueueBase(const ezMessageQueueBase& rhs, ezAllocatorBase* pStorageAllocator, ezAllocatorBase* pQueueAllocator) :
  m_Queue(pQueueAllocator)
{
  m_Queue = rhs.m_Queue;
  m_pStorageAllocator = pStorageAllocator;
}

template <typename MetaDataType, typename MutexType>
ezMessageQueueBase<MetaDataType, MutexType>::~ezMessageQueueBase()
{
  Clear();
}

template <typename MetaDataType, typename MutexType>
void ezMessageQueueBase<MetaDataType, MutexType>::operator=(const ezMessageQueueBase& rhs)
{
  m_Queue = rhs.m_Queue;
  m_pStorageAllocator = rhs.m_pStorageAllocator;
}

template <typename MetaDataType, typename MutexType>
EZ_FORCE_INLINE typename ezMessageQueueBase<MetaDataType, MutexType>::Entry& ezMessageQueueBase<MetaDataType, MutexType>::operator[](ezUInt32 uiIndex)
{
  return m_Queue[uiIndex];
}

template <typename MetaDataType, typename MutexType>
EZ_FORCE_INLINE const typename ezMessageQueueBase<MetaDataType, MutexType>::Entry& ezMessageQueueBase<MetaDataType, MutexType>::operator[](ezUInt32 uiIndex) const
{
  return m_Queue[uiIndex];
}

template <typename MetaDataType, typename MutexType>
EZ_FORCE_INLINE ezUInt32 ezMessageQueueBase<MetaDataType, MutexType>::GetCount() const
{
  return m_Queue.GetCount();
}  

template <typename MetaDataType, typename MutexType>
EZ_FORCE_INLINE bool ezMessageQueueBase<MetaDataType, MutexType>::IsEmpty() const
{
  return m_Queue.IsEmpty();
}

template <typename MetaDataType, typename MutexType>
void ezMessageQueueBase<MetaDataType, MutexType>::Clear()
{
  for (ezUInt32 i = 0; i < m_Queue.GetCount(); ++i)
  {
    EZ_DELETE_RAW_BUFFER(m_pStorageAllocator, m_Queue[i].m_pMessage);
  }

  m_Queue.Clear();
}

template <typename MetaDataType, typename MutexType>
EZ_FORCE_INLINE void ezMessageQueueBase<MetaDataType, MutexType>::Reserve(ezUInt32 uiCount)
{
  m_Queue.Reserve(uiCount);
}

template <typename MetaDataType, typename MutexType>
EZ_FORCE_INLINE void ezMessageQueueBase<MetaDataType, MutexType>::Compact()
{
  m_Queue.Compact();
}

template <typename MetaDataType, typename MutexType>
void ezMessageQueueBase<MetaDataType, MutexType>::Enqueue(const ezMessage& message, const MetaDataType& metaData)
{
  ezUInt8* pMessageStorage = EZ_NEW_RAW_BUFFER(m_pStorageAllocator, ezUInt8, message.GetSize());
  ezMemoryUtils::Copy(pMessageStorage, reinterpret_cast<const ezUInt8*>(&message), message.GetSize());

  Entry entry;
  entry.m_pMessage = reinterpret_cast<ezMessage*>(pMessageStorage);
  entry.m_MetaData = metaData;

  {
    ezLock<MutexType> lock(m_mutex);
    
    m_Queue.PushBack(entry);
  }
}

template <typename MetaDataType, typename MutexType>
bool ezMessageQueueBase<MetaDataType, MutexType>::TryDequeue(ezMessage*& out_pMessage, MetaDataType& out_metaData)
{
  ezLock<MutexType> lock(m_mutex);

  if (!m_Queue.IsEmpty())
  {
    Entry& entry = m_Queue.PeekFront();
    out_pMessage = entry.m_pMessage;
    out_metaData = entry.m_MetaData;

    m_Queue.PopFront();
    return true;
  }

  return false;
}

template <typename MetaDataType, typename MutexType>
bool ezMessageQueueBase<MetaDataType, MutexType>::TryPeek(ezMessage*& out_pMessage, MetaDataType& out_metaData)
{
  ezLock<MutexType> lock(m_mutex);

  if (!m_Queue.IsEmpty())
  {
    Entry& entry = m_Queue.PeekFront();
    out_pMessage = entry.m_pMessage;
    out_metaData = entry.m_MetaData;

    return true;
  }

  return false;
}

template <typename MetaDataType, typename MutexType>
template <typename C>
EZ_FORCE_INLINE void ezMessageQueueBase<MetaDataType, MutexType>::Sort()
{
  m_Queue.template Sort<C>();
}

template <typename MetaDataType, typename MutexType>
EZ_FORCE_INLINE ezAllocatorBase* ezMessageQueueBase<MetaDataType, MutexType>::GetStorageAllocator() const
{
  return m_pStorageAllocator;
}



template <typename MD, typename M, typename SA, typename QA>
ezMessageQueue<MD, M, SA, QA>::ezMessageQueue() : 
  ezMessageQueueBase<MD, M>(SA::GetAllocator(), QA::GetAllocator())
{
}

template <typename MD, typename M, typename SA, typename QA>
ezMessageQueue<MD, M, SA, QA>:: ezMessageQueue(ezAllocatorBase* pStorageAllocator, ezAllocatorBase* pQueueAllocator) : 
  ezMessageQueueBase<MD, M>(pStorageAllocator, pQueueAllocator)
{
}

template <typename MD, typename M, typename SA, typename QA>
ezMessageQueue<MD, M, SA, QA>::ezMessageQueue(const ezMessageQueue<MD, M, SA, QA>& rhs) : 
  ezMessageQueueBase<MD, M>(rhs, SA::GetAllocator(), QA::GetAllocator())
{
}

template <typename MD, typename M, typename SA, typename QA>
ezMessageQueue<MD, M, SA, QA>:: ezMessageQueue(const ezMessageQueueBase<MD, M>& rhs) : 
  ezMessageQueueBase<MD, M>(rhs, SA::GetAllocator(), QA::GetAllocator())
{
}

template <typename MD, typename M, typename SA, typename QA>
void ezMessageQueue<MD, M, SA, QA>::operator=(const ezMessageQueue<MD, M, SA, QA>& rhs)
{
  ezMessageQueueBase<MD, M>::operator=(rhs);
}

template <typename MD, typename M, typename SA, typename QA>
void ezMessageQueue<MD, M, SA, QA>::operator=(const ezMessageQueueBase<MD, M>& rhs)
{
  ezMessageQueueBase<MD, M>::operator=(rhs);
}

