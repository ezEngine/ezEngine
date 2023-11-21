
template <typename Type>
ezProcessingStreamIterator<Type>::ezProcessingStreamIterator(const ezProcessingStream* pStream, ezUInt64 uiNumElements, ezUInt64 uiStartIndex)

{
  EZ_ASSERT_DEV(pStream != nullptr, "Stream pointer may not be null!");
  EZ_ASSERT_DEV(pStream->GetElementSize() == sizeof(Type), "Data size missmatch");

  m_uiElementStride = pStream->GetElementStride();

  m_pCurrentPtr = ezMemoryUtils::AddByteOffset(pStream->GetWritableData(), static_cast<std::ptrdiff_t>(uiStartIndex * m_uiElementStride));
  m_pEndPtr = ezMemoryUtils::AddByteOffset(pStream->GetWritableData(), static_cast<std::ptrdiff_t>((uiStartIndex + uiNumElements) * m_uiElementStride));
}

template <typename Type>
EZ_ALWAYS_INLINE Type& ezProcessingStreamIterator<Type>::Current() const
{
  return *static_cast<Type*>(m_pCurrentPtr);
}

template <typename Type>
EZ_ALWAYS_INLINE bool ezProcessingStreamIterator<Type>::HasReachedEnd() const
{
  return m_pCurrentPtr >= m_pEndPtr;
}

template <typename Type>
EZ_ALWAYS_INLINE void ezProcessingStreamIterator<Type>::Advance()
{
  m_pCurrentPtr = ezMemoryUtils::AddByteOffset(m_pCurrentPtr, static_cast<std::ptrdiff_t>(m_uiElementStride));
}

template <typename Type>
EZ_ALWAYS_INLINE void ezProcessingStreamIterator<Type>::Advance(ezUInt32 uiNumElements)
{
  m_pCurrentPtr = ezMemoryUtils::AddByteOffset(m_pCurrentPtr, static_cast<std::ptrdiff_t>(m_uiElementStride * uiNumElements));
}
