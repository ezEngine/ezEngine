#pragma once

#include <Foundation/Strings/StringConversion.h>

inline ezStringBuilder::ezStringBuilder(ezIAllocator* pAllocator) : m_Data(pAllocator)
{
  m_uiCharacterCount = 0;
  AppendTerminator();
}

inline ezStringBuilder::ezStringBuilder(const ezStringBuilder& rhs) : m_Data(rhs.GetAllocator())
{
  m_uiCharacterCount = 0;
  AppendTerminator();

  *this = rhs;
}

inline ezStringBuilder::ezStringBuilder(const char* szUTF8, ezIAllocator* pAllocator) : m_Data(pAllocator)
{
  m_uiCharacterCount = 0;
  AppendTerminator();

  *this = szUTF8;
}

inline ezStringBuilder::ezStringBuilder(const wchar_t* szWChar, ezIAllocator* pAllocator) : m_Data(pAllocator)
{
  m_uiCharacterCount = 0;
  AppendTerminator();

  *this = szWChar;
}

inline ezStringBuilder::ezStringBuilder(const ezStringIterator& rhs, ezIAllocator* pAllocator) : m_Data(pAllocator)
{
  m_uiCharacterCount = 0;
  AppendTerminator();

  *this = rhs;
}

inline ezIAllocator* ezStringBuilder::GetAllocator() const
{
  return m_Data.GetAllocator();
}

inline void ezStringBuilder::operator=(const char* szUTF8)
{
  Clear();

  Append(szUTF8);
}

inline void ezStringBuilder::operator=(const wchar_t* szWChar)
{
  Clear();

  Append(szWChar);
}

inline void ezStringBuilder::operator=(const ezStringBuilder& rhs)
{
  m_Data = rhs.m_Data;
  m_uiCharacterCount = rhs.m_uiCharacterCount;
}

inline ezUInt32 ezStringBuilder::GetElementCount() const
{
  return m_Data.GetCount() - 1; // exclude the '\0' terminator
}

inline ezUInt32 ezStringBuilder::GetCharacterCount() const
{
  return m_uiCharacterCount;
}

inline void ezStringBuilder::Clear()
{
  m_uiCharacterCount = 0;
  m_Data.SetCount(1);
  m_Data[0] = '\0';
}

inline void ezStringBuilder::Append(ezUInt32 uiChar)
{
  char szChar[6] = { 0, 0, 0, 0, 0, 0 };
  char* pChar = &szChar[0];

  ezUnicodeUtils::EncodeUtf32ToUtf8(uiChar, pChar);
  Append(szChar);
}

inline void ezStringBuilder::Append(const wchar_t* pData1, const wchar_t* pData2, const wchar_t* pData3, const wchar_t* pData4, const wchar_t* pData5, const wchar_t* pData6)
{
  // this is a bit heavy on the stack size (6KB)
  // but it is really only a convenience function, as one could always just use the char* Append function and convert explicitely
  ezStringUtf8 s1(pData1, m_Data.GetAllocator());
  ezStringUtf8 s2(pData2, m_Data.GetAllocator());
  ezStringUtf8 s3(pData3, m_Data.GetAllocator());
  ezStringUtf8 s4(pData4, m_Data.GetAllocator());
  ezStringUtf8 s5(pData5, m_Data.GetAllocator());
  ezStringUtf8 s6(pData6, m_Data.GetAllocator());

  Append(s1.GetData(), s2.GetData(), s3.GetData(), s4.GetData(), s5.GetData(), s6.GetData());
}

inline void ezStringBuilder::Prepend(const wchar_t* pData1, const wchar_t* pData2, const wchar_t* pData3, const wchar_t* pData4, const wchar_t* pData5, const wchar_t* pData6)
{
  // this is a bit heavy on the stack size (6KB)
  // but it is really only a convenience function, as one could always just use the char* Append function and convert explicitely
  ezStringUtf8 s1(pData1, m_Data.GetAllocator());
  ezStringUtf8 s2(pData2, m_Data.GetAllocator());
  ezStringUtf8 s3(pData3, m_Data.GetAllocator());
  ezStringUtf8 s4(pData4, m_Data.GetAllocator());
  ezStringUtf8 s5(pData5, m_Data.GetAllocator());
  ezStringUtf8 s6(pData6, m_Data.GetAllocator());

  Prepend(s1.GetData(), s2.GetData(), s3.GetData(), s4.GetData(), s5.GetData(), s6.GetData());
}

inline const char* ezStringBuilder::GetData() const
{
  return &m_Data[0];
}

inline void ezStringBuilder::AppendTerminator()
{
  // make sure the string terminates with a zero.
  if (m_Data.IsEmpty() || (m_Data.Peek() != '\0'))
    m_Data.Append('\0');
}

inline void ezStringBuilder::ToUpper()
{
  const ezUInt32 uiNewStringLength = ezStringUtils::ToUpperString(&m_Data[0]);

  // the array stores the number of bytes, so set the count to the actually used number of bytes
  m_Data.SetCount(uiNewStringLength + 1);
}

inline void ezStringBuilder::ToLower()
{
  const ezUInt32 uiNewStringLength = ezStringUtils::ToLowerString(&m_Data[0]);

  // the array stores the number of bytes, so set the count to the actually used number of bytes
  m_Data.SetCount(uiNewStringLength + 1);
}

inline void ezStringBuilder::AppendFormat(const char* szUtf8Format, ...)
{
  va_list args;
  va_start (args, szUtf8Format);

  AppendFormat(szUtf8Format, args);

  va_end (args);
}

inline void ezStringBuilder::PrependFormat(const char* szUtf8Format, ...)
{
  va_list args;
  va_start (args, szUtf8Format);

  PrependFormat(szUtf8Format, args);

  va_end (args);
}

inline void ezStringBuilder::Format(const char* szUtf8Format, ...)
{
  va_list args;
  va_start (args, szUtf8Format);

  Format(szUtf8Format, args);

  va_end (args);
}

inline void ezStringBuilder::Format(const char* szUtf8Format, va_list& args)
{
  Clear();
  AppendFormat(szUtf8Format, args);
}

inline void ezStringBuilder::ChangeCharacter(ezStringIterator& It, ezUInt32 uiCharacter)
{
  EZ_ASSERT(It.IsValid(), "The given character iterator does not point to a valid character.");
  EZ_ASSERT(It.GetData() >= GetData() && It.GetData() < GetData() + GetElementCount(), "The given character iterator does not point into this string. It was either created from another string, or this string has been reallocated in the mean time.");

  // this is only an optimization for pure ASCII strings
  // without it, the code below would still work
  if (ezUnicodeUtils::IsASCII(*It.GetData()) && ezUnicodeUtils::IsASCII(uiCharacter))
  {
    char* pPos = const_cast<char*>(It.GetData()); // yes, I know...
    *pPos = uiCharacter & 0xFF;
    return;
  }

  ChangeCharacterNonASCII(It, uiCharacter);
}

inline ezStringIterator ezStringBuilder::GetIteratorFront() const
{
  return ezStringIterator(GetData(), GetData() + GetElementCount(), GetData(), GetCharacterCount() == GetElementCount());
}

inline ezStringIterator ezStringBuilder::GetIteratorBack() const
{
  ezStringIterator it (GetData(), GetData() + GetElementCount(), GetData() + GetElementCount(), GetCharacterCount() == GetElementCount());
  it.ResetToBack();
  return it;
}

EZ_FORCE_INLINE bool ezStringBuilder::IsPureASCII() const
{ 
  return m_uiCharacterCount + 1 == m_Data.GetCount();
}

inline void ezStringBuilder::Insert (const char* szInsertAtPos, const char* szTextToInsert, const char* szTextToInsertEnd)
{
  ReplaceSubString(szInsertAtPos, szInsertAtPos, szTextToInsert, szTextToInsertEnd);
}

inline void ezStringBuilder::Remove(const char* szRemoveFromPos, const char* szRemoveToPos)
{
  ReplaceSubString(szRemoveFromPos, szRemoveToPos, NULL);
}

template <typename Container>
void ezStringBuilder::Split(bool bReturnEmptyStrings, Container& Output, const char* szSeparator1, const char* szSeparator2, const char* szSeparator3, const char* szSeparator4, const char* szSeparator5, const char* szSeparator6) const
{
  Output.Clear();

  if (IsEmpty())
    return;

  const ezUInt32 uiParams = 6;

  const char* Seps[uiParams] =
  {
    szSeparator1,
    szSeparator2,
    szSeparator3,
    szSeparator4,
    szSeparator5,
    szSeparator6
  };

  ezUInt32 SepLen[uiParams];

  for (ezInt32 i = 0; i < uiParams; ++i)
    SepLen[i] = ezStringUtils::GetStringElementCount(Seps[i]);

  const char* szReadPos = GetData();

  while (true)
  {
    const char* szFoundPos = ezMaxStringEnd;
    ezInt32 iFoundSeparator = 0;

    for (ezInt32 i = 0; i < uiParams; ++i)
    {
      const char* szFound = ezStringUtils::FindSubString(szReadPos, Seps[i]);

      if ((szFound != NULL) && (szFound < szFoundPos))
      {
        szFoundPos = szFound;
        iFoundSeparator = i;
      }
    }

    // nothing found
    if (szFoundPos == ezMaxStringEnd)
    {
      const ezUInt32 uiLen = ezStringUtils::GetStringElementCount(szReadPos);

      if (bReturnEmptyStrings || (uiLen > 0))
        Output.Append(ezStringIterator(szReadPos, szReadPos + uiLen, szReadPos));

      return;
    }
    
    if (bReturnEmptyStrings || (szFoundPos > szReadPos))
      Output.Append(ezStringIterator(szReadPos, szFoundPos, szReadPos));

    szReadPos = szFoundPos + SepLen[iFoundSeparator];
  }
}

inline bool ezStringBuilder::HasAnyExtension() const
{
  return ezPathUtils::HasAnyExtension(GetData(), GetData() + GetElementCount());
}

inline bool ezStringBuilder::HasExtension(const char* szExtension) const
{
  return ezPathUtils::HasExtension(GetData(), szExtension, GetData() + GetElementCount());
}

inline ezStringIterator ezStringBuilder::GetFileExtension() const
{
  return ezPathUtils::GetFileExtension(GetData(), GetData() + GetElementCount());
}

inline ezStringIterator ezStringBuilder::GetFileName() const
{
  return ezPathUtils::GetFileName(GetData(), GetData() + GetElementCount());
}

inline ezStringIterator ezStringBuilder::GetFileNameAndExtension() const
{
  return ezPathUtils::GetFileNameAndExtension(GetData(), GetData() + GetElementCount());
}

inline ezStringIterator ezStringBuilder::GetFileDirectory() const
{
  return ezPathUtils::GetFileDirectory(GetData(), GetData() + GetElementCount());
}

inline bool ezStringBuilder::IsAbsolutePath() const
{
  return ezPathUtils::IsAbsolutePath(GetData());
}

inline bool ezStringBuilder::IsRelativePath() const
{
  return ezPathUtils::IsRelativePath(GetData());
}

