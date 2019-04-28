#include <FoundationPCH.h>

#include <Foundation/Strings/StringView.h>
#include <Foundation/Strings/StringBuilder.h>

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
    ezUnicodeUtils::MoveToNextUtf8(m_pStart, 1);
    --uiShrinkCharsFront;
  }

  while (IsValid() && (uiShrinkCharsBack > 0))
  {
    ezUnicodeUtils::MoveToPriorUtf8(m_pEnd, 1);
    --uiShrinkCharsBack;
  }
}
