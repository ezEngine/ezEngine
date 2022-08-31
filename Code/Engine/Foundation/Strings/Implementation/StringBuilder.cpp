#include <Foundation/FoundationPCH.h>

#include <Foundation/Containers/DynamicArray.h>
#include <Foundation/Strings/FormatString.h>
#include <Foundation/Strings/StringBuilder.h>

#include <stdarg.h>

ezStringBuilder::ezStringBuilder(ezStringView pData1, ezStringView pData2, ezStringView pData3, ezStringView pData4, ezStringView pData5, ezStringView pData6)
{
  m_uiCharacterCount = 0;
  AppendTerminator();

  Append(pData1, pData2, pData3, pData4, pData5, pData6);
}

void ezStringBuilder::Set(ezStringView pData1, ezStringView pData2, ezStringView pData3, ezStringView pData4, ezStringView pData5, ezStringView pData6)
{
  Clear();
  Append(pData1, pData2, pData3, pData4, pData5, pData6);
}

void ezStringBuilder::SetSubString_FromTo(const char* pStart, const char* pEnd)
{
  EZ_ASSERT_DEBUG(ezUnicodeUtils::IsValidUtf8(pStart, pEnd), "Invalid substring, the start does not point to a valid Utf-8 character");

  ezStringView view(pStart, pEnd);
  *this = view;
}

void ezStringBuilder::SetSubString_ElementCount(const char* pStart, ezUInt32 uiElementCount)
{
  EZ_ASSERT_DEBUG(
    ezUnicodeUtils::IsValidUtf8(pStart, pStart + uiElementCount), "Invalid substring, the start does not point to a valid Utf-8 character");

  ezStringView view(pStart, pStart + uiElementCount);
  *this = view;
}

void ezStringBuilder::SetSubString_CharacterCount(const char* pStart, ezUInt32 uiCharacterCount)
{
  const char* pEnd = pStart;
  ezUnicodeUtils::MoveToNextUtf8(pEnd, uiCharacterCount);

  ezStringView view(pStart, pEnd);
  *this = view;
}

void ezStringBuilder::Append(ezStringView pData1, ezStringView pData2, ezStringView pData3, ezStringView pData4, ezStringView pData5, ezStringView pData6)
{
  // it is not possible to find out how many parameters were passed to a vararg function
  // with a fixed size of parameters we do not need to have a parameter that tells us how many strings will come

  const ezUInt32 uiMaxParams = 6;

  const ezStringView pStrings[uiMaxParams] = {pData1, pData2, pData3, pData4, pData5, pData6};
  ezUInt32 uiStrLen[uiMaxParams] = {0};

  ezUInt32 uiMoreBytes = 0;

  // first figure out how much the string has to grow
  for (ezUInt32 i = 0; i < uiMaxParams; ++i)
  {
    if (pStrings[i].IsEmpty())
      continue;

    EZ_ASSERT_DEBUG(pStrings[i].GetStartPointer() < m_Data.GetData() || pStrings[i].GetStartPointer() >= m_Data.GetData() + m_Data.GetCapacity(),
      "Parameter {0} comes from the string builders own storage. This type assignment is not allowed.", i);

    ezUInt32 uiCharacters = 0;
    ezStringUtils::GetCharacterAndElementCount(pStrings[i].GetStartPointer(), uiCharacters, uiStrLen[i], pStrings[i].GetEndPointer());
    uiMoreBytes += uiStrLen[i];
    m_uiCharacterCount += uiCharacters;

    EZ_ASSERT_DEBUG(ezUnicodeUtils::IsValidUtf8(pStrings[i].GetStartPointer(), pStrings[i].GetEndPointer()), "Parameter {0} is not a valid Utf8 sequence.", i + 1);
  }

  ezUInt32 uiPrevCount = m_Data.GetCount(); // already contains a 0 terminator
  EZ_ASSERT_DEBUG(uiPrevCount > 0, "There should be a 0 terminator somewhere around here.");

  // now resize
  m_Data.SetCountUninitialized(uiPrevCount + uiMoreBytes);

  // and then append all the strings
  for (ezUInt32 i = 0; i < uiMaxParams; ++i)
  {
    if (uiStrLen[i] == 0)
      continue;

    // make enough room to copy the entire string, including the T-800
    ezStringUtils::Copy(&m_Data[uiPrevCount - 1], uiStrLen[i] + 1, pStrings[i].GetStartPointer(), pStrings[i].GetStartPointer() + uiStrLen[i]);

    uiPrevCount += uiStrLen[i];
  }
}

void ezStringBuilder::Prepend(ezStringView pData1, ezStringView pData2, ezStringView pData3, ezStringView pData4, ezStringView pData5, ezStringView pData6)
{
  // it is not possible to find out how many parameters were passed to a vararg function
  // with a fixed size of parameters we do not need to have a parameter that tells us how many strings will come

  const ezUInt32 uiMaxParams = 6;

  const ezStringView pStrings[uiMaxParams] = {pData1, pData2, pData3, pData4, pData5, pData6};
  ezUInt32 uiStrLen[uiMaxParams] = {0};

  ezUInt32 uiMoreBytes = 0;

  // first figure out how much the string has to grow
  for (ezUInt32 i = 0; i < uiMaxParams; ++i)
  {
    if (pStrings[i].IsEmpty())
      continue;

    ezUInt32 uiCharacters = 0;
    ezStringUtils::GetCharacterAndElementCount(pStrings[i].GetStartPointer(), uiCharacters, uiStrLen[i], pStrings[i].GetEndPointer());
    uiMoreBytes += uiStrLen[i];
    m_uiCharacterCount += uiCharacters;

    EZ_ASSERT_DEBUG(ezUnicodeUtils::IsValidUtf8(pStrings[i].GetStartPointer(), pStrings[i].GetEndPointer()), "Parameter {0} is not a valid Utf8 sequence.", i + 1);
  }

  ezUInt32 uiPrevCount = m_Data.GetCount(); // already contains a 0 terminator
  EZ_ASSERT_DEBUG(uiPrevCount > 0, "There should be a 0 terminator somewhere around here.");

  // now resize
  m_Data.SetCountUninitialized(uiPrevCount + uiMoreBytes);

  // move the previous string data at the end
  ezMemoryUtils::CopyOverlapped(&m_Data[0] + uiMoreBytes, GetData(), uiPrevCount);

  ezUInt32 uiWritePos = 0;

  // and then prepend all the strings
  for (ezUInt32 i = 0; i < uiMaxParams; ++i)
  {
    if (uiStrLen[i] == 0)
      continue;

    // make enough room to copy the entire string, including the T-800
    ezMemoryUtils::Copy(&m_Data[uiWritePos], pStrings[i].GetStartPointer(), uiStrLen[i]);

    uiWritePos += uiStrLen[i];
  }
}

void ezStringBuilder::PrintfArgs(const char* szUtf8Format, va_list args0)
{
  va_list args;
  va_copy(args, args0);

  Clear();

  const ezUInt32 TempBuffer = 4096;

  char szTemp[TempBuffer];
  const ezInt32 iCount = ezStringUtils::vsnprintf(szTemp, TempBuffer - 1, szUtf8Format, args);

  EZ_ASSERT_DEV(iCount != -1, "There was an error while formatting the string. Probably and unescaped usage of the %% sign.");

  if (iCount == -1)
  {
    va_end(args);
    return;
  }

  if (iCount > TempBuffer - 1)
  {
    ezDynamicArray<char> Temp;
    Temp.SetCountUninitialized(iCount + 1);

    ezStringUtils::vsnprintf(&Temp[0], iCount + 1, szUtf8Format, args);

    Append(&Temp[0]);
  }
  else
  {
    Append(&szTemp[0]);
  }

  va_end(args);
}

void ezStringBuilder::ChangeCharacterNonASCII(iterator& it, ezUInt32 uiCharacter)
{
  char* pPos = const_cast<char*>(it.GetData()); // yes, I know...

  const ezUInt32 uiOldCharLength = ezUnicodeUtils::GetUtf8SequenceLength(*pPos);
  const ezUInt32 uiNewCharLength = ezUnicodeUtils::GetSizeForCharacterInUtf8(uiCharacter);

  // if the old character and the new one are encoded with the same length, we can replace the character in-place
  if (uiNewCharLength == uiOldCharLength)
  {
    // just overwrite all characters at the given position with the new Utf8 string
    ezUnicodeUtils::EncodeUtf32ToUtf8(uiCharacter, pPos);

    // if the encoding length is identical, this will also handle all ASCII strings
    // if the string was pure ASCII before, this won't change, so no need to update that state
    return;
  }

  // in this case we can still update the string without reallocation, but the tail of the string has to be moved forwards
  if (uiNewCharLength < uiOldCharLength)
  {
    // just overwrite all characters at the given position with the new Utf8 string
    ezUnicodeUtils::EncodeUtf32ToUtf8(uiCharacter, pPos);

    // pPos will be changed (moved forwards) to the next character position

    // how much has changed
    const ezUInt32 uiDifference = uiOldCharLength - uiNewCharLength;
    const ezUInt32 uiTrailStringBytes = (ezUInt32)(GetData() + GetElementCount() - it.GetData() - uiOldCharLength + 1); // ???

    // move the trailing characters forwards
    ezMemoryUtils::CopyOverlapped(pPos, pPos + uiDifference, uiTrailStringBytes);

    // update the data array
    m_Data.PopBack(uiDifference);

    // 'It' references this already, no need to change anything.
  }
  else
  {
    // in this case we insert a character that is longer int Utf8 encoding than the character that already exists there *sigh*
    // so we must first move the trailing string backwards to make room, then we can write the new char in there

    // how much has changed
    const ezUInt32 uiDifference = uiNewCharLength - uiOldCharLength;
    const ezUInt32 uiTrailStringBytes = (ezUInt32)(GetData() + GetElementCount() - it.GetData() - uiOldCharLength + 1);
    auto iCurrentPos = (it.GetData() - GetData());
    // resize the array
    m_Data.SetCountUninitialized(m_Data.GetCount() + uiDifference);

    // these might have changed (array realloc)
    pPos = &m_Data[0] + iCurrentPos;
    it.SetCurrentPosition(pPos);

    // move the trailing string backwards
    ezMemoryUtils::CopyOverlapped(pPos + uiNewCharLength, pPos + uiOldCharLength, uiTrailStringBytes);

    // just overwrite all characters at the given position with the new Utf8 string
    ezUnicodeUtils::EncodeUtf32ToUtf8(uiCharacter, pPos);
  }
}

void ezStringBuilder::Shrink(ezUInt32 uiShrinkCharsFront, ezUInt32 uiShrinkCharsBack)
{
  if (uiShrinkCharsFront + uiShrinkCharsBack >= m_uiCharacterCount)
  {
    Clear();
    return;
  }

  const char* szNewStart = &m_Data[0];

  if (IsPureASCII())
  {
    if (uiShrinkCharsBack > 0)
    {
      m_Data.PopBack(uiShrinkCharsBack + 1);
      AppendTerminator();
    }

    szNewStart = &m_Data[uiShrinkCharsFront];
  }
  else
  {
    if (uiShrinkCharsBack > 0)
    {
      const char* szEnd = GetData() + GetElementCount();
      const char* szNewEnd = szEnd;
      ezUnicodeUtils::MoveToPriorUtf8(szNewEnd, uiShrinkCharsBack);

      const ezUInt32 uiLessBytes = (ezUInt32)(szEnd - szNewEnd);

      m_Data.PopBack(uiLessBytes + 1);
      AppendTerminator();
    }

    ezUnicodeUtils::MoveToNextUtf8(szNewStart, uiShrinkCharsFront);
  }

  if (szNewStart > &m_Data[0])
  {
    const ezUInt32 uiLessBytes = (ezUInt32)(szNewStart - &m_Data[0]);

    ezMemoryUtils::CopyOverlapped(&m_Data[0], szNewStart, m_Data.GetCount() - uiLessBytes);
    m_Data.PopBack(uiLessBytes);
  }

  m_uiCharacterCount -= uiShrinkCharsFront;
  m_uiCharacterCount -= uiShrinkCharsBack;
}

void ezStringBuilder::ReplaceSubString(const char* szStartPos, const char* szEndPos, ezStringView szReplaceWith)
{
  EZ_ASSERT_DEV(ezMath::IsInRange(szStartPos, GetData(), GetData() + m_Data.GetCount()), "szStartPos is not inside this string.");
  EZ_ASSERT_DEV(ezMath::IsInRange(szEndPos, GetData(), GetData() + m_Data.GetCount()), "szEndPos is not inside this string.");
  EZ_ASSERT_DEV(szStartPos <= szEndPos, "ezStartPos must be before ezEndPos");

  ezUInt32 uiWordChars = 0;
  ezUInt32 uiWordBytes = 0;
  ezStringUtils::GetCharacterAndElementCount(szReplaceWith.GetStartPointer(), uiWordChars, uiWordBytes, szReplaceWith.GetEndPointer());

  const ezUInt32 uiSubStringBytes = (ezUInt32)(szEndPos - szStartPos);

  char* szWritePos = const_cast<char*>(szStartPos); // szStartPos points into our own data anyway
  const char* szReadPos = szReplaceWith.GetStartPointer();

  // most simple case, just replace characters
  if (uiSubStringBytes == uiWordBytes)
  {
    while (szWritePos < szEndPos)
    {
      if (!ezUnicodeUtils::IsUtf8ContinuationByte(*szWritePos))
        --m_uiCharacterCount;

      *szWritePos = *szReadPos;
      ++szWritePos;
      ++szReadPos;
    }

    // the number of bytes might be identical, but that does not mean that the number of characters is also identical
    // therefore we subtract the number of characters that were found in the old substring
    // and add the number of characters for the new substring
    m_uiCharacterCount += uiWordChars;
    return;
  }

  // the replacement is shorter than the existing stuff -> move characters to the left, no reallocation needed
  if (uiWordBytes < uiSubStringBytes)
  {
    m_uiCharacterCount -= ezStringUtils::GetCharacterCount(szStartPos, szEndPos);
    m_uiCharacterCount += uiWordChars;

    // first copy the replacement to the correct position
    ezMemoryUtils::Copy(szWritePos, szReplaceWith.GetStartPointer(), uiWordBytes);

    const ezUInt32 uiDifference = uiSubStringBytes - uiWordBytes;

    const char* szStringEnd = GetData() + m_Data.GetCount();

    // now move all the characters from behind the replaced string to the correct position
    ezMemoryUtils::CopyOverlapped(szWritePos + uiWordBytes, szWritePos + uiSubStringBytes, szStringEnd - (szWritePos + uiSubStringBytes));

    m_Data.PopBack(uiDifference);

    return;
  }

  // else the replacement is longer than the existing word
  {
    m_uiCharacterCount -= ezStringUtils::GetCharacterCount(szStartPos, szEndPos);
    m_uiCharacterCount += uiWordChars;

    const ezUInt32 uiDifference = uiWordBytes - uiSubStringBytes;
    const ezUInt64 uiRelativeWritePosition = szWritePos - GetData();
    const ezUInt64 uiDataByteCountBefore = m_Data.GetCount();

    m_Data.SetCountUninitialized(m_Data.GetCount() + uiDifference);

    // all pointer are now possibly invalid since the data may be reallocated!
    szWritePos = const_cast<char*>(GetData()) + uiRelativeWritePosition;
    const char* szStringEnd = GetData() + uiDataByteCountBefore;

    // first move the characters to the proper position from back to front
    ezMemoryUtils::CopyOverlapped(szWritePos + uiWordBytes, szWritePos + uiSubStringBytes, szStringEnd - (szWritePos + uiSubStringBytes));

    // now copy the replacement to the correct position
    ezMemoryUtils::Copy(szWritePos, szReplaceWith.GetStartPointer(), uiWordBytes);
  }
}

const char* ezStringBuilder::ReplaceFirst(ezStringView sSearchFor, ezStringView sReplacement, const char* szStartSearchAt)
{
  if (szStartSearchAt == nullptr)
    szStartSearchAt = GetData();
  else
  {
    EZ_ASSERT_DEV(ezMath::IsInRange(szStartSearchAt, GetData(), GetData() + m_Data.GetCount() - 1), "szStartSearchAt is not inside the string range.");
  }

  const char* szFoundAt = ezStringUtils::FindSubString(szStartSearchAt, sSearchFor.GetStartPointer(), GetData() + m_Data.GetCount() - 1, sSearchFor.GetEndPointer());

  if (szFoundAt == nullptr)
    return nullptr;

  const ezUInt32 uiOffset = (ezUInt32)(szFoundAt - GetData());

  const ezUInt32 uiSearchStrLength = sSearchFor.GetElementCount();

  ReplaceSubString(szFoundAt, szFoundAt + uiSearchStrLength, sReplacement);

  return GetData() + uiOffset; // memory might have been reallocated
}

const char* ezStringBuilder::ReplaceLast(ezStringView sSearchFor, ezStringView sReplacement, const char* szStartSearchAt)
{
  if (szStartSearchAt == nullptr)
    szStartSearchAt = GetData() + m_Data.GetCount() - 1;
  else
  {
    EZ_ASSERT_DEV(ezMath::IsInRange(szStartSearchAt, GetData(), GetData() + m_Data.GetCount() - 1), "szStartSearchAt is not inside the string range.");
  }

  const char* szFoundAt = ezStringUtils::FindLastSubString(GetData(), sSearchFor.GetStartPointer(), szStartSearchAt, GetData() + m_Data.GetCount() - 1, sSearchFor.GetEndPointer());

  if (szFoundAt == nullptr)
    return nullptr;

  const ezUInt32 uiOffset = (ezUInt32)(szFoundAt - GetData());

  const ezUInt32 uiSearchStrLength = sSearchFor.GetElementCount();

  ReplaceSubString(szFoundAt, szFoundAt + uiSearchStrLength, sReplacement);

  return GetData() + uiOffset; // memory might have been reallocated
}

ezUInt32 ezStringBuilder::ReplaceAll(ezStringView sSearchFor, ezStringView sReplacement)
{
  const ezUInt32 uiSearchBytes = sSearchFor.GetElementCount();
  const ezUInt32 uiWordBytes = sReplacement.GetElementCount();

  ezUInt32 uiReplacements = 0;
  ezUInt32 uiOffset = 0;

  while (true)
  {
    // during ReplaceSubString the string data might get reallocated and the memory addresses do not stay valid
    // so we need to work with offsets and recompute the pointers every time
    const char* szFoundAt = ezStringUtils::FindSubString(GetData() + uiOffset, sSearchFor.GetStartPointer(), GetData() + m_Data.GetCount() - 1, sSearchFor.GetEndPointer());

    if (szFoundAt == nullptr)
      return uiReplacements;

    // do not search withing the replaced part, otherwise we get recursive replacement which will not end
    uiOffset = static_cast<ezUInt32>(szFoundAt - GetData()) + uiWordBytes;

    ReplaceSubString(szFoundAt, szFoundAt + uiSearchBytes, sReplacement);

    ++uiReplacements;
  }

  return uiReplacements;
}


const char* ezStringBuilder::ReplaceFirst_NoCase(ezStringView sSearchFor, ezStringView sReplacement, const char* szStartSearchAt)
{
  if (szStartSearchAt == nullptr)
    szStartSearchAt = GetData();
  else
  {
    EZ_ASSERT_DEV(ezMath::IsInRange(szStartSearchAt, GetData(), GetData() + m_Data.GetCount() - 1), "szStartSearchAt is not inside the string range.");
  }

  const char* szFoundAt = ezStringUtils::FindSubString_NoCase(szStartSearchAt, sSearchFor.GetStartPointer(), GetData() + m_Data.GetCount() - 1, sSearchFor.GetEndPointer());

  if (szFoundAt == nullptr)
    return nullptr;

  const ezUInt32 uiOffset = (ezUInt32)(szFoundAt - GetData());

  const ezUInt32 uiSearchStrLength = sSearchFor.GetElementCount();

  ReplaceSubString(szFoundAt, szFoundAt + uiSearchStrLength, sReplacement);

  return GetData() + uiOffset; // memory might have been reallocated
}

const char* ezStringBuilder::ReplaceLast_NoCase(ezStringView sSearchFor, ezStringView sReplacement, const char* szStartSearchAt)
{
  if (szStartSearchAt == nullptr)
    szStartSearchAt = GetData() + m_Data.GetCount() - 1;
  else
  {
    EZ_ASSERT_DEV(ezMath::IsInRange(szStartSearchAt, GetData(), GetData() + m_Data.GetCount() - 1), "szStartSearchAt is not inside the string range.");
  }

  const char* szFoundAt = ezStringUtils::FindLastSubString_NoCase(GetData(), sSearchFor.GetStartPointer(), szStartSearchAt, GetData() + m_Data.GetCount() - 1, sSearchFor.GetEndPointer());

  if (szFoundAt == nullptr)
    return nullptr;

  const ezUInt32 uiOffset = (ezUInt32)(szFoundAt - GetData());

  const ezUInt32 uiSearchStrLength = sSearchFor.GetElementCount();

  ReplaceSubString(szFoundAt, szFoundAt + uiSearchStrLength, sReplacement);

  return GetData() + uiOffset; // memory might have been reallocated
}

ezUInt32 ezStringBuilder::ReplaceAll_NoCase(ezStringView sSearchFor, ezStringView sReplacement)
{
  const ezUInt32 uiSearchBytes = sSearchFor.GetElementCount();
  const ezUInt32 uiWordBytes = sReplacement.GetElementCount();

  ezUInt32 uiReplacements = 0;
  ezUInt32 uiOffset = 0;

  while (true)
  {
    // during ReplaceSubString the string data might get reallocated and the memory addresses do not stay valid
    // so we need to work with offsets and recompute the pointers every time
    const char* szFoundAt = ezStringUtils::FindSubString_NoCase(GetData() + uiOffset, sSearchFor.GetStartPointer(), GetData() + m_Data.GetCount() - 1, sSearchFor.GetEndPointer());

    if (szFoundAt == nullptr)
      return uiReplacements;

    // do not search withing the replaced part, otherwise we get recursive replacement which will not end
    uiOffset = static_cast<ezUInt32>(szFoundAt - GetData()) + uiWordBytes;

    ReplaceSubString(szFoundAt, szFoundAt + uiSearchBytes, sReplacement);

    ++uiReplacements;
  }

  return uiReplacements;
}

const char* ezStringBuilder::ReplaceWholeWord(const char* szSearchFor, ezStringView szReplaceWith, ezStringUtils::EZ_CHARACTER_FILTER IsDelimiterCB)
{
  const char* szPos = FindWholeWord(szSearchFor, IsDelimiterCB);

  if (szPos == nullptr)
    return nullptr;

  const ezUInt32 uiOffset = static_cast<ezUInt32>(szPos - GetData());

  ReplaceSubString(szPos, szPos + ezStringUtils::GetStringElementCount(szSearchFor), szReplaceWith);
  return GetData() + uiOffset;
}

const char* ezStringBuilder::ReplaceWholeWord_NoCase(const char* szSearchFor, ezStringView szReplaceWith, ezStringUtils::EZ_CHARACTER_FILTER IsDelimiterCB)
{
  const char* szPos = FindWholeWord_NoCase(szSearchFor, IsDelimiterCB);

  if (szPos == nullptr)
    return nullptr;

  const ezUInt32 uiOffset = static_cast<ezUInt32>(szPos - GetData());

  ReplaceSubString(szPos, szPos + ezStringUtils::GetStringElementCount(szSearchFor), szReplaceWith);
  return GetData() + uiOffset;
}


ezUInt32 ezStringBuilder::ReplaceWholeWordAll(const char* szSearchFor, ezStringView szReplaceWith, ezStringUtils::EZ_CHARACTER_FILTER IsDelimiterCB)
{
  const ezUInt32 uiSearchBytes = ezStringUtils::GetStringElementCount(szSearchFor);
  const ezUInt32 uiWordBytes = ezStringUtils::GetStringElementCount(szReplaceWith.GetStartPointer(), szReplaceWith.GetEndPointer());

  ezUInt32 uiReplacements = 0;
  ezUInt32 uiOffset = 0;

  while (true)
  {
    // during ReplaceSubString the string data might get reallocated and the memory addresses do not stay valid
    // so we need to work with offsets and recompute the pointers every time
    const char* szFoundAt = ezStringUtils::FindWholeWord(GetData() + uiOffset, szSearchFor, IsDelimiterCB, GetData() + m_Data.GetCount() - 1);

    if (szFoundAt == nullptr)
      return uiReplacements;

    // do not search withing the replaced part, otherwise we get recursive replacement which will not end
    uiOffset = static_cast<ezUInt32>(szFoundAt - GetData()) + uiWordBytes;

    ReplaceSubString(szFoundAt, szFoundAt + uiSearchBytes, szReplaceWith);

    ++uiReplacements;
  }

  return uiReplacements;
}

ezUInt32 ezStringBuilder::ReplaceWholeWordAll_NoCase(const char* szSearchFor, ezStringView szReplaceWith, ezStringUtils::EZ_CHARACTER_FILTER IsDelimiterCB)
{
  const ezUInt32 uiSearchBytes = ezStringUtils::GetStringElementCount(szSearchFor);
  const ezUInt32 uiWordBytes = ezStringUtils::GetStringElementCount(szReplaceWith.GetStartPointer(), szReplaceWith.GetEndPointer());

  ezUInt32 uiReplacements = 0;
  ezUInt32 uiOffset = 0;

  while (true)
  {
    // during ReplaceSubString the string data might get reallocated and the memory addresses do not stay valid
    // so we need to work with offsets and recompute the pointers every time
    const char* szFoundAt = ezStringUtils::FindWholeWord_NoCase(GetData() + uiOffset, szSearchFor, IsDelimiterCB, GetData() + m_Data.GetCount() - 1);

    if (szFoundAt == nullptr)
      return uiReplacements;

    // do not search withing the replaced part, otherwise we get recursive replacement which will not end
    uiOffset = static_cast<ezUInt32>(szFoundAt - GetData()) + uiWordBytes;

    ReplaceSubString(szFoundAt, szFoundAt + uiSearchBytes, szReplaceWith);

    ++uiReplacements;
  }

  return uiReplacements;
}

void ezStringBuilder::operator=(ezStringView rhs)
{
  ezUInt32 uiBytes;
  ezUInt32 uiCharacters;

  ezStringUtils::GetCharacterAndElementCount(rhs.GetStartPointer(), uiCharacters, uiBytes, rhs.GetEndPointer());

  // if we need more room, allocate up front (rhs cannot use our own data in this case)
  if (uiBytes + 1 > m_Data.GetCount())
    m_Data.SetCountUninitialized(uiBytes + 1);

  // the data might actually come from our very own string, so we 'move' the memory in there, just to be safe
  // if it comes from our own array, the data will always be a sub-set -> smaller than this array
  // in this case we defer the SetCount till later, to ensure that the data is not corrupted (destructed) before we copy it
  // however, when the new data is larger than the old, it cannot be from our own data, so we can (and must) reallocate before copying
  ezMemoryUtils::CopyOverlapped(&m_Data[0], rhs.GetStartPointer(), uiBytes);

  m_Data.SetCountUninitialized(uiBytes + 1);
  m_Data[uiBytes] = '\0';

  m_uiCharacterCount = uiCharacters;
}

enum PathUpState
{
  NotStarted,
  OneDot,
  TwoDots,
  FoundDotSlash,
  FoundDotDotSlash,
  Invalid,
};

void ezStringBuilder::MakeCleanPath()
{
  if (IsEmpty())
    return;

  Trim(" \t\r\n");

  RemoveDoubleSlashesInPath();

  // remove Windows specific DOS device path indicators from the start
  TrimWordStart("//?/");
  TrimWordStart("//./");

  const char* const szEndPos = &m_Data[m_Data.GetCount() - 1];
  const char* szCurReadPos = &m_Data[0];
  char* const szCurWritePos = &m_Data[0];
  int writeOffset = 0;

  ezInt32 iLevelsDown = 0;
  PathUpState FoundPathUp = NotStarted;

  while (szCurReadPos < szEndPos)
  {
    char CurChar = *szCurReadPos;

    if (CurChar == '.')
    {
      if (FoundPathUp == NotStarted)
        FoundPathUp = OneDot;
      else if (FoundPathUp == OneDot)
        FoundPathUp = TwoDots;
      else
        FoundPathUp = Invalid;
    }
    else if (ezPathUtils::IsPathSeparator(CurChar))
    {
      CurChar = '/';

      if (FoundPathUp == OneDot)
      {
        FoundPathUp = FoundDotSlash;
      }
      else if (FoundPathUp == TwoDots)
      {
        FoundPathUp = FoundDotDotSlash;
      }
      else
      {
        ++iLevelsDown;
        FoundPathUp = NotStarted;
      }
    }
    else
      FoundPathUp = NotStarted;

    if (FoundPathUp == FoundDotDotSlash)
    {
      if (iLevelsDown > 0)
      {
        --iLevelsDown;
        EZ_ASSERT_DEBUG(writeOffset >= 3, "invalid write offset");
        writeOffset -= 3; // go back, skip two dots, one slash

        while ((writeOffset > 0) && (szCurWritePos[writeOffset - 1] != '/'))
        {
          EZ_ASSERT_DEBUG(writeOffset > 0, "invalid write offset");
          --writeOffset;
        }
      }
      else
      {
        szCurWritePos[writeOffset] = '/';
        ++writeOffset;
      }

      FoundPathUp = NotStarted;
    }
    else if (FoundPathUp == FoundDotSlash)
    {
      EZ_ASSERT_DEBUG(writeOffset > 0, "invalid write offset");
      writeOffset -= 1; // go back to where we wrote the dot

      FoundPathUp = NotStarted;
    }
    else
    {
      szCurWritePos[writeOffset] = CurChar;
      ++writeOffset;
    }

    ++szCurReadPos;
  }

  const ezUInt32 uiPrevByteCount = m_Data.GetCount();
  const ezUInt32 uiNewByteCount = (ezUInt32)(writeOffset) + 1;

  EZ_ASSERT_DEBUG(uiPrevByteCount >= uiNewByteCount, "It should not be possible that a path grows during cleanup. Old: {0} Bytes, New: {1} Bytes",
    uiPrevByteCount, uiNewByteCount);

  // we will only remove characters and only ASCII ones (slash, backslash, dot)
  // so the number of characters shrinks equally to the number of bytes
  m_uiCharacterCount -= (uiPrevByteCount - uiNewByteCount);

  // make sure to write the terminating \0 and reset the count
  szCurWritePos[writeOffset] = '\0';
  m_Data.SetCountUninitialized(uiNewByteCount);
}

void ezStringBuilder::PathParentDirectory(ezUInt32 uiLevelsUp)
{
  EZ_ASSERT_DEV(uiLevelsUp > 0, "We have to do something!");

  for (ezUInt32 i = 0; i < uiLevelsUp; ++i)
    AppendPath("../");

  MakeCleanPath();
}

void ezStringBuilder::AppendPath(ezStringView sPath1, ezStringView sPath2, ezStringView sPath3, ezStringView sPath4)
{
  const ezStringView sPaths[4] = {sPath1, sPath2, sPath3, sPath4};

  for (ezUInt32 i = 0; i < 4; ++i)
  {
    ezStringView sThisPath = sPaths[i];

    if (!sThisPath.IsEmpty())
    {
      if ((IsEmpty() && ezPathUtils::IsAbsolutePath(sPaths[i])))
      {
        // this is for Linux systems where absolute paths start with a slash, wouldn't want to remove that
      }
      else
      {
        // prevent creating multiple path separators through concatenation
        while (ezPathUtils::IsPathSeparator(*sThisPath.GetStartPointer()))
          sThisPath.Shrink(1, 0);
      }

      if (IsEmpty() || ezPathUtils::IsPathSeparator(GetIteratorBack().GetCharacter()))
        Append(sThisPath);
      else
        Append("/", sThisPath);
    }
  }
}

void ezStringBuilder::AppendWithSeparator(ezStringView optional, ezStringView sText1, ezStringView sText2 /*= ezStringView()*/,
  ezStringView sText3 /*= ezStringView()*/, ezStringView sText4 /*= ezStringView()*/, ezStringView sText5 /*= ezStringView()*/,
  ezStringView sText6 /*= ezStringView()*/)
{
  // if this string already ends with the optional string, reset it to be empty
  if (IsEmpty() || ezStringUtils::EndsWith(GetData(), optional.GetStartPointer(), GetData() + GetElementCount(), optional.GetEndPointer()))
  {
    optional = ezStringView();
  }

  const ezUInt32 uiMaxParams = 7;

  const ezStringView pStrings[uiMaxParams] = {optional, sText1, sText2, sText3, sText4, sText5, sText6};
  ezUInt32 uiStrLen[uiMaxParams] = {0};
  ezUInt32 uiMoreBytes = 0;

  // first figure out how much the string has to grow
  for (ezUInt32 i = 0; i < uiMaxParams; ++i)
  {
    if (pStrings[i].IsEmpty())
      continue;

    EZ_ASSERT_DEBUG(pStrings[i].GetStartPointer() < m_Data.GetData() || pStrings[i].GetStartPointer() >= m_Data.GetData() + m_Data.GetCapacity(),
      "Parameter {0} comes from the string builders own storage. This type assignment is not allowed.", i);

    ezUInt32 uiCharacters = 0;
    ezStringUtils::GetCharacterAndElementCount(pStrings[i].GetStartPointer(), uiCharacters, uiStrLen[i], pStrings[i].GetEndPointer());
    uiMoreBytes += uiStrLen[i];
    m_uiCharacterCount += uiCharacters;

    EZ_ASSERT_DEV(ezUnicodeUtils::IsValidUtf8(pStrings[i].GetStartPointer(), pStrings[i].GetEndPointer()), "Parameter {0} is not a valid Utf8 sequence.", i + 1);
  }

  ezUInt32 uiPrevCount = m_Data.GetCount(); // already contains a 0 terminator
  EZ_ASSERT_DEBUG(uiPrevCount > 0, "There should be a 0 terminator somewhere around here.");

  // now resize
  m_Data.SetCountUninitialized(uiPrevCount + uiMoreBytes);

  // and then append all the strings
  for (ezUInt32 i = 0; i < uiMaxParams; ++i)
  {
    if (uiStrLen[i] == 0)
      continue;

    // make enough room to copy the entire string, including the T-800
    ezStringUtils::Copy(&m_Data[uiPrevCount - 1], uiStrLen[i] + 1, pStrings[i].GetStartPointer(), pStrings[i].GetStartPointer() + uiStrLen[i]);

    uiPrevCount += uiStrLen[i];
  }
}

void ezStringBuilder::ChangeFileName(ezStringView sNewFileName)
{
  ezStringView it = ezPathUtils::GetFileName(GetView());

  ReplaceSubString(it.GetStartPointer(), it.GetEndPointer(), sNewFileName);
}

void ezStringBuilder::ChangeFileNameAndExtension(ezStringView sNewFileNameWithExtension)
{
  ezStringView it = ezPathUtils::GetFileNameAndExtension(GetView());

  ReplaceSubString(it.GetStartPointer(), it.GetEndPointer(), sNewFileNameWithExtension);
}

void ezStringBuilder::ChangeFileExtension(ezStringView sNewExtension)
{
  EZ_ASSERT_DEV(!sNewExtension.StartsWith("."), "The given extension string must not start with a dot.");

  ezStringView it = ezPathUtils::GetFileExtension(GetView());

  if (it.IsEmpty() && !EndsWith("."))
    Append(".", sNewExtension);
  else
    ReplaceSubString(it.GetStartPointer(), it.GetEndPointer(), sNewExtension);
}

void ezStringBuilder::RemoveFileExtension()
{
  if (HasAnyExtension())
  {
    ChangeFileExtension("");
    Shrink(0, 1); // remove the dot
  }
}

ezResult ezStringBuilder::MakeRelativeTo(ezStringView sAbsolutePathToMakeThisRelativeTo)
{
  ezStringBuilder sAbsBase = sAbsolutePathToMakeThisRelativeTo;
  sAbsBase.MakeCleanPath();
  ezStringBuilder sAbsThis = *this;
  sAbsThis.MakeCleanPath();

  if (sAbsBase.IsEqual_NoCase(sAbsThis.GetData()))
  {
    Clear();
    return EZ_SUCCESS;
  }

  if (!sAbsBase.EndsWith("/"))
    sAbsBase.Append("/");

  if (!sAbsThis.EndsWith("/"))
  {
    sAbsThis.Append("/");

    if (sAbsBase.StartsWith(sAbsThis.GetData()))
    {
      Clear();
      const char* szStart = &sAbsBase.GetData()[sAbsThis.GetElementCount()];

      while (*szStart != '\0')
      {
        if (*szStart == '/')
          Append("../");

        ++szStart;
      }

      return EZ_SUCCESS;
    }
    else
      sAbsThis.Shrink(0, 1);
  }

  const ezUInt32 uiMinLen = ezMath::Min(sAbsBase.GetElementCount(), sAbsThis.GetElementCount());

  ezInt32 iSame = uiMinLen - 1;
  for (; iSame >= 0; --iSame)
  {
    if (sAbsBase.GetData()[iSame] != '/')
      continue;

    // We need to check here if sAbsThis starts with sAbsBase in the range[0, iSame + 1]. However, we can't compare the first N bytes because those might not be a valid utf8 substring in absBase.
    // Thus we can't use IsEqualN_NoCase as N would need to be the number of characters, not bytes. Computing the number of characters in absBase would mean iterating the string twice.
    // As an alternative, as we know [0, iSame + 1] is a valid utf8 string in sAbsBase we can ask whether absThis starts with that substring.
    if (ezStringUtils::StartsWith_NoCase(sAbsThis.GetData(), sAbsBase.GetData(), sAbsThis.GetData() + sAbsThis.GetElementCount(), sAbsBase.GetData() + iSame + 1))
      break;
  }

  if (iSame < 0)
  {
    return EZ_FAILURE;
  }

  Clear();

  for (ezUInt32 ui = iSame + 1; ui < sAbsBase.GetElementCount(); ++ui)
  {
    if (sAbsBase.GetData()[ui] == '/')
      Append("../");
  }

  if (sAbsThis.GetData()[iSame] == '/')
    ++iSame;

  Append(&(sAbsThis.GetData()[iSame]));

  return EZ_SUCCESS;
}

/// An empty folder (zero length) does not contain ANY files.\n
/// A non-existing file-name (zero length) is never in any folder.\n
/// Example:\n
/// IsFileBelowFolder ("", "XYZ") -> always false\n
/// IsFileBelowFolder ("XYZ", "") -> always false\n
/// IsFileBelowFolder ("", "") -> always false\n
bool ezStringBuilder::IsPathBelowFolder(const char* szPathToFolder)
{
  EZ_ASSERT_DEV(
    !ezStringUtils::IsNullOrEmpty(szPathToFolder), "The given path must not be empty. Because is 'nothing' under the empty path, or 'everything' ?");

  // a non-existing file is never in any folder
  if (IsEmpty())
    return false;

  MakeCleanPath();

  ezStringBuilder sBasePath(szPathToFolder);
  sBasePath.MakeCleanPath();

  if (IsEqual_NoCase(sBasePath.GetData()))
    return true;

  if (!sBasePath.EndsWith("/"))
    sBasePath.Append("/");

  return StartsWith_NoCase(sBasePath.GetData());
}

void ezStringBuilder::MakePathSeparatorsNative()
{
  const char sep = ezPathUtils::OsSpecificPathSeparator;

  MakeCleanPath();
  ReplaceAll("/", ezStringView(&sep, 1));
}

void ezStringBuilder::RemoveDoubleSlashesInPath()
{
  if (IsEmpty())
    return;

  const char* szReadPos = &m_Data[0];
  char* szCurWritePos = &m_Data[0];

  ezInt32 iAllowedSlashes = 2;

  while (*szReadPos != '\0')
  {
    char CurChar = *szReadPos;
    ++szReadPos;

    if (CurChar == '\\')
      CurChar = '/';

    if (CurChar != '/')
      iAllowedSlashes = 1;
    else
    {
      if (iAllowedSlashes > 0)
        --iAllowedSlashes;
      else
        continue;
    }

    *szCurWritePos = CurChar;
    ++szCurWritePos;
  }


  const ezUInt32 uiPrevByteCount = m_Data.GetCount();
  const ezUInt32 uiNewByteCount = (ezUInt32)(szCurWritePos - &m_Data[0]) + 1;

  EZ_ASSERT_DEBUG(uiPrevByteCount >= uiNewByteCount, "It should not be possible that a path grows during cleanup. Old: {0} Bytes, New: {1} Bytes",
    uiPrevByteCount, uiNewByteCount);

  // we will only remove characters and only ASCII ones (slash, backslash)
  // so the number of characters shrinks equally to the number of bytes
  m_uiCharacterCount -= (uiPrevByteCount - uiNewByteCount);

  // make sure to write the terminating \0 and reset the count
  *szCurWritePos = '\0';
  m_Data.SetCountUninitialized(uiNewByteCount);
}


void ezStringBuilder::ReadAll(ezStreamReader& Stream)
{
  Clear();

  ezHybridArray<ezUInt8, 1024 * 4> Bytes(m_Data.GetAllocator());
  ezUInt8 Temp[1024];

  while (true)
  {
    const ezUInt32 uiRead = (ezUInt32)Stream.ReadBytes(Temp, 1024);

    if (uiRead == 0)
      break;

    Bytes.PushBackRange(ezArrayPtr<ezUInt8>(Temp, uiRead));
  }

  Bytes.PushBack('\0');

  *this = (const char*)&Bytes[0];
}

void ezStringBuilder::Trim(const char* szTrimChars)
{
  return Trim(szTrimChars, szTrimChars);
}

void ezStringBuilder::Trim(const char* szTrimCharsStart, const char* szTrimCharsEnd)
{
  const char* szNewStart = GetData();
  const char* szNewEnd = GetData() + GetElementCount();
  ezStringUtils::Trim(szNewStart, szNewEnd, szTrimCharsStart, szTrimCharsEnd);
  Shrink(ezStringUtils::GetCharacterCount(GetData(), szNewStart), ezStringUtils::GetCharacterCount(szNewEnd, GetData() + GetElementCount()));
}

bool ezStringBuilder::TrimWordStart(const char* szWord1, const char* szWord2 /*= nullptr*/, const char* szWord3 /*= nullptr*/,
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

bool ezStringBuilder::TrimWordEnd(const char* szWord1, const char* szWord2 /*= nullptr*/, const char* szWord3 /*= nullptr*/,
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

void ezStringBuilder::Format(const ezFormatString& string)
{
  Clear();
  const char* szText = string.GetText(*this);

  // this is for the case that GetText does not use the ezStringBuilder as temp storage
  if (szText != GetData())
    *this = szText;
}

void ezStringBuilder::AppendFormat(const ezFormatString& string)
{
  ezStringBuilder tmp;
  ezStringView view = string.GetText(tmp);

  Append(view);
}

void ezStringBuilder::PrependFormat(const ezFormatString& string)
{
  ezStringBuilder tmp;

  Prepend(string.GetText(tmp));
}

void ezStringBuilder::Printf(const char* szUtf8Format, ...)
{
  va_list args;
  va_start(args, szUtf8Format);

  PrintfArgs(szUtf8Format, args);

  va_end(args);
}

EZ_STATICLINK_FILE(Foundation, Foundation_Strings_Implementation_StringBuilder);
