#ifndef AE_FOUNDATION_STRINGS_STRING_H
#define AE_FOUNDATION_STRINGS_STRING_H

#include "BasicString.h"
#include "Declarations.h"

namespace AE_NS_FOUNDATION
{
  //! A string class that can handle strings of any length.
  /*! A dynamic string is a string whose data is always allocated on the heap.
      As long as the string is empty, it will not allocate any memory.
      When it is changed, it is reallocated, if its capacity is too small.
      Operations that make the string shorter usually do not trigger reallocations.
  */
  class AE_FOUNDATION_DLL aeString : public aeBasicString
  {
  public:
    //! Initializes the string to be empty.
    aeString(void);
    //! Initializes the string with a copy of the given string.
    aeString(const aeBasicString& cc);
    //! Initializes the string with a copy of the given string.
    aeString(const aeString& cc);
    //! Initializes the string with a copy of the given string.
    aeString(const char* cc);
    //! Initializes the string with a copy of the given string, but only up to uiMaxChar characters.
    aeString(const char* cc, aeUInt32 uiMaxChar);
    //! Deallocates all data, if anything was allocated.
    ~aeString();

    //! Makes sure the string has room for at least uiCapacity characters. Thus subsequent string-modifications might become more efficient.
    void reserve(aeUInt32 uiCapacity);

    //! Resizes the string, but NEVER deallocates data when shrinking the string. Only reallocates when growing above the current capacity.
    void resize(aeUInt32 uiNewSize);

    //! Returns how many characters the string can hold without need for reallocation.
    aeUInt32 capacity(void) const;

    //! Resets the string to "". Does NOT deallocate any data, unless bDeallocateData is specifically set to true.
    void clear(bool bDeallocateData = false);

    //! Appends the given string to this string.
    void Concatenate(const char* szSource);
    //! Appends up to uiCharsToCopy characters of the given string to this string.
    void Concatenate(const char* szSource, aeUInt32 uiCharsToCopy);

    // get operator[] from aeBasicString into this class
    using aeBasicString::operator[];
    //! Allows read/write-access to the character with the given index
    char& operator[](aeUInt32 index);

    //! Appends the given string to this string.
    void operator+=(const aeBasicString& str);
    //! Appends the given string to this string.
    void operator+=(const char* s);
    //! Appends the given character to this string.
    void operator+=(char c);

    //! Copies the given string into this one.
    void operator=(const aeString& str);
    //! Copies the given string into this one.
    void operator=(const aeBasicString& str);
    //! Copies the given string into this one.
    void operator=(const char* s);
    //! Copies the given character into this one.
    void operator=(char c);

    //! Replaces the sub-string between [uiPos; uiPos+uiLength] with szReplaceWith.
    void ReplaceSubString(aeUInt32 uiPos, aeUInt32 uiLength, const char* szReplaceWith);
    //! Inserts szInsert at/before uiPos.
    void Insert(aeUInt32 uiPos, const char* szInsert);

    //! Replaces the first occurance of szSearchFor after index uiStartPos with szReplaceWith. Returns true if anything was replaced.
    bool Replace(const char* szSearchFor, const char* szReplaceWith, aeUInt32 uiStartPos = 0);
    //! Replaces the first occurance of szSearchFor after index uiStartPos with szReplaceWith. Returns how many occurances were found and replaced.
    aeUInt32 ReplaceAll(const char* szSearchFor, const char* szReplaceWith, aeUInt32 uiStartPos = 0);

  private:
    //! Computes the next best capacity size to use for reallocations. Rounds to the next higher multiple of 32.
    static aeUInt32 ComputeStringCapacity(aeUInt32 uiStringLength);

    //! The current maximum of characters, that this string can hold.
    aeUInt32 m_uiCapacity;
  };

  //! Concatenation of basic strings => always returns a dynamic string.
  AE_FOUNDATION_DLL const aeString operator+(const aeBasicString& lhs, const char* rhs);
  //! Concatenation of basic strings => always returns a dynamic string.
  AE_FOUNDATION_DLL const aeString operator+(const char* lhs, const aeBasicString& rhs);
  //! Concatenation of basic strings => always returns a dynamic string.
  AE_FOUNDATION_DLL const aeString operator+(const aeBasicString& lhs, const aeBasicString& rhs);
} // namespace AE_NS_FOUNDATION

#include "Inline/String.inl"

#endif
