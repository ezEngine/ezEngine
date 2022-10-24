#include <Foundation/FoundationPCH.h>

#include <Foundation/Strings/StringBuilder.h>
#include <Foundation/Strings/StringView.h>

ezUInt32 ezStringView::GetCharacter() const
{
  if (!IsValid())
    return 0;

  return ezUnicodeUtils::ConvertUtf8ToUtf32(m_pStart);
}

const char* ezStringView::GetData(ezStringBuilder& tempStorage) const
{
  tempStorage = *this;
  return tempStorage.GetData();
}

const char* ezStringView::GetData() const
{
  return m_pStart;
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

bool ezStringView::TrimWordStart(const char* szWord1, const char* szWord2 /*= nullptr*/, const char* szWord3 /*= nullptr*/,
  const char* szWord4 /*= nullptr*/, const char* szWord5 /*= nullptr*/)
{
  /// \test TrimWordStart
  bool trimmed = false;

  while (true)
  {
    if (!ezStringUtils::IsNullOrEmpty(szWord1) && StartsWith_NoCase(szWord1))
    {
      Shrink(ezStringUtils::GetCharacterCount(szWord1), 0);
      trimmed = true;
      continue;
    }

    if (!ezStringUtils::IsNullOrEmpty(szWord2) && StartsWith_NoCase(szWord2))
    {
      Shrink(ezStringUtils::GetCharacterCount(szWord2), 0);
      trimmed = true;
      continue;
    }

    if (!ezStringUtils::IsNullOrEmpty(szWord3) && StartsWith_NoCase(szWord3))
    {
      Shrink(ezStringUtils::GetCharacterCount(szWord3), 0);
      trimmed = true;
      continue;
    }

    if (!ezStringUtils::IsNullOrEmpty(szWord4) && StartsWith_NoCase(szWord4))
    {
      Shrink(ezStringUtils::GetCharacterCount(szWord4), 0);
      trimmed = true;
      continue;
    }

    if (!ezStringUtils::IsNullOrEmpty(szWord5) && StartsWith_NoCase(szWord5))
    {
      Shrink(ezStringUtils::GetCharacterCount(szWord5), 0);
      trimmed = true;
      continue;
    }

    return trimmed;
  }
}

bool ezStringView::TrimWordEnd(const char* szWord1, const char* szWord2 /*= nullptr*/, const char* szWord3 /*= nullptr*/,
  const char* szWord4 /*= nullptr*/, const char* szWord5 /*= nullptr*/)
{
  /// \test TrimWordEnd

  bool trimmed = false;

  while (true)
  {

    if (!ezStringUtils::IsNullOrEmpty(szWord1) && EndsWith_NoCase(szWord1))
    {
      Shrink(0, ezStringUtils::GetCharacterCount(szWord1));
      trimmed = true;
      continue;
    }

    if (!ezStringUtils::IsNullOrEmpty(szWord2) && EndsWith_NoCase(szWord2))
    {
      Shrink(0, ezStringUtils::GetCharacterCount(szWord2));
      trimmed = true;
      continue;
    }

    if (!ezStringUtils::IsNullOrEmpty(szWord3) && EndsWith_NoCase(szWord3))
    {
      Shrink(0, ezStringUtils::GetCharacterCount(szWord3));
      trimmed = true;
      continue;
    }

    if (!ezStringUtils::IsNullOrEmpty(szWord4) && EndsWith_NoCase(szWord4))
    {
      Shrink(0, ezStringUtils::GetCharacterCount(szWord4));
      trimmed = true;
      continue;
    }

    if (!ezStringUtils::IsNullOrEmpty(szWord5) && EndsWith_NoCase(szWord5))
    {
      Shrink(0, ezStringUtils::GetCharacterCount(szWord5));
      trimmed = true;
      continue;
    }

    return trimmed;
  }
}

bool ezStringView::HasAnyExtension() const
{
  return ezPathUtils::HasAnyExtension(*this);
}

bool ezStringView::HasExtension(const char* szExtension) const
{
  return ezPathUtils::HasExtension(*this, szExtension);
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
