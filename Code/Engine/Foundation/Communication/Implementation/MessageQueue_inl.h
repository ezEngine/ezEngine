
template <typename MetaDataType>
ezMessageQueueBase<MetaDataType>::ezMessageQueueBase(ezAllocatorBase* pAllocator) :
  m_Queue(pAllocator)
{
}

template <typename MetaDataType>
ezMessageQueueBase<MetaDataType>::ezMessageQueueBase(const ezMessageQueueBase& rhs, ezAllocatorBase* pAllocator) :
  m_Queue(pAllocator)
{
  m_Queue = rhs.m_Queue;
}

template <typename MetaDataType>
ezMessageQueueBase<MetaDataType>::~ezMessageQueueBase()
{
  Clear();
}

template <typename MetaDataType>
void ezMessageQueueBase<MetaDataType>::operator=(const ezMessageQueueBase& rhs)
{
  m_Queue = rhs.m_Queue;
}

template <typename MetaDataType>
EZ_FORCE_INLINE typename ezMessageQueueBase<MetaDataType>::Entry& ezMessageQueueBase<MetaDataType>::operator[](ezUInt32 uiIndex)
{
  return m_Queue[uiIndex];
}

template <typename MetaDataType>
EZ_FORCE_INLINE const typename ezMessageQueueBase<MetaDataType>::Entry& ezMessageQueueBase<MetaDataType>::operator[](ezUInt32 uiIndex) const
{
  return m_Queue[uiIndex];
}

template <typename MetaDataType>
EZ_FORCE_INLINE ezUInt32 ezMessageQueueBase<MetaDataType>::GetCount() const
{
  return m_Queue.GetCount();
}  

template <typename MetaDataType>
EZ_FORCE_INLINE bool ezMessageQueueBase<MetaDataType>::IsEmpty() const
{
  return m_Queue.IsEmpty();
}

template <typename MetaDataType>
void ezMessageQueueBase<MetaDataType>::Clear()
{
  m_Queue.Clear();
}

template <typename MetaDataType>
EZ_FORCE_INLINE void ezMessageQueueBase<MetaDataType>::Reserve(ezUInt32 uiCount)
{
  m_Queue.Reserve(uiCount);
}

template <typename MetaDataType>
EZ_FORCE_INLINE void ezMessageQueueBase<MetaDataType>::Compact()
{
  m_Queue.Compact();
}

template <typename MetaDataType>
void ezMessageQueueBase<MetaDataType>::Enqueue(ezMessage* pMessage, const MetaDataType& metaData)
{
  Entry entry;
  entry.m_pMessage = pMessage;
  entry.m_MetaData = metaData;

  {
    EZ_LOCK(m_Mutex);
    
    m_Queue.PushBack(entry);
  }
}

template <typename MetaDataType>
bool ezMessageQueueBase<MetaDataType>::TryDequeue(ezMessage*& out_pMessage, MetaDataType& out_metaData)
{
  EZ_LOCK(m_Mutex);

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

template <typename MetaDataType>
bool ezMessageQueueBase<MetaDataType>::TryPeek(ezMessage*& out_pMessage, MetaDataType& out_metaData)
{
  EZ_LOCK(m_Mutex);

  if (!m_Queue.IsEmpty())
  {
    Entry& entry = m_Queue.PeekFront();
    out_pMessage = entry.m_pMessage;
    out_metaData = entry.m_MetaData;

    return true;
  }

  return false;
}

template <typename MetaDataType>
EZ_FORCE_INLINE typename ezMessageQueueBase<MetaDataType>::Entry& ezMessageQueueBase<MetaDataType>::Peek()
{
  return m_Queue.PeekFront();
}

template <typename MetaDataType>
EZ_FORCE_INLINE void ezMessageQueueBase<MetaDataType>::Dequeue()
{
  m_Queue.PopFront();
}

template <typename MetaDataType>
template <typename Comparer>
EZ_FORCE_INLINE void ezMessageQueueBase<MetaDataType>::Sort(const Comparer& comparer)
{
  m_Queue.Sort(comparer);
}

template <typename MetaDataType>
void ezMessageQueueBase<MetaDataType>::Acquire()
{
  m_Mutex.Acquire();
}

template <typename MetaDataType>
void ezMessageQueueBase<MetaDataType>::Release()
{
  m_Mutex.Release();
}


template <typename MD, typename A>
ezMessageQueue<MD, A>::ezMessageQueue() : 
  ezMessageQueueBase<MD>(A::GetAllocator())
{
}

template <typename MD, typename A>
ezMessageQueue<MD, A>:: ezMessageQueue(ezAllocatorBase* pQueueAllocator) : 
  ezMessageQueueBase<MD>(pQueueAllocator)
{
}

template <typename MD, typename A>
ezMessageQueue<MD, A>::ezMessageQueue(const ezMessageQueue<MD, A>& rhs) : 
  ezMessageQueueBase<MD>(rhs, A::GetAllocator())
{
}

template <typename MD, typename A>
ezMessageQueue<MD, A>:: ezMessageQueue(const ezMessageQueueBase<MD>& rhs) : 
  ezMessageQueueBase<MD>(rhs, A::GetAllocator())
{
}

template <typename MD, typename A>
void ezMessageQueue<MD, A>::operator=(const ezMessageQueue<MD, A>& rhs)
{
  ezMessageQueueBase<MD>::operator=(rhs);
}

template <typename MD, typename A>
void ezMessageQueue<MD, A>::operator=(const ezMessageQueueBase<MD>& rhs)
{
  ezMessageQueueBase<MD>::operator=(rhs);
}

