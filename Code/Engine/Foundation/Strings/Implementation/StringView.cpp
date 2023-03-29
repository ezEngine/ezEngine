#include <Foundation/FoundationPCH.h>

#include <Foundation/Strings/StringBuilder.h>
#include <Foundation/Strings/StringUtils.h>
#include <Foundation/Strings/StringView.h>

ezUInt32 ezStringView::GetCharacter() const
{
  if (!IsValid())
    return 0;

  return ezUnicodeUtils::ConvertUtf8ToUtf32(m_pStart);
}

const char* ezStringView::GetData(ezStringBuilder& ref_sTempStorage) const
{
  ref_sTempStorage = *this;
  return ref_sTempStorage.GetData();
}

bool ezStringView::IsEqualN(ezStringView sOther, ezUInt32 uiCharsToCompare) const
{
  return ezStringUtils::IsEqualN(GetStartPointer(), sOther.GetStartPointer(), uiCharsToCompare, GetEndPointer(), sOther.GetEndPointer());
}

bool ezStringView::IsEqualN_NoCase(ezStringView sOther, ezUInt32 uiCharsToCompare) const
{
  return ezStringUtils::IsEqualN_NoCase(GetStartPointer(), sOther.GetStartPointer(), uiCharsToCompare, GetEndPointer(), sOther.GetEndPointer());
}

ezInt32 ezStringView::Compare(ezStringView sOther) const
{
  return ezStringUtils::Compare(GetStartPointer(), sOther.GetStartPointer(), GetEndPointer(), sOther.GetEndPointer());
}

ezInt32 ezStringView::CompareN(ezStringView sOther, ezUInt32 uiCharsToCompare) const
{
  return ezStringUtils::CompareN(GetStartPointer(), sOther.GetStartPointer(), uiCharsToCompare, GetEndPointer(), sOther.GetEndPointer());
}

ezInt32 ezStringView::Compare_NoCase(ezStringView sOther) const
{
  return ezStringUtils::Compare_NoCase(GetStartPointer(), sOther.GetStartPointer(), GetEndPointer(), sOther.GetEndPointer());
}

ezInt32 ezStringView::CompareN_NoCase(ezStringView sOther, ezUInt32 uiCharsToCompare) const
{
  return ezStringUtils::CompareN_NoCase(GetStartPointer(), sOther.GetStartPointer(), uiCharsToCompare, GetEndPointer(), sOther.GetEndPointer());
}

const char* ezStringView::ComputeCharacterPosition(ezUInt32 uiCharacterIndex) const
{
  const char* pos = GetStartPointer();
  ezUnicodeUtils::MoveToNextUtf8(pos, GetEndPointer(), uiCharacterIndex);
  return pos;
}

const char* ezStringView::FindSubString(ezStringView sStringToFind, const char* szStartSearchAt /*= nullptr*/) const
{
  if (szStartSearchAt == nullptr)
    szStartSearchAt = GetStartPointer();

  EZ_ASSERT_DEV((szStartSearchAt >= GetStartPointer()) && (szStartSearchAt <= GetEndPointer()), "The given pointer to start searching at is not inside this strings valid range.");

  return ezStringUtils::FindSubString(szStartSearchAt, sStringToFind.GetStartPointer(), GetEndPointer(), sStringToFind.GetEndPointer());
}

const char* ezStringView::FindSubString_NoCase(ezStringView sStringToFind, const char* szStartSearchAt /*= nullptr*/) const
{
  if (szStartSearchAt == nullptr)
    szStartSearchAt = GetStartPointer();

  EZ_ASSERT_DEV((szStartSearchAt >= GetStartPointer()) && (szStartSearchAt <= GetEndPointer()), "The given pointer to start searching at is not inside this strings valid range.");

  return ezStringUtils::FindSubString_NoCase(szStartSearchAt, sStringToFind.GetStartPointer(), GetEndPointer(), sStringToFind.GetEndPointer());
}

const char* ezStringView::FindLastSubString(ezStringView sStringToFind, const char* szStartSearchAt /*= nullptr*/) const
{
  if (szStartSearchAt == nullptr)
    szStartSearchAt = GetEndPointer();

  EZ_ASSERT_DEV((szStartSearchAt >= GetStartPointer()) && (szStartSearchAt <= GetEndPointer()), "The given pointer to start searching at is not inside this strings valid range.");

  return ezStringUtils::FindLastSubString(GetStartPointer(), sStringToFind.GetStartPointer(), szStartSearchAt, GetEndPointer(), sStringToFind.GetEndPointer());
}

const char* ezStringView::FindLastSubString_NoCase(ezStringView sStringToFind, const char* szStartSearchAt /*= nullptr*/) const
{
  if (szStartSearchAt == nullptr)
    szStartSearchAt = GetEndPointer();

  EZ_ASSERT_DEV((szStartSearchAt >= GetStartPointer()) && (szStartSearchAt <= GetEndPointer()), "The given pointer to start searching at is not inside this strings valid range.");

  return ezStringUtils::FindLastSubString_NoCase(GetStartPointer(), sStringToFind.GetStartPointer(), szStartSearchAt, GetEndPointer(), sStringToFind.GetEndPointer());
}

const char* ezStringView::FindWholeWord(const char* szSearchFor, ezStringUtils::EZ_CHARACTER_FILTER isDelimiterCB, const char* szStartSearchAt /*= nullptr*/) const
{
  if (szStartSearchAt == nullptr)
    szStartSearchAt = GetStartPointer();

  EZ_ASSERT_DEV((szStartSearchAt >= GetStartPointer()) && (szStartSearchAt <= GetEndPointer()), "The given pointer to start searching at is not inside this strings valid range.");

  return ezStringUtils::FindWholeWord(szStartSearchAt, szSearchFor, isDelimiterCB, GetEndPointer());
}

const char* ezStringView::FindWholeWord_NoCase(const char* szSearchFor, ezStringUtils::EZ_CHARACTER_FILTER isDelimiterCB, const char* szStartSearchAt /*= nullptr*/) const
{
  if (szStartSearchAt == nullptr)
    szStartSearchAt = GetStartPointer();

  EZ_ASSERT_DEV((szStartSearchAt >= GetStartPointer()) && (szStartSearchAt <= GetEndPointer()), "The given pointer to start searching at is not inside this strings valid range.");

  return ezStringUtils::FindWholeWord_NoCase(szStartSearchAt, szSearchFor, isDelimiterCB, GetEndPointer());
}

void ezStringView::Shrink(ezUInt32 uiShrinkCharsFront, ezUInt32 uiShrinkCharsBack)
{
  while (IsValid() && (uiShrinkCharsFront > 0))
  {
    ezUnicodeUtils::MoveToNextUtf8(m_pStart, m_pEnd, 1);
    --uiShrinkCharsFront;
  }

  while (IsValid() && (uiShrinkCharsBack > 0))
  {
    ezUnicodeUtils::MoveToPriorUtf8(m_pEnd, 1);
    --uiShrinkCharsBack;
  }
}

ezStringView ezStringView::GetShrunk(ezUInt32 uiShrinkCharsFront, ezUInt32 uiShrinkCharsBack) const
{
  ezStringView tmp = *this;
  tmp.Shrink(uiShrinkCharsFront, uiShrinkCharsBack);
  return tmp;
}

void ezStringView::ChopAwayFirstCharacterUtf8()
{
  if (IsValid())
  {
    ezUnicodeUtils::MoveToNextUtf8(m_pStart, m_pEnd, 1);
  }
}

void ezStringView::ChopAwayFirstCharacterAscii()
{
  if (IsValid())
  {
    EZ_ASSERT_DEBUG(ezUnicodeUtils::IsASCII(*m_pStart), "ChopAwayFirstCharacterAscii() was called on a non-ASCII character.");

    m_pStart += 1;
  }
}

bool ezStringView::TrimWordStart(ezStringView sWord1, ezStringView sWord2, ezStringView sWord3, ezStringView sWord4, ezStringView sWord5)
{
  /// \test TrimWordStart
  bool trimmed = false;

  while (true)
  {
    if (!sWord1.IsEmpty() && StartsWith_NoCase(sWord1))
    {
      Shrink(ezStringUtils::GetCharacterCount(sWord1.GetStartPointer(), sWord1.GetEndPointer()), 0);
      trimmed = true;
      continue;
    }

    if (!sWord2.IsEmpty() && StartsWith_NoCase(sWord2))
    {
      Shrink(ezStringUtils::GetCharacterCount(sWord2.GetStartPointer(), sWord2.GetEndPointer()), 0);
      trimmed = true;
      continue;
    }

    if (!sWord3.IsEmpty() && StartsWith_NoCase(sWord3))
    {
      Shrink(ezStringUtils::GetCharacterCount(sWord3.GetStartPointer(), sWord3.GetEndPointer()), 0);
      trimmed = true;
      continue;
    }

    if (!sWord4.IsEmpty() && StartsWith_NoCase(sWord4))
    {
      Shrink(ezStringUtils::GetCharacterCount(sWord4.GetStartPointer(), sWord4.GetEndPointer()), 0);
      trimmed = true;
      continue;
    }

    if (!sWord5.IsEmpty() && StartsWith_NoCase(sWord5))
    {
      Shrink(ezStringUtils::GetCharacterCount(sWord5.GetStartPointer(), sWord5.GetEndPointer()), 0);
      trimmed = true;
      continue;
    }

    return trimmed;
  }
}

bool ezStringView::TrimWordEnd(ezStringView sWord1, ezStringView sWord2, ezStringView sWord3, ezStringView sWord4, ezStringView sWord5)
{
  /// \test TrimWordEnd

  bool trimmed = false;

  while (true)
  {
    if (!sWord1.IsEmpty() && EndsWith_NoCase(sWord1))
    {
      Shrink(0, ezStringUtils::GetCharacterCount(sWord1.GetStartPointer(), sWord1.GetEndPointer()));
      trimmed = true;
      continue;
    }

    if (!sWord2.IsEmpty() && EndsWith_NoCase(sWord2))
    {
      Shrink(0, ezStringUtils::GetCharacterCount(sWord2.GetStartPointer(), sWord2.GetEndPointer()));
      trimmed = true;
      continue;
    }

    if (!sWord3.IsEmpty() && EndsWith_NoCase(sWord3))
    {
      Shrink(0, ezStringUtils::GetCharacterCount(sWord3.GetStartPointer(), sWord3.GetEndPointer()));
      trimmed = true;
      continue;
    }

    if (!sWord4.IsEmpty() && EndsWith_NoCase(sWord4))
    {
      Shrink(0, ezStringUtils::GetCharacterCount(sWord4.GetStartPointer(), sWord4.GetEndPointer()));
      trimmed = true;
      continue;
    }

    if (!sWord5.IsEmpty() && EndsWith_NoCase(sWord5))
    {
      Shrink(0, ezStringUtils::GetCharacterCount(sWord5.GetStartPointer(), sWord5.GetEndPointer()));
      trimmed = true;
      continue;
    }

    return trimmed;
  }
}

ezStringView::iterator ezStringView::GetIteratorFront() const
{
  return begin(*this);
}

ezStringView::reverse_iterator ezStringView::GetIteratorBack() const
{
  return rbegin(*this);
}

bool ezStringView::HasAnyExtension() const
{
  return ezPathUtils::HasAnyExtension(*this);
}

bool ezStringView::HasExtension(ezStringView sExtension) const
{
  return ezPathUtils::HasExtension(*this, sExtension);
}

ezStringView ezStringView::GetFileExtension() const
{
  return ezPathUtils::GetFileExtension(*this);
}

ezStringView ezStringView::GetFileName() const
{
  return ezPathUtils::GetFileName(*this);
}

ezStringView ezStringView::GetFileNameAndExtension() const
{
  return ezPathUtils::GetFileNameAndExtension(*this);
}

ezStringView ezStringView::GetFileDirectory() const
{
  return ezPathUtils::GetFileDirectory(*this);
}

bool ezStringView::IsAbsolutePath() const
{
  return ezPathUtils::IsAbsolutePath(*this);
}

bool ezStringView::IsRelativePath() const
{
  return ezPathUtils::IsRelativePath(*this);
}

bool ezStringView::IsRootedPath() const
{
  return ezPathUtils::IsRootedPath(*this);
}

ezStringView ezStringView::GetRootedPathRootName() const
{
  return ezPathUtils::GetRootedPathRootName(*this);
}

EZ_STATICLINK_FILE(Foundation, Foundation_Strings_Implementation_StringView);
