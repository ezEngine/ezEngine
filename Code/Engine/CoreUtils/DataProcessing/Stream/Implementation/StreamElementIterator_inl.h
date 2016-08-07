
template<typename Type>
ezStreamElementIterator<Type>::ezStreamElementIterator( const ezStream* pStream, ezUInt64 uiNumElements, ezUInt64 uiStartIndex /*= 0*/ )
  : m_pCurrentPtr(nullptr)
  , m_pEndPtr(nullptr)
  , m_uiElementStride(0)
{
  EZ_ASSERT_DEV( pStream, "Stream pointer may not be null!" );

  m_uiElementStride = pStream->GetElementStride();

  m_pCurrentPtr = ezMemoryUtils::AddByteOffset( pStream->GetWritableData(), uiStartIndex * m_uiElementStride );
  m_pEndPtr = ezMemoryUtils::AddByteOffset( pStream->GetWritableData(), (uiStartIndex + uiNumElements) * m_uiElementStride );
}

template<typename Type>
Type& ezStreamElementIterator<Type>::Current() const
{
  return *static_cast<Type*>(m_pCurrentPtr);
}

template<typename Type>
bool ezStreamElementIterator<Type>::HasReachedEnd() const
{
  return m_pCurrentPtr >= m_pEndPtr;
}

template<typename Type>
void ezStreamElementIterator<Type>::Advance()
{
  m_pCurrentPtr = ezMemoryUtils::AddByteOffset( m_pCurrentPtr, static_cast<ptrdiff_t>(m_uiElementStride) );
}
