#pragma once

#include <Foundation/Algorithm/HashingUtils.h>
#include <Foundation/Containers/HybridArray.h>
#include <Foundation/Strings/Implementation/StringBase.h>
#include <Foundation/Strings/StringConversion.h>
#include <Foundation/Strings/StringUtils.h>
#include <Foundation/Strings/StringView.h>

class ezStringBuilder;
class ezStreamReader;

/// \brief A string class for storing and passing around strings.
///
/// This class only allows read-access to its data. It does not allow modifications.
/// To build / modify strings, use the ezStringBuilder class.
/// ezHybridString has an internal array to store short strings without any memory allocations, it will dynamically
/// allocate additional memory, if that cache is insufficient. Thus a hybrid string will always take up a certain amount
/// of memory, which might be of concern when it is used as a member variable, in such cases you might want to use an
/// ezHybridString with a very small internal array (1 would basically make it into a completely dynamic string).
/// On the other hand, creating ezHybridString instances on the stack and working locally with them, is quite fast.
/// Prefer to use the typedef'd string types \a ezString, \a ezDynamicString, \a ezString32 etc.
/// Most strings in an application are rather short, typically shorter than 20 characters.
/// Use \a ezString, which is a typedef'd ezHybridString to use a cache size that is sufficient for more than 90%
/// of all use cases.
template <ezUInt16 Size>
struct ezHybridStringBase : public ezStringBase<ezHybridStringBase<Size>>
{
protected:
  /// \brief Creates an empty string.
  ezHybridStringBase(ezAllocator* pAllocator); // [tested]

  /// \brief Copies the data from \a rhs.
  ezHybridStringBase(const ezHybridStringBase& rhs, ezAllocator* pAllocator); // [tested]

  /// \brief Moves the data from \a rhs.
  ezHybridStringBase(ezHybridStringBase&& rhs, ezAllocator* pAllocator); // [tested]

  /// \brief Copies the data from \a rhs.
  ezHybridStringBase(const char* rhs, ezAllocator* pAllocator); // [tested]

  /// \brief Copies the data from \a rhs.
  ezHybridStringBase(const wchar_t* rhs, ezAllocator* pAllocator); // [tested]

  /// \brief Copies the data from \a rhs.
  ezHybridStringBase(const ezStringView& rhs, ezAllocator* pAllocator); // [tested]

  /// \brief Copies the data from \a rhs.
  ezHybridStringBase(const ezStringBuilder& rhs, ezAllocator* pAllocator); // [tested]

  /// \brief Moves the data from \a rhs.
  ezHybridStringBase(ezStringBuilder&& rhs, ezAllocator* pAllocator); // [tested]

  /// \brief Destructor.
  ~ezHybridStringBase(); // [tested]

  /// \brief Copies the data from \a rhs.
  void operator=(const ezHybridStringBase& rhs); // [tested]

  /// \brief Moves the data from \a rhs.
  void operator=(ezHybridStringBase&& rhs); // [tested]

  /// \brief Copies the data from \a rhs.
  void operator=(const char* rhs); // [tested]

  /// \brief Copies the data from \a rhs.
  void operator=(const wchar_t* rhs); // [tested]

  /// \brief Copies the data from \a rhs.
  void operator=(const ezStringView& rhs); // [tested]

  /// \brief Copies the data from \a rhs.
  void operator=(const ezStringBuilder& rhs); // [tested]

  /// \brief Moves the data from \a rhs.
  void operator=(ezStringBuilder&& rhs); // [tested]

#if EZ_ENABLED(EZ_INTEROP_STL_STRINGS)
  /// \brief Copies the data from \a rhs.
  ezHybridStringBase(const std::string_view& rhs, ezAllocator* pAllocator);

  /// \brief Copies the data from \a rhs.
  ezHybridStringBase(const std::string& rhs, ezAllocator* pAllocator);

  /// \brief Copies the data from \a rhs.
  void operator=(const std::string_view& rhs);

  /// \brief Copies the data from \a rhs.
  void operator=(const std::string& rhs);
#endif

public:
  /// \brief Resets this string to an empty string.
  ///
  /// This will not deallocate any previously allocated data, but reuse that memory.
  void Clear(); // [tested]

  /// \brief Returns a pointer to the internal Utf8 string.
  const char* GetData() const; // [tested]

  /// \brief Returns the amount of bytes that this string takes (excluding the '\0' terminator).
  ezUInt32 GetElementCount() const; // [tested]

  /// \brief Returns the number of characters in this string. Might be less than GetElementCount, if it contains Utf8
  /// multi-byte characters.
  ///
  /// \note This is a slow operation, as it has to run through the entire string to count the Unicode characters.
  /// Only call this once and use the result as long as the string doesn't change. Don't call this in a loop.
  ezUInt32 GetCharacterCount() const; // [tested]

  /// \brief Returns a view to a sub-string of this string, starting at character uiFirstCharacter, up until uiFirstCharacter +
  /// uiNumCharacters.
  ///
  /// Note that this view will only be valid as long as this ezHybridString lives.
  /// Once the original string is destroyed, all views to them will point into invalid memory.
  ezStringView GetSubString(ezUInt32 uiFirstCharacter, ezUInt32 uiNumCharacters) const; // [tested]

  /// \brief Returns a view to the sub-string containing the first uiNumCharacters characters of this string.
  ///
  /// Note that this view will only be valid as long as this ezHybridString lives.
  /// Once the original string is destroyed, all views to them will point into invalid memory.
  ezStringView GetFirst(ezUInt32 uiNumCharacters) const; // [tested]

  /// \brief Returns a view to the sub-string containing the last uiNumCharacters characters of this string.
  ///
  /// Note that this view will only be valid as long as this ezHybridString lives.
  /// Once the original string is destroyed, all views to them will point into invalid memory.
  ezStringView GetLast(ezUInt32 uiNumCharacters) const; // [tested]

  /// \brief Replaces the current string with the content from the stream. Reads the stream to its end.
  void ReadAll(ezStreamReader& inout_stream);

  /// \brief Returns the amount of bytes that are currently allocated on the heap.
  ezUInt64 GetHeapMemoryUsage() const { return m_Data.GetHeapMemoryUsage(); }

private:
  friend class ezStringBuilder;

  ezHybridArray<char, Size> m_Data;
};


/// \brief \see ezHybridStringBase
template <ezUInt16 Size, typename AllocatorWrapper = ezDefaultAllocatorWrapper>
struct ezHybridString : public ezHybridStringBase<Size>
{
public:
  ezHybridString();
  ezHybridString(ezAllocator* pAllocator);

  ezHybridString(const ezHybridString<Size, AllocatorWrapper>& other);
  ezHybridString(const ezHybridStringBase<Size>& other);
  ezHybridString(const char* rhs);
  ezHybridString(const wchar_t* rhs);
  ezHybridString(const ezStringView& rhs);
  ezHybridString(const ezStringBuilder& rhs);
  ezHybridString(ezStringBuilder&& rhs);
  ezHybridString(ezHybridString<Size, AllocatorWrapper>&& other);
  ezHybridString(ezHybridStringBase<Size>&& other);

  void operator=(const ezHybridString<Size, AllocatorWrapper>& rhs);
  void operator=(const ezHybridStringBase<Size>& rhs);
  void operator=(const char* szString);
  void operator=(const wchar_t* pString);
  void operator=(const ezStringView& rhs);
  void operator=(const ezStringBuilder& rhs);
  void operator=(ezStringBuilder&& rhs);
  void operator=(ezHybridString<Size, AllocatorWrapper>&& rhs);
  void operator=(ezHybridStringBase<Size>&& rhs);

#if EZ_ENABLED(EZ_INTEROP_STL_STRINGS)
  ezHybridString(const std::string_view& rhs);
  ezHybridString(const std::string& rhs);
  void operator=(const std::string_view& rhs);
  void operator=(const std::string& rhs);
#endif
};

/// \brief String that uses the static allocator to prevent leak reports in RTTI attributes.
using ezUntrackedString = ezHybridString<32, ezStaticsAllocatorWrapper>;

using ezDynamicString = ezHybridString<1>;
using ezString = ezHybridString<32>;
using ezString16 = ezHybridString<16>;
using ezString24 = ezHybridString<24>;
using ezString32 = ezHybridString<32>;
using ezString48 = ezHybridString<48>;
using ezString64 = ezHybridString<64>;
using ezString128 = ezHybridString<128>;
using ezString256 = ezHybridString<256>;

static_assert(ezGetTypeClass<ezString>::value == ezTypeIsClass::value);

template <ezUInt16 Size>
struct ezCompareHelper<ezHybridString<Size>>
{
  static EZ_ALWAYS_INLINE bool Less(ezStringView lhs, ezStringView rhs)
  {
    return lhs.Compare(rhs) < 0;
  }

  static EZ_ALWAYS_INLINE bool Equal(ezStringView lhs, ezStringView rhs)
  {
    return lhs.IsEqual(rhs);
  }
};

struct ezCompareString_NoCase
{
  static EZ_ALWAYS_INLINE bool Less(ezStringView lhs, ezStringView rhs)
  {
    return lhs.Compare_NoCase(rhs) < 0;
  }

  static EZ_ALWAYS_INLINE bool Equal(ezStringView lhs, ezStringView rhs)
  {
    return lhs.IsEqual_NoCase(rhs);
  }
};

struct CompareConstChar
{
  /// \brief Returns true if a is less than b
  static EZ_ALWAYS_INLINE bool Less(const char* a, const char* b) { return ezStringUtils::Compare(a, b) < 0; }

  /// \brief Returns true if a is equal to b
  static EZ_ALWAYS_INLINE bool Equal(const char* a, const char* b) { return ezStringUtils::IsEqual(a, b); }
};

// For ezFormatString
EZ_FOUNDATION_DLL ezStringView BuildString(char* szTmp, ezUInt32 uiLength, const ezString& sArg);
EZ_FOUNDATION_DLL ezStringView BuildString(char* szTmp, ezUInt32 uiLength, const ezUntrackedString& sArg);

#include <Foundation/Strings/Implementation/String_inl.h>
