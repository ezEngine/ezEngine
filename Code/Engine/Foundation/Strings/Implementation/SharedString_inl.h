#pragma once

inline ezSharedString::ezSharedString()
{
  m_pShared = &s_EmptyString;
}

inline ezSharedString::ezSharedString(const ezSharedString& rhs)
{
  m_pShared = &s_EmptyString;
  *this = rhs;
}

inline ezSharedString::ezSharedString(const char* rhs)
{
  m_pShared = &s_EmptyString;
  *this = rhs;
}

inline ezSharedString::ezSharedString(const wchar_t* rhs)
{
  m_pShared = &s_EmptyString;
  *this = rhs;
}

inline ezSharedString::ezSharedString(const ezStringIterator& rhs)
{
  m_pShared = &s_EmptyString;
  *this = rhs;
}

inline ezSharedString::~ezSharedString()
{
  Release();
}

inline void ezSharedString::Clear()
{
  Release();
  m_pShared = &s_EmptyString;
}

EZ_FORCE_INLINE const char* ezSharedString::GetData() const
{
  return m_pShared->m_szString;
}

EZ_FORCE_INLINE ezUInt32 ezSharedString::GetElementCount() const
{
  return m_pShared->m_uiElementCount;
}

EZ_FORCE_INLINE ezUInt32 ezSharedString::GetCharacterCount() const
{
  return m_pShared->m_uiCharacterCount;
}

inline bool ezSharedString::TryOverwrite(const char* szString, const char* szStringEnd)
{
  if (m_pShared && m_pShared->m_RefCount == 1)
  {
    // the shared string is used by this string exclusively, can just change it

    ezUInt32 uiOtherLength;
    ezUInt32 uiOtherCharacters;
    ezStringUtils::GetCharacterAndElementCount(szString, uiOtherCharacters, uiOtherLength, szStringEnd);

    if (uiOtherLength < m_pShared->m_uiCapacity) 
    {
      // 'Move' because the incoming string could be a substring from this string itself
      ezMemoryUtils::Move(m_pShared->m_szString, szString, uiOtherLength);
      m_pShared->m_szString[uiOtherLength] = '\0';
      m_pShared->m_uiElementCount = uiOtherLength;
      m_pShared->m_uiCharacterCount = uiOtherCharacters;
      return true;
    }

    // if the capacity is not sufficient, just reallocate
  }

  return false;
}

inline void ezSharedString::operator=(const char* szString)
{
  if (TryOverwrite(szString))
    return;
  
  // either the string is shared, or the capacity is not sufficient

  Clear();

  if (ezStringUtils::IsNullOrEmpty(szString))
    return;
    
  m_pShared = ezInternal::ezSharedStringBase::CreateSharedString(szString);
}
  
inline void ezSharedString::operator=(const ezSharedString& rhs)
{
  if (rhs.m_pShared == m_pShared)
    return;

  Release();

  m_pShared = rhs.m_pShared;

  Acquire();
}

inline void ezSharedString::operator=(const wchar_t* szString)
{
  ezStringUtf8 sConversion(szString);
  *this = sConversion.GetData();
}

inline void ezSharedString::operator=(const ezStringIterator& rhs)
{
  if (TryOverwrite(rhs.GetData(), rhs.GetEnd()))
    return;

  if (ezStringUtils::IsNullOrEmpty(rhs.GetData()) || rhs.GetElementCount() == 0)
  {
    Clear();
    return;
  }

  // we first make a copy here, and then delete the previous data, as the string iterator might be created from our very own data
  ezInternal::ezSharedStringBase* pNewString = ezInternal::ezSharedStringBase::CreateSharedString(rhs.GetData(), rhs.GetEnd());

  Clear();

  m_pShared = pNewString;
}

inline void ezSharedString::Acquire()
{
  if (m_pShared != &s_EmptyString)
    m_pShared->Acquire();
}

inline void ezSharedString::Release()
{
  if (m_pShared != &s_EmptyString)
    m_pShared->Release();
}

inline ezStringIterator ezSharedString::GetIteratorFront() const
{
  return ezStringIterator(m_pShared->m_szString, m_pShared->m_szString + m_pShared->m_uiElementCount, m_pShared->m_szString, m_pShared->m_uiCharacterCount == m_pShared->m_uiElementCount);
}

inline ezStringIterator ezSharedString::GetIteratorBack() const
{
  ezStringIterator it (m_pShared->m_szString, m_pShared->m_szString + m_pShared->m_uiElementCount, m_pShared->m_szString + m_pShared->m_uiElementCount, m_pShared->m_uiCharacterCount == m_pShared->m_uiElementCount);
  it.ResetToBack();
  return it;
}

inline ezStringIterator ezSharedString::GetSubString(ezUInt32 uiFirstCharacter, ezUInt32 uiNumCharacters) const
{
  EZ_ASSERT(uiFirstCharacter < m_pShared->m_uiCharacterCount, "The string only has %i characters, cannot start a sub-string at character %i.", m_pShared->m_uiCharacterCount, uiFirstCharacter);
  EZ_ASSERT(uiFirstCharacter + uiNumCharacters <= m_pShared->m_uiCharacterCount, "The string only has %i characters, cannot get a sub-string up to character %i.", m_pShared->m_uiCharacterCount, uiFirstCharacter + uiNumCharacters);

  const char* szStart = m_pShared->m_szString;
  ezUnicodeUtils::MoveToNextUtf8(szStart, uiFirstCharacter);

  const char* szEnd = szStart;
  ezUnicodeUtils::MoveToNextUtf8(szEnd, uiNumCharacters);

  // we can actually determine whether at least the substring is only ASCII, yeah!
  return ezStringIterator(szStart, szEnd, szStart, (ezUInt32) (szEnd - szStart) == uiNumCharacters);
}

inline ezStringIterator ezSharedString::GetFirst(ezUInt32 uiNumCharacters) const
{
  return GetSubString(0, uiNumCharacters);
}

inline ezStringIterator ezSharedString::GetLast(ezUInt32 uiNumCharacters) const
{
  EZ_ASSERT(uiNumCharacters < m_pShared->m_uiCharacterCount, "The string only contains %i characters, cannot return the last %i characters.", m_pShared->m_uiCharacterCount, uiNumCharacters);
  return GetSubString(m_pShared->m_uiCharacterCount - uiNumCharacters, uiNumCharacters);
}
