#include "../String.h"



namespace AE_NS_FOUNDATION
{
  void aeString::ReplaceSubString (aeUInt32 uiPos, aeUInt32 uiReplaceLength, const char* szReplaceWith)
  {
    AE_CHECK_DEV (szReplaceWith != nullptr, "aeString::ReplaceSubString: Cannot replace something with a nullptr-string.");
    AE_CHECK_DEV (uiPos + uiReplaceLength <= m_uiLength, "aeString::ReplaceSubString: The given range is invalid. Cannot replace [%d to %d], the String is only %d characters long.", uiPos, uiPos + uiReplaceLength, m_uiLength);

    const aeUInt32 uiWordLen = aeStringFunctions::GetLength (szReplaceWith);

    // most simple case, just replace characters
    if (uiReplaceLength == uiWordLen)
    {
      for (aeUInt32 ui = 0; ui < uiWordLen; ++ui)
        m_pDataToRead[uiPos + ui] = szReplaceWith[ui];

      return;
    }

    // the replacement is shorter than the existing stuff -> move characters to the left, no reallocation needed
    if (uiWordLen < uiReplaceLength)
    {
      // first copy the replacement to the correct position
      aeStringFunctions::Copy (&m_pDataToRead[uiPos], uiWordLen+1, szReplaceWith, uiWordLen);

      const aeUInt32 uiDifference = uiReplaceLength - uiWordLen;

      // now move all the characters from behind the replaced string to the correct position
      for (aeUInt32 ui = uiPos + uiWordLen; ui < m_uiLength - uiDifference; ++ui)
        m_pDataToRead[ui] = m_pDataToRead[ui + uiDifference];

      m_uiLength -= uiDifference;
      m_pDataToRead[m_uiLength] = '\0';
      return;
    }

    // else the replacement is longer than the existing word
    {
      const aeUInt32 uiOldLength = m_uiLength;
      const aeUInt32 uiNewLength = uiOldLength - uiReplaceLength + uiWordLen;
      const aeUInt32 uiMoveBack = uiOldLength - (uiPos + uiReplaceLength);

      resize(uiNewLength);

      // first move the characters to the proper position from back to front
      for (aeUInt32 move = 0; move < uiMoveBack; ++move)
      {
        const aeUInt32 uiOffsetDst = uiNewLength - move - 1;
        const aeUInt32 uiOffsetSrc = uiOldLength - move - 1;

        m_pDataToRead[uiOffsetDst] = m_pDataToRead[uiOffsetSrc];
      }

      // now copy the replacement to the correct position
      for (aeUInt32 ui = 0; ui < uiWordLen; ++ui)
      {
        m_pDataToRead[uiPos + ui] = szReplaceWith[ui];
      }
    }
  }

  void aeString::Insert (aeUInt32 uiPos, const char* szInsert)
  {
    ReplaceSubString (uiPos, 0, szInsert);
  }

  bool aeString::Replace (const char* szSearchFor, const char* szReplaceWith, aeUInt32 uiStartPos)
  {
    const aeInt32 iPos = FindFirstStringPos (szSearchFor, uiStartPos);
    if (iPos < 0)
      return (false);

    ReplaceSubString (iPos, aeStringFunctions::GetLength (szSearchFor), szReplaceWith);
    return (true);
  }

  aeUInt32 aeString::ReplaceAll (const char* szSearchFor, const char* szReplaceWith, aeUInt32 uiStartPos)
  {
    const aeUInt32 uiSearchLen = aeStringFunctions::GetLength (szSearchFor);
    const aeUInt32 uiReplaceLen = aeStringFunctions::GetLength (szReplaceWith);

    aeUInt32 uiCounter = 0;

    aeInt32 iFoundPos = FindFirstStringPos (szSearchFor, uiStartPos);
    while (iFoundPos >= 0)
    {
      ReplaceSubString (iFoundPos, uiSearchLen, szReplaceWith);
      ++uiCounter;

      uiStartPos = iFoundPos + uiReplaceLen;

      iFoundPos = FindFirstStringPos (szSearchFor, uiStartPos);

      AE_CHECK_DEV (uiCounter < 100000, "aeString::ReplaceAll: Too many replacements! Probably an endless loop.");
    }

    return (uiCounter);
  }
}

