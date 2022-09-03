#ifndef AE_FOUNDATION_STRINGS_BASICSTRING_INL
#define AE_FOUNDATION_STRINGS_BASICSTRING_INL

#include "../StringFunctions.h"

namespace AE_NS_FOUNDATION
{
  inline aeBasicString::aeBasicString (const char* szString) : m_pDataToRead ((char*) szString)
  {
    if (szString == nullptr)
      m_pDataToRead = (char*)"";

    m_uiLength = aeStringFunctions::GetLength (szString);
  }

  inline const char aeBasicString::operator[](aeUInt32 index) const
  {
    AE_CHECK_DEV (index < m_uiLength, "aeBasicString::operator[]: Cannot access character %d, the string only has a length of %d.", index, m_uiLength);
    
    return (m_pDataToRead[index]);
  }

  inline bool aeBasicString::CompareEqual (const char* szString2) const
  {
    return (aeStringFunctions::CompareEqual (m_pDataToRead, szString2));
  }

  inline bool aeBasicString::CompareEqual (const char* szString2, aeUInt32 uiCharsToCompare) const
  {
    return (aeStringFunctions::CompareEqual (m_pDataToRead, szString2, uiCharsToCompare));
  }

  inline bool aeBasicString::CompareEqual_NoCase (const char* szString2) const
  {
    return (aeStringFunctions::CompareEqual_NoCase (m_pDataToRead, szString2));
  }

  inline bool aeBasicString::CompareEqual_NoCase (const char* szString2, aeUInt32 uiCharsToCompare) const
  {
    return (aeStringFunctions::CompareEqual_NoCase (m_pDataToRead, szString2, uiCharsToCompare));
  }

  inline aeInt32 aeBasicString::CompareAlphabetically (const char* szString2) const
  {
    return (aeStringFunctions::CompareAlphabetically (m_pDataToRead, szString2));
  }

  inline aeInt32 aeBasicString::CompareAlphabetically (const char* szString2, aeUInt32 uiCharsToCompare) const
  {
    return (aeStringFunctions::CompareAlphabetically (m_pDataToRead, szString2, uiCharsToCompare));
  }

  inline aeInt32 aeBasicString::CompareAlphabetically_NoCase (const char* szString2) const
  {
    return (aeStringFunctions::CompareAlphabetically_NoCase (m_pDataToRead, szString2));
  }

  inline aeInt32 aeBasicString::CompareAlphabetically_NoCase (const char* szString2, aeUInt32 uiCharsToCompare) const
  {
    return (aeStringFunctions::CompareAlphabetically_NoCase (m_pDataToRead, szString2, uiCharsToCompare));
  }

  inline aeInt32 aeBasicString::FindFirstStringPos (const char* szStringToFind, aeUInt32 uiStartPos) const
  {
    return (aeStringFunctions::FindFirstStringPos (m_pDataToRead, szStringToFind, uiStartPos));
  }

  inline aeInt32 aeBasicString::FindFirstStringPos_NoCase (const char* szStringToFind, aeUInt32 uiStartPos) const
  {
    return (aeStringFunctions::FindFirstStringPos_NoCase (m_pDataToRead, szStringToFind, uiStartPos));
  }

  inline aeInt32 aeBasicString::FindLastStringPos  (const char* szStringToFind, aeUInt32 uiStartPos) const
  {
    return (aeStringFunctions::FindLastStringPos (m_pDataToRead, szStringToFind, uiStartPos));
  }

  inline aeInt32 aeBasicString::FindLastStringPos_NoCase  (const char* szStringToFind, aeUInt32 uiStartPos) const
  {
    return (aeStringFunctions::FindLastStringPos_NoCase (m_pDataToRead, szStringToFind, uiStartPos));
  }

  inline bool aeBasicString::StartsWith (const char* szStartsWith) const
  {
    return (aeStringFunctions::StartsWith (c_str (), szStartsWith));
  }

  inline bool aeBasicString::StartsWith_NoCase (const char* szStartsWith) const
  {
    return (aeStringFunctions::StartsWith_NoCase (c_str (), szStartsWith));
  }

  inline bool aeBasicString::EndsWith (const char* szEndsWith) const
  {
    return (aeStringFunctions::EndsWith (c_str (), szEndsWith));
  }

  inline bool aeBasicString::EndsWith_NoCase (const char* szEndsWith) const
  {
    return (aeStringFunctions::EndsWith_NoCase (c_str (), szEndsWith));
  }

  inline aeInt32 aeBasicString::FindWholeWord (const char* szSearchFor, aeStringFunctions::AE_IS_WORD_DELIMITER IsDelimiterCB, aeUInt32 uiStartPos) const
  {
    return (aeStringFunctions::FindWholeWord (c_str (), szSearchFor, IsDelimiterCB, uiStartPos));
  }

  inline aeInt32 aeBasicString::FindWholeWord_NoCase (const char* szSearchFor, aeStringFunctions::AE_IS_WORD_DELIMITER IsDelimiterCB, aeUInt32 uiStartPos) const
  {
    return (aeStringFunctions::FindWholeWord_NoCase (c_str (), szSearchFor, IsDelimiterCB, uiStartPos));
  }

  inline bool operator== (const aeBasicString& lhs, const aeBasicString& rhs)
  {
    if (lhs.length () != rhs.length ())
      return (false);

    return (aeStringFunctions::CompareEqual (lhs.c_str (), rhs.c_str ()));
  }

  inline bool operator== (const char* lhs, const aeBasicString& rhs)
  {
    return (aeStringFunctions::CompareEqual (lhs, rhs.c_str ()));
  }

  inline bool operator== (const aeBasicString& lhs, const char* rhs)
  {
    return (aeStringFunctions::CompareEqual (lhs.c_str (), rhs));
  }


  inline bool operator!= (const aeBasicString& lhs, const aeBasicString& rhs)
  {
    return (! (lhs == rhs) );
  }

  inline bool operator!= (const char* lhs, const aeBasicString& rhs)
  {
    return (! (lhs == rhs) );
  }

  inline bool operator!= (const aeBasicString& lhs, const char* rhs)
  {
    return (! (lhs == rhs) );
  }


  inline bool operator< (const aeBasicString& lhs, const aeBasicString& rhs)
  {
    return (aeStringFunctions::CompareAlphabetically (lhs.c_str (), rhs.c_str ()) < 0);
  }

  inline bool operator< (const char* lhs, const aeBasicString& rhs)
  {
    return (aeStringFunctions::CompareAlphabetically (lhs, rhs.c_str ()) < 0);
  }

  inline bool operator< (const aeBasicString& lhs, const char* rhs)
  {
    return (aeStringFunctions::CompareAlphabetically (lhs.c_str (), rhs) < 0);
  }


  inline bool operator> (const aeBasicString& lhs, const aeBasicString& rhs)
  {
    return (aeStringFunctions::CompareAlphabetically (lhs.c_str (), rhs.c_str ()) > 0);
  }

  inline bool operator> (const char* lhs, const aeBasicString& rhs)
  {
    return (aeStringFunctions::CompareAlphabetically (lhs, rhs.c_str ()) > 0);
  }

  inline bool operator> (const aeBasicString& lhs, const char* rhs)
  {
    return (aeStringFunctions::CompareAlphabetically (lhs.c_str (), rhs) > 0);
  }


  inline bool operator<= (const aeBasicString& lhs, const aeBasicString& rhs)
  {
    return (aeStringFunctions::CompareAlphabetically (lhs.c_str (), rhs.c_str ()) <= 0);
  }

  inline bool operator<= (const char* lhs, const aeBasicString& rhs)
  {
    return (aeStringFunctions::CompareAlphabetically (lhs, rhs.c_str ()) <= 0);
  }

  inline bool operator<= (const aeBasicString& lhs, const char* rhs)
  {
    return (aeStringFunctions::CompareAlphabetically (lhs.c_str (), rhs) <= 0);
  }


  inline bool operator>= (const aeBasicString& lhs, const aeBasicString& rhs)
  {
    return (aeStringFunctions::CompareAlphabetically (lhs.c_str (), rhs.c_str ()) >= 0);
  }

  inline bool operator>= (const char* lhs, const aeBasicString& rhs)
  {
    return (aeStringFunctions::CompareAlphabetically (lhs, rhs.c_str ()) >= 0);
  }

  inline bool operator>= (const aeBasicString& lhs, const char* rhs)
  {
    return (aeStringFunctions::CompareAlphabetically (lhs.c_str (), rhs) >= 0);
  }


}

#endif
