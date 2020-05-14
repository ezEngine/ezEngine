#pragma once

#include <Foundation/Containers/HybridArray.h>
#include <Foundation/Strings/Implementation/StringBase.h>
#include <Foundation/Strings/StringConversion.h>
#include <Foundation/Strings/StringUtils.h>
#include <Foundation/Strings/StringView.h>
#include <Foundation/Algorithm/HashingUtils.h>

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
  ezHybridStringBase(ezAllocatorBase* pAllocator); // [tested]

  /// \brief Copies the data from \a rhs.
  ezHybridStringBase(const ezHybridStringBase& rhs, ezAllocatorBase* pAllocator); // [tested]

  /// \brief Moves the data from \a rhs.
  ezHybridStringBase(ezHybridStringBase&& rhs, ezAllocatorBase* pAllocator); // [tested]

  /// \brief Copies the data from \a rhs.
  ezHybridStringBase(const char* rhs, ezAllocatorBase* pAllocator); // [tested]

  /// \brief Copies the data from \a rhs.
  ezHybridStringBase(const wchar_t* rhs, ezAllocatorBase* pAllocator); // [tested]

  /// \brief Copies the data from \a rhs.
  ezHybridStringBase(const ezStringView& rhs, ezAllocatorBase* pAllocator); // [tested]

  /// \brief Copies the data from \a rhs.
  ezHybridStringBase(const ezStringBuilder& rhs, ezAllocatorBase* pAllocator); // [tested]

  /// \brief Moves the data from \a rhs.
  ezHybridStringBase(ezStringBuilder&& rhs, ezAllocatorBase* pAllocator); // [tested]

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

public:
  /// \brief Returns a string view to this string's data.
  operator ezStringView() const; // [tested]

  /// \brief Returns a string view to this string's data.
  ezStringView GetView() const;

  /// \brief Returns a pointer to the internal Utf8 string.
  EZ_ALWAYS_INLINE operator const char*() const { return GetData(); }

  /// \brief Resets this string to an empty string.
  ///
  /// This will not deallocate any previously allocated data, but reuse that memory.
  void Clear(); // [tested]

  /// \brief Returns a pointer to the internal Utf8 string.
  const char* GetData() const; // [tested]

  /// \brief Returns the amount of bytes that this string takes (excluding the '\0' terminator).
  ezUInt32 GetElementCount() const; // [tested]

  /// \brief Returns the number of characters in this string.
  ezUInt32 GetCharacterCount() const; // [tested]

  /// \brief Returns an iterator to a sub-string of this string, starting at character uiFirstCharacter, up until uiFirstCharacter +
  /// uiNumCharacters.
  ///
  /// Note that this iterator will only be valid as long as this ezHybridString lives.
  /// Once the original string is destroyed, all iterators to them will point into invalid memory.
  ezStringView GetSubString(ezUInt32 uiFirstCharacter, ezUInt32 uiNumCharacters) const; // [tested]

  /// \brief Returns an iterator to the sub-string containing the first uiNumCharacters characters of this string.
  ///
  /// Note that this iterator will only be valid as long as this ezHybridString lives.
  /// Once the original string is destroyed, all iterators to them will point into invalid memory.
  ezStringView GetFirst(ezUInt32 uiNumCharacters) const; // [tested]

  /// \brief Returns an iterator to the sub-string containing the last uiNumCharacters characters of this string.
  ///
  /// Note that this iterator will only be valid as long as this ezHybridString lives.
  /// Once the original string is destroyed, all iterators to them will point into invalid memory.
  ezStringView GetLast(ezUInt32 uiNumCharacters) const; // [tested]

  /// \brief Replaces the current string with the content from the stream. Reads the stream to its end.
  void ReadAll(ezStreamReader& Stream);

  /// \brief Returns the amount of bytes that are currently allocated on the heap.
  ezUInt64 GetHeapMemoryUsage() const { return m_Data.GetHeapMemoryUsage(); }

private:
  friend class ezStringBuilder;

  ezHybridArray<char, Size> m_Data;
  ezUInt32 m_uiCharacterCount = 0;
};


/// \brief \see ezHybridStringBase
template <ezUInt16 Size, typename AllocatorWrapper = ezDefaultAllocatorWrapper>
struct ezHybridString : public ezHybridStringBase<Size>
{
public:
  EZ_DECLARE_MEM_RELOCATABLE_TYPE();

  ezHybridString();
  ezHybridString(ezAllocatorBase* pAllocator);

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
  void operator=(const wchar_t* szString);
  void operator=(const ezStringView& rhs);
  void operator=(const ezStringBuilder& rhs);
  void operator=(ezStringBuilder&& rhs);

  void operator=(ezHybridString<Size, AllocatorWrapper>&& rhs);
  void operator=(ezHybridStringBase<Size>&& rhs);
};

using ezDynamicString = ezHybridString<1>;
/// \brief String that uses the static allocator to prevent leak reports in RTTI attributes.
using ezUntrackedString = ezHybridString<32, ezStaticAllocatorWrapper>;
using ezString = ezHybridString<32>;
using ezString16 = ezHybridString<16>;
using ezString24 = ezHybridString<24>;
using ezString32 = ezHybridString<32>;
using ezString48 = ezHybridString<48>;
using ezString64 = ezHybridString<64>;
using ezString128 = ezHybridString<128>;
using ezString256 = ezHybridString<256>;

EZ_CHECK_AT_COMPILETIME_MSG(ezGetTypeClass<ezString>::value == 2, "string is not memory relocatable");

template <ezUInt16 Size>
struct ezCompareHelper<ezHybridString<Size>>
{
  template <typename DerivedLhs, typename DerivedRhs>
  EZ_ALWAYS_INLINE bool Less(const ezStringBase<DerivedLhs>& lhs, const ezStringBase<DerivedRhs>& rhs) const
  {
    return ezStringUtils::Compare(lhs.InternalGetData(), rhs.InternalGetData(), lhs.InternalGetDataEnd(), rhs.InternalGetDataEnd()) < 0;
  }

  template <typename DerivedRhs>
  EZ_ALWAYS_INLINE bool Less(const char* lhs, const ezStringBase<DerivedRhs>& rhs) const
  {
    return rhs.Compare(lhs) > 0;
  }

  template <typename DerivedLhs>
  EZ_ALWAYS_INLINE bool Less(const ezStringBase<DerivedLhs>& lhs, const char* rhs) const
  {
    return lhs.Compare(rhs) < 0;
  }

  template <typename DerivedLhs, typename DerivedRhs>
  EZ_ALWAYS_INLINE bool Equal(const ezStringBase<DerivedLhs>& lhs, const ezStringBase<DerivedRhs>& rhs) const
  {
    return ezStringUtils::IsEqual(lhs.InternalGetData(), rhs.InternalGetData(), lhs.InternalGetDataEnd(), rhs.InternalGetDataEnd());
  }

  template <typename DerivedRhs>
  EZ_ALWAYS_INLINE bool Equal(const char* lhs, const ezStringBase<DerivedRhs>& rhs) const
  {
    return rhs.IsEqual(lhs);
  }

  template <typename DerivedLhs>
  EZ_ALWAYS_INLINE bool Equal(const ezStringBase<DerivedLhs>& lhs, const char* rhs) const
  {
    return lhs.IsEqual(rhs);
  }
};

struct ezCompareString_NoCase
{
  template <typename DerivedLhs, typename DerivedRhs>
  EZ_ALWAYS_INLINE bool Less(const ezStringBase<DerivedLhs>& lhs, const ezStringBase<DerivedRhs>& rhs) const
  {
    return ezStringUtils::Compare_NoCase(lhs.InternalGetData(), rhs.InternalGetData(), lhs.InternalGetDataEnd(), rhs.InternalGetDataEnd()) <
           0;
  }

  template <typename DerivedRhs>
  EZ_ALWAYS_INLINE bool Less(const char* lhs, const ezStringBase<DerivedRhs>& rhs) const
  {
    return rhs.Compare_NoCase(lhs) > 0;
  }

  template <typename DerivedLhs>
  EZ_ALWAYS_INLINE bool Less(const ezStringBase<DerivedLhs>& lhs, const char* rhs) const
  {
    return lhs.Compare_NoCase(rhs) < 0;
  }

  template <typename DerivedLhs, typename DerivedRhs>
  EZ_ALWAYS_INLINE bool Equal(const ezStringBase<DerivedLhs>& lhs, const ezStringBase<DerivedRhs>& rhs) const
  {
    return ezStringUtils::IsEqual_NoCase(lhs.InternalGetData(), rhs.InternalGetData(), lhs.InternalGetDataEnd(), rhs.InternalGetDataEnd());
  }

  template <typename DerivedRhs>
  EZ_ALWAYS_INLINE bool Equal(const char* lhs, const ezStringBase<DerivedRhs>& rhs) const
  {
    return rhs.IsEqual_NoCase(lhs);
  }

  template <typename DerivedLhs>
  EZ_ALWAYS_INLINE bool Equal(const ezStringBase<DerivedLhs>& lhs, const char* rhs) const
  {
    return lhs.IsEqual_NoCase(rhs);
  }
};

struct CompareConstChar
{
  /// \brief Returns true if a is less than b
  EZ_ALWAYS_INLINE bool Less(const char* a, const char* b) const { return ezStringUtils::Compare(a, b) < 0; }

  /// \brief Returns true if a is equal to b
  EZ_ALWAYS_INLINE bool Equal(const char* a, const char* b) const { return ezStringUtils::IsEqual(a, b); }
};

// For ezFormatString
EZ_FOUNDATION_DLL ezStringView BuildString(char* tmp, ezUInt32 uiLength, const ezString& arg);
EZ_FOUNDATION_DLL ezStringView BuildString(char* tmp, ezUInt32 uiLength, const ezUntrackedString& arg);

#include <Foundation/Strings/Implementation/String_inl.h>

