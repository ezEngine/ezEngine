#pragma once

#include <Foundation/Strings/StringUtils.h>
#include <Foundation/Strings/StringIterator.h>
#include <Foundation/Strings/Implementation/StringBase.h>

/// A simple string class that has a static char array for storage and will not allocate any data.
/// Use this mainly for putting it into other containers (such as maps, sets, hashmaps), where only assignment and 
/// read operations are used, but construction without dynamic allocations is required.
template<ezUInt32 SIZE>
class ezStaticString : public ezStringBase< ezStaticString<SIZE> >
{
public:
  /// The string will be initialized to be empty.
  ezStaticString();

  /// Copies the data from the other string. Clamps the string, if it is longer than this string can handle.
  template<ezUInt32 OTHER_SIZE>
  ezStaticString(const ezStaticString<OTHER_SIZE>& rhs);

  /// Copies the data from the other string. Clamps the string, if it is longer than this string can handle.
  ezStaticString(const char* rhs);

  /// Copies the data from the other string. Clamps the string, if it is longer than this string can handle.
  template<ezUInt32 OTHER_SIZE>
  void operator=(const ezStaticString<OTHER_SIZE>& rhs);

  /// Copies the data from the other string. Clamps the string, if it is longer than this string can handle.
  void operator=(const char* rhs);
  
  /// Sets this string to be empty.
  void Clear();

  /// Returns the data for read access.
  const char* GetData() const;

  /// Returns the number of elements (bytes, NOT characters) in this string.
  ezUInt32 GetElementCount() const;

private:
  ezUInt16 m_uiElementCount;
  char m_szData[SIZE];
};

#include <Foundation/Strings/Implementation/StaticString_inl.h>

