
template<typename Type>
ezProcessingStreamIterator<Type>::ezProcessingStreamIterator(const ezProcessingStream* pStream, ezUInt64 uiNumElements, ezUInt64 uiStartIndex)
  : m_pCurrentPtr(nullptr)
  , m_pEndPtr(nullptr)
  , m_uiElementStride(0)
{
  EZ_ASSERT_DEV(pStream != nullptr, "Stream pointer may not be null!");

  m_uiElementStride = pStream->GetElementStride();

  m_pCurrentPtr = ezMemoryUtils::AddByteOffset(pStream->GetWritableData(), static_cast<ptrdiff_t>(uiStartIndex * m_uiElementStride));
  m_pEndPtr = ezMemoryUtils::AddByteOffset(pStream->GetWritableData(), static_cast<ptrdiff_t>((uiStartIndex + uiNumElements) * m_uiElementStride));
}

template<typename Type>
EZ_FORCE_INLINE Type& ezProcessingStreamIterator<Type>::Current() const
{
  return *static_cast<Type*>(m_pCurrentPtr);
}

template<typename Type>
EZ_FORCE_INLINE bool ezProcessingStreamIterator<Type>::HasReachedEnd() const
{
  return m_pCurrentPtr >= m_pEndPtr;
}

template<typename Type>
EZ_FORCE_INLINE void ezProcessingStreamIterator<Type>::Advance()
{
  m_pCurrentPtr = ezMemoryUtils::AddByteOffset(m_pCurrentPtr, static_cast<ptrdiff_t>(m_uiElementStride));
}
