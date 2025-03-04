
template <typename T>
EZ_ALWAYS_INLINE const T& ezRenderDataBatch::Iterator<T>::operator*() const
{
  return *ezStaticCast<const T*>(m_pCurrent->m_pRenderData);
}

template <typename T>
EZ_ALWAYS_INLINE const T* ezRenderDataBatch::Iterator<T>::operator->() const
{
  return ezStaticCast<const T*>(m_pCurrent->m_pRenderData);
}

template <typename T>
EZ_ALWAYS_INLINE ezRenderDataBatch::Iterator<T>::operator const T*() const
{
  return ezStaticCast<const T*>(m_pCurrent->m_pRenderData);
}

template <typename T>
EZ_ALWAYS_INLINE void ezRenderDataBatch::Iterator<T>::Next()
{
  ++m_pCurrent;
}

template <typename T>
EZ_ALWAYS_INLINE bool ezRenderDataBatch::Iterator<T>::IsValid() const
{
  return m_pCurrent < m_pEnd;
}

template <typename T>
EZ_ALWAYS_INLINE void ezRenderDataBatch::Iterator<T>::operator++()
{
  Next();
}

template <typename T>
EZ_ALWAYS_INLINE ezRenderDataBatch::Iterator<T>::Iterator(const SortableRenderData* pStart, const SortableRenderData* pEnd)
{
  m_pCurrent = pStart;
  m_pEnd = pEnd;
}


EZ_ALWAYS_INLINE ezUInt32 ezRenderDataBatch::GetCount() const
{
  return m_Data.GetCount();
}

template <typename T>
EZ_ALWAYS_INLINE const T* ezRenderDataBatch::GetFirstData() const
{
  return m_Data.IsEmpty() == false ? ezStaticCast<const T*>(m_Data.GetPtr()->m_pRenderData) : nullptr;
}

template <typename T>
EZ_ALWAYS_INLINE ezRenderDataBatch::Iterator<T> ezRenderDataBatch::GetIterator(ezUInt32 uiStartIndex, ezUInt32 uiCount) const
{
  ezUInt32 uiEndIndex = ezMath::Min(uiStartIndex + uiCount, m_Data.GetCount());
  return Iterator<T>(m_Data.GetPtr() + uiStartIndex, m_Data.GetPtr() + uiEndIndex);
}

//////////////////////////////////////////////////////////////////////////

EZ_ALWAYS_INLINE ezUInt32 ezRenderDataBatchList::GetBatchCount() const
{
  return m_Batches.GetCount();
}

EZ_ALWAYS_INLINE const ezRenderDataBatch& ezRenderDataBatchList::GetBatch(ezUInt32 uiIndex) const
{
  return m_Batches[uiIndex];
}
