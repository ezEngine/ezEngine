
#pragma once

template<ezUInt32 SIZE>
EZ_FORCE_INLINE ezStaticString<SIZE>::ezStaticString()
{
  Clear();
}

template<ezUInt32 SIZE>
template<ezUInt32 OTHER_SIZE>
EZ_FORCE_INLINE ezStaticString<SIZE>::ezStaticString(const ezStaticString<OTHER_SIZE>& rhs)
{
  *this = rhs;
}

template<ezUInt32 SIZE>
EZ_FORCE_INLINE ezStaticString<SIZE>::ezStaticString(const char* rhs)
{
  *this = rhs;
}

template<ezUInt32 SIZE>
template<ezUInt32 OTHER_SIZE>
EZ_FORCE_INLINE void ezStaticString<SIZE>::operator=(const ezStaticString<OTHER_SIZE>& rhs)
{
  *this = rhs.GetData();
}

template<ezUInt32 SIZE>
void ezStaticString<SIZE>::operator=(const char* rhs)
{
  m_uiElementCount = ezStringUtils::Copy(m_szData, SIZE, rhs);
}

template<ezUInt32 SIZE>
void ezStaticString<SIZE>::Clear() 
{ 
  m_szData[0] = '\0'; 
  m_uiElementCount = 0;
}

template<ezUInt32 SIZE>
EZ_FORCE_INLINE const char* ezStaticString<SIZE>::GetData() const
{ 
  return m_szData; 
}

template<ezUInt32 SIZE>
EZ_FORCE_INLINE ezUInt32 ezStaticString<SIZE>::GetElementCount() const 
{ 
  return m_uiElementCount; 
}