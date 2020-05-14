#pragma once

#include <Foundation/Strings/StringConversion.h>

inline ezStringBuilder::ezStringBuilder(ezAllocatorBase* pAllocator)
  : m_Data(pAllocator)
{
  m_uiCharacterCount = 0;
  AppendTerminator();
}

inline ezStringBuilder::ezStringBuilder(const ezStringBuilder& rhs)
  : m_Data(rhs.GetAllocator())
{
  m_uiCharacterCount = 0;
  AppendTerminator();

  *this = rhs;
}

inline ezStringBuilder::ezStringBuilder(ezStringBuilder&& rhs) noexcept
  : m_Data(rhs.GetAllocator())
{
  m_uiCharacterCount = 0;
  AppendTerminator();

  *this = std::move(rhs);
}

inline ezStringBuilder::ezStringBuilder(const char* szUTF8, ezAllocatorBase* pAllocator)
  : m_Data(pAllocator)
{
  m_uiCharacterCount = 0;
  AppendTerminator();

  *this = szUTF8;
}

inline ezStringBuilder::ezStringBuilder(const wchar_t* szWChar, ezAllocatorBase* pAllocator)
  : m_Data(pAllocator)
{
  m_uiCharacterCount = 0;
  AppendTerminator();

  *this = szWChar;
}

inline ezStringBuilder::ezStringBuilder(const ezStringView& rhs, ezAllocatorBase* pAllocator)
  : m_Data(pAllocator)
{
  m_uiCharacterCount = 0;
  AppendTerminator();

  *this = rhs;
}

EZ_ALWAYS_INLINE ezAllocatorBase* ezStringBuilder::GetAllocator() const
{
  return m_Data.GetAllocator();
}

EZ_ALWAYS_INLINE void ezStringBuilder::operator=(const char* szUTF8)
{
  Set(szUTF8);
}

EZ_FORCE_INLINE void ezStringBuilder::operator=(const wchar_t* szWChar)
{
  // fine to do this, szWChar can never come from the stringbuilder's own data array
  Clear();
  Append(szWChar);
}

EZ_ALWAYS_INLINE void ezStringBuilder::operator=(const ezStringBuilder& rhs)
{
  m_uiCharacterCount = rhs.m_uiCharacterCount;
  m_Data = rhs.m_Data;
}

EZ_ALWAYS_INLINE void ezStringBuilder::operator=(ezStringBuilder&& rhs) noexcept
{
  m_uiCharacterCount = rhs.m_uiCharacterCount;
  m_Data = std::move(rhs.m_Data);
}

EZ_ALWAYS_INLINE ezUInt32 ezStringBuilder::GetElementCount() const
{
  return m_Data.GetCount() - 1; // exclude the '\0' terminator
}

EZ_ALWAYS_INLINE ezUInt32 ezStringBuilder::GetCharacterCount() const
{
  return m_uiCharacterCount;
}

EZ_ALWAYS_INLINE ezStringBuilder::operator ezStringView() const
{
  return ezStringView(GetData(), GetData() + GetElementCount());
}


EZ_ALWAYS_INLINE ezStringView ezStringBuilder::GetView() const
{
  return ezStringView(GetData(), GetData() + GetElementCount());
}

EZ_FORCE_INLINE void ezStringBuilder::Clear()
{
  m_uiCharacterCount = 0;
  m_Data.SetCountUninitialized(1);
  m_Data[0] = '\0';
}

inline void ezStringBuilder::Append(ezUInt32 uiChar)
{
  char szChar[6] = {0, 0, 0, 0, 0, 0};
  char* pChar = &szChar[0];

  ezUnicodeUtils::EncodeUtf32ToUtf8(uiChar, pChar);
  ezUInt32 uiCharLen = (ezUInt32)(pChar - szChar);
  ezUInt32 uiOldCount = m_Data.GetCount();
  m_Data.SetCountUninitialized(uiOldCount + uiCharLen);
  uiOldCount--;
  for (ezUInt32 i = 0; i < uiCharLen; i++)
  {
    m_Data[uiOldCount + i] = szChar[i];
  }
  m_Data[uiOldCount + uiCharLen] = '\0';
  ++m_uiCharacterCount;
}

inline void ezStringBuilder::Prepend(ezUInt32 uiChar)
{
  char szChar[6] = {0, 0, 0, 0, 0, 0};
  char* pChar = &szChar[0];

  ezUnicodeUtils::EncodeUtf32ToUtf8(uiChar, pChar);
  Prepend(szChar);
}

inline void ezStringBuilder::Append(const wchar_t* pData1, const wchar_t* pData2, const wchar_t* pData3, const wchar_t* pData4,
  const wchar_t* pData5, const wchar_t* pData6)
{
  // this is a bit heavy on the stack size (6KB)
  // but it is really only a convenience function, as one could always just use the char* Append function and convert explicitly
  ezStringUtf8 s1(pData1, m_Data.GetAllocator());
  ezStringUtf8 s2(pData2, m_Data.GetAllocator());
  ezStringUtf8 s3(pData3, m_Data.GetAllocator());
  ezStringUtf8 s4(pData4, m_Data.GetAllocator());
  ezStringUtf8 s5(pData5, m_Data.GetAllocator());
  ezStringUtf8 s6(pData6, m_Data.GetAllocator());

  Append(s1.GetData(), s2.GetData(), s3.GetData(), s4.GetData(), s5.GetData(), s6.GetData());
}

inline void ezStringBuilder::Prepend(const wchar_t* pData1, const wchar_t* pData2, const wchar_t* pData3, const wchar_t* pData4,
  const wchar_t* pData5, const wchar_t* pData6)
{
  // this is a bit heavy on the stack size (6KB)
  // but it is really only a convenience function, as one could always just use the char* Append function and convert explicitly
  ezStringUtf8 s1(pData1, m_Data.GetAllocator());
  ezStringUtf8 s2(pData2, m_Data.GetAllocator());
  ezStringUtf8 s3(pData3, m_Data.GetAllocator());
  ezStringUtf8 s4(pData4, m_Data.GetAllocator());
  ezStringUtf8 s5(pData5, m_Data.GetAllocator());
  ezStringUtf8 s6(pData6, m_Data.GetAllocator());

  Prepend(s1.GetData(), s2.GetData(), s3.GetData(), s4.GetData(), s5.GetData(), s6.GetData());
}

EZ_ALWAYS_INLINE const char* ezStringBuilder::GetData() const
{
  EZ_ASSERT_DEBUG(!m_Data.IsEmpty(), "ezStringBuilder has been corrupted, the array can never be empty.");

  return &m_Data[0];
}

inline void ezStringBuilder::AppendTerminator()
{
  // make sure the string terminates with a zero.
  if (m_Data.IsEmpty() || (m_Data.PeekBack() != '\0'))
    m_Data.PushBack('\0');
}

inline void ezStringBuilder::ToUpper()
{
  const ezUInt32 uiNewStringLength = ezStringUtils::ToUpperString(&m_Data[0]);

  // the array stores the number of bytes, so set the count to the actually used number of bytes
  m_Data.SetCountUninitialized(uiNewStringLength + 1);
}

inline void ezStringBuilder::ToLower()
{
  const ezUInt32 uiNewStringLength = ezStringUtils::ToLowerString(&m_Data[0]);

  // the array stores the number of bytes, so set the count to the actually used number of bytes
  m_Data.SetCountUninitialized(uiNewStringLength + 1);
}

inline void ezStringBuilder::ChangeCharacter(iterator& it, ezUInt32 uiCharacter)
{
  EZ_ASSERT_DEV(it.IsValid(), "The given character iterator does not point to a valid character.");
  EZ_ASSERT_DEV(it.GetData() >= GetData() && it.GetData() < GetData() + GetElementCount(),
    "The given character iterator does not point into this string. It was either created from another string, or this string "
    "has been reallocated in the mean time.");

  // this is only an optimization for pure ASCII strings
  // without it, the code below would still work
  if (ezUnicodeUtils::IsASCII(*it) && ezUnicodeUtils::IsASCII(uiCharacter))
  {
    char* pPos = const_cast<char*>(it.GetData()); // yes, I know...
    *pPos = uiCharacter & 0xFF;
    return;
  }

  ChangeCharacterNonASCII(it, uiCharacter);
}

EZ_ALWAYS_INLINE bool ezStringBuilder::IsPureASCII() const
{
  return m_uiCharacterCount + 1 == m_Data.GetCount();
}

EZ_ALWAYS_INLINE void ezStringBuilder::Reserve(ezUInt32 uiNumElements)
{
  m_Data.Reserve(uiNumElements);
}

EZ_ALWAYS_INLINE void ezStringBuilder::Insert(const char* szInsertAtPos, const ezStringView& szTextToInsert)
{
  ReplaceSubString(szInsertAtPos, szInsertAtPos, szTextToInsert);
}

EZ_ALWAYS_INLINE void ezStringBuilder::Remove(const char* szRemoveFromPos, const char* szRemoveToPos)
{
  ReplaceSubString(szRemoveFromPos, szRemoveToPos, ezStringView());
}

template <typename Container>
void ezStringBuilder::Split(bool bReturnEmptyStrings, Container& Output, const char* szSeparator1, const char* szSeparator2,
  const char* szSeparator3, const char* szSeparator4, const char* szSeparator5, const char* szSeparator6) const
{
  Output.Clear();

  if (IsEmpty())
    return;

  const ezUInt32 uiParams = 6;

  const char* Seps[uiParams] = {szSeparator1, szSeparator2, szSeparator3, szSeparator4, szSeparator5, szSeparator6};

  ezUInt32 SepLen[uiParams];

  for (ezInt32 i = 0; i < uiParams; ++i)
    SepLen[i] = ezStringUtils::GetStringElementCount(Seps[i]);

  const char* szReadPos = GetData();

  while (true)
  {
    const char* szFoundPos = ezUnicodeUtils::GetMaxStringEnd<char>();
    ezInt32 iFoundSeparator = 0;

    for (ezInt32 i = 0; i < uiParams; ++i)
    {
      const char* szFound = ezStringUtils::FindSubString(szReadPos, Seps[i]);

      if ((szFound != nullptr) && (szFound < szFoundPos))
      {
        szFoundPos = szFound;
        iFoundSeparator = i;
      }
    }

    // nothing found
    if (szFoundPos == ezUnicodeUtils::GetMaxStringEnd<char>())
    {
      const ezUInt32 uiLen = ezStringUtils::GetStringElementCount(szReadPos);

      if (bReturnEmptyStrings || (uiLen > 0))
        Output.PushBack(ezStringView(szReadPos, szReadPos + uiLen));

      return;
    }

    if (bReturnEmptyStrings || (szFoundPos > szReadPos))
      Output.PushBack(ezStringView(szReadPos, szFoundPos));

    szReadPos = szFoundPos + SepLen[iFoundSeparator];
  }
}

EZ_FORCE_INLINE bool ezStringBuilder::HasAnyExtension() const
{
  return ezPathUtils::HasAnyExtension(GetData(), GetData() + GetElementCount());
}

EZ_FORCE_INLINE bool ezStringBuilder::HasExtension(const char* szExtension) const
{
  return ezPathUtils::HasExtension(GetData(), szExtension, GetData() + GetElementCount());
}

EZ_FORCE_INLINE ezStringView ezStringBuilder::GetFileExtension() const
{
  return ezPathUtils::GetFileExtension(GetData(), GetData() + GetElementCount());
}

EZ_FORCE_INLINE ezStringView ezStringBuilder::GetFileName() const
{
  return ezPathUtils::GetFileName(GetData(), GetData() + GetElementCount());
}

EZ_FORCE_INLINE ezStringView ezStringBuilder::GetFileNameAndExtension() const
{
  return ezPathUtils::GetFileNameAndExtension(GetData(), GetData() + GetElementCount());
}

EZ_FORCE_INLINE ezStringView ezStringBuilder::GetFileDirectory() const
{
  return ezPathUtils::GetFileDirectory(GetData(), GetData() + GetElementCount());
}

EZ_FORCE_INLINE bool ezStringBuilder::IsAbsolutePath() const
{
  return ezPathUtils::IsAbsolutePath(GetData());
}

EZ_FORCE_INLINE bool ezStringBuilder::IsRelativePath() const
{
  return ezPathUtils::IsRelativePath(GetData());
}

EZ_FORCE_INLINE bool ezStringBuilder::IsRootedPath() const
{
  return ezPathUtils::IsRootedPath(GetData());
}

EZ_FORCE_INLINE ezStringView ezStringBuilder::GetRootedPathRootName() const
{
  return ezPathUtils::GetRootedPathRootName(GetData());
}

#include <Foundation/Strings/Implementation/AllStrings_inl.h>
