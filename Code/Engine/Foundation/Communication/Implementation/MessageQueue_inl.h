
template <typename MetaDataType, typename MutexType>
ezMessageQueueBase<MetaDataType, MutexType>::ezMessageQueueBase(ezAllocatorBase* pAllocator) :
  m_Queue(pAllocator)
{
}

template <typename MetaDataType, typename MutexType>
ezMessageQueueBase<MetaDataType, MutexType>::ezMessageQueueBase(const ezMessageQueueBase& rhs, ezAllocatorBase* pAllocator) :
  m_Queue(pAllocator)
{
  m_Queue = rhs.m_Queue;
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
void ezMessageQueueBase<MetaDataType, MutexType>::Enqueue(ezMessage* pMessage, const MetaDataType& metaData)
{
  Entry entry;
  entry.m_pMessage = pMessage;
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
EZ_FORCE_INLINE typename ezMessageQueueBase<MetaDataType, MutexType>::Entry& ezMessageQueueBase<MetaDataType, MutexType>::Peek()
{
  return m_Queue.PeekFront();
}

template <typename MetaDataType, typename MutexType>
EZ_FORCE_INLINE void ezMessageQueueBase<MetaDataType, MutexType>::Dequeue()
{
  m_Queue.PopFront();
}

template <typename MetaDataType, typename MutexType>
template <typename Comparer>
EZ_FORCE_INLINE void ezMessageQueueBase<MetaDataType, MutexType>::Sort(const Comparer& comparer)
{
  m_Queue.Sort(comparer);
}



template <typename MD, typename M, typename A>
ezMessageQueue<MD, M, A>::ezMessageQueue() : 
  ezMessageQueueBase<MD, M>(A::GetAllocator())
{
}

template <typename MD, typename M, typename A>
ezMessageQueue<MD, M, A>:: ezMessageQueue(ezAllocatorBase* pQueueAllocator) : 
  ezMessageQueueBase<MD, M>(pQueueAllocator)
{
}

template <typename MD, typename M, typename A>
ezMessageQueue<MD, M, A>::ezMessageQueue(const ezMessageQueue<MD, M, A>& rhs) : 
  ezMessageQueueBase<MD, M>(rhs, A::GetAllocator())
{
}

template <typename MD, typename M, typename A>
ezMessageQueue<MD, M, A>:: ezMessageQueue(const ezMessageQueueBase<MD, M>& rhs) : 
  ezMessageQueueBase<MD, M>(rhs, A::GetAllocator())
{
}

template <typename MD, typename M, typename A>
void ezMessageQueue<MD, M, A>::operator=(const ezMessageQueue<MD, M, A>& rhs)
{
  ezMessageQueueBase<MD, M>::operator=(rhs);
}

template <typename MD, typename M, typename A>
void ezMessageQueue<MD, M, A>::operator=(const ezMessageQueueBase<MD, M>& rhs)
{
  ezMessageQueueBase<MD, M>::operator=(rhs);
}

