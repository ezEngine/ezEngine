#pragma once

#include <Foundation/ThirdParty/utf8/utf8.h>

#include <Foundation/Containers/HybridArray.h>
#include <Foundation/Memory/MemoryUtils.h>
#include <Foundation/Strings/FormatString.h>
#include <Foundation/Strings/Implementation/StringBase.h>
#include <Foundation/Strings/PathUtils.h>
#include <Foundation/Strings/StringUtils.h>
#include <Foundation/Strings/StringView.h>

template <ezUInt16 Size>
struct ezHybridStringBase;

template <ezUInt16 Size, typename AllocatorWrapper>
struct ezHybridString;

class ezStreamReader;
class ezFormatString;

/// \brief ezStringBuilder is a class that is meant for creating and modifying strings.
///
/// It is not meant to store strings for a longer duration.
/// Each ezStringBuilder uses an ezHybridArray to allocate a large buffer on the stack, such that string manipulations
/// are possible without memory allocations, unless the string is too large.
/// No sharing of data happens between ezStringBuilder instances, as it is expected that they will be modified anyway.
/// Instead all data is always copied, therefore instances should not be passed by copy.
/// All string data is stored Utf8 encoded, just as all other string classes, too.
/// That makes it difficult to modify individual characters. Instead you should prefer high-level functions
/// such as 'ReplaceSubString'. If individual characters must be modified, it might make more sense to create
/// a second ezStringBuilder, and iterate over the first while rebuilding the desired result in the second.
/// Once a string is built and should only be stored for read access, it should be stored in an ezString instance.
class EZ_FOUNDATION_DLL ezStringBuilder : public ezStringBase<ezStringBuilder>
{
public:
  /// \brief Initializes the string to be empty. No data is allocated, but the ezStringBuilder ALWAYS creates an array on the stack.
  ezStringBuilder(ezAllocator* pAllocator = ezFoundation::GetDefaultAllocator()); // [tested]

  /// \brief Copies the given string into this one.
  ezStringBuilder(const ezStringBuilder& rhs); // [tested]

  /// \brief Moves the given string into this one.
  ezStringBuilder(ezStringBuilder&& rhs) noexcept;

  /// \brief Copies the given string into this one.
  template <ezUInt16 Size>
  ezStringBuilder(const ezHybridStringBase<Size>& rhs)
    : m_Data(rhs.m_Data)
  {
  }

  /// \brief Copies the given string into this one.
  template <ezUInt16 Size, typename A>
  ezStringBuilder(const ezHybridString<Size, A>& rhs)
    : m_Data(rhs.m_Data)
  {
  }


  /// \brief Moves the given string into this one.
  template <ezUInt16 Size>
  ezStringBuilder(ezHybridStringBase<Size>&& rhs)
    : m_Data(std::move(rhs.m_Data))
  {
  }

  /// \brief Moves the given string into this one.
  template <ezUInt16 Size, typename A>
  ezStringBuilder(ezHybridString<Size, A>&& rhs)
    : m_Data(std::move(rhs.m_Data))
  {
  }

  /// \brief Constructor that appends all the given strings.
  ezStringBuilder(ezStringView sData1, ezStringView sData2, ezStringView sData3 = {}, ezStringView sData4 = {},
    ezStringView sData5 = {}, ezStringView sData6 = {}); // [tested]

                                                         /// \brief Copies the given Utf8 string into this one.
  /* implicit */ ezStringBuilder(const char* szUTF8, ezAllocator* pAllocator = ezFoundation::GetDefaultAllocator()); // [tested]

                                                                                                                     /// \brief Copies the given wchar_t string into this one.
  /* implicit */ ezStringBuilder(const wchar_t* pWChar, ezAllocator* pAllocator = ezFoundation::GetDefaultAllocator()); // [tested]

                                                                                                                        /// \brief Copies the given substring into this one. The ezStringView might actually be a substring of this very string.
  /* implicit */ ezStringBuilder(ezStringView rhs, ezAllocator* pAllocator = ezFoundation::GetDefaultAllocator()); // [tested]

  /// \brief Copies the given string into this one.
  void operator=(const ezStringBuilder& rhs); // [tested]

  /// \brief Moves the given string into this one.
  void operator=(ezStringBuilder&& rhs) noexcept;

  /// \brief Copies the given Utf8 string into this one.
  void operator=(const char* szUTF8); // [tested]

  /// \brief Copies the given wchar_t string into this one.
  void operator=(const wchar_t* pWChar); // [tested]

  /// \brief Copies the given substring into this one. The ezStringView might actually be a substring of this very string.
  void operator=(ezStringView rhs); // [tested]

  /// \brief Copies the given string into this one.
  template <ezUInt16 Size>
  void operator=(const ezHybridStringBase<Size>& rhs)
  {
    m_Data = rhs.m_Data;
  }

  /// \brief Copies the given string into this one.
  template <ezUInt16 Size, typename A>
  void operator=(const ezHybridString<Size, A>& rhs)
  {
    m_Data = rhs.m_Data;
  }

  /// \brief Moves the given string into this one.
  template <ezUInt16 Size>
  void operator=(ezHybridStringBase<Size>&& rhs)
  {
    m_Data = std::move(rhs.m_Data);
  }

  /// \brief Moves the given string into this one.
  template <ezUInt16 Size, typename A>
  void operator=(ezHybridString<Size, A>&& rhs) noexcept
  {
    m_Data = std::move(rhs.m_Data);
  }

  /// \brief Returns the allocator that is used by this object.
  ezAllocator* GetAllocator() const;

  /// \brief Resets this string to be empty. Does not deallocate any previously allocated data, as it might be reused later again.
  void Clear(); // [tested]

  /// \brief Returns a char pointer to the internal Utf8 data.
  const char* GetData() const; // [tested]

  /// \brief Returns the number of bytes that this string takes up.
  ezUInt32 GetElementCount() const; // [tested]

  /// \brief Returns the number of characters of which this string consists. Might be less than GetElementCount, if it contains Utf8
  /// multi-byte characters.
  ///
  /// \note This is a slow operation, as it has to run through the entire string to count the Unicode characters.
  /// Only call this once and use the result as long as the string doesn't change. Don't call this in a loop.
  ezUInt32 GetCharacterCount() const; // [tested]

  /// \brief Converts all characters to upper case. Might move the string data around, so all iterators to the data will be invalid
  /// afterwards.
  void ToUpper(); // [tested]

  /// \brief Converts all characters to lower case. Might move the string data around, so all iterators to the data will be invalid
  /// afterwards.
  void ToLower(); // [tested]

  /// \brief Changes the single character in this string, to which the iterator currently points.
  ///
  /// The string might need to be moved around, if its encoding size changes, however the given iterator will be adjusted
  /// so that it will always stay valid.
  /// \note
  /// This can be a very costly operation (unless this string is pure ASCII).
  /// It is only provided for the few rare cases where it is more convenient and performance is not of concern.
  /// If possible, do not use this function, at all.
  void ChangeCharacter(iterator& ref_it, ezUInt32 uiCharacter); // [tested]

  /// \brief Sets the string to the given string.
  void Set(ezStringView sData1); // [tested]
  /// \brief Sets the string by concatenating all given strings.
  void Set(ezStringView sData1, ezStringView sData2); // [tested]
  /// \brief Sets the string by concatenating all given strings.
  void Set(ezStringView sData1, ezStringView sData2, ezStringView sData3); // [tested]
  /// \brief Sets the string by concatenating all given strings.
  void Set(ezStringView sData1, ezStringView sData2, ezStringView sData3, ezStringView sData4); // [tested]
  /// \brief Sets the string by concatenating all given strings.
  void Set(ezStringView sData1, ezStringView sData2, ezStringView sData3, ezStringView sData4, ezStringView sData5, ezStringView sData6 = {}); // [tested]

  /// \brief Sets several path pieces. Makes sure they are always properly separated by a slash.
  void SetPath(ezStringView sData1, ezStringView sData2, ezStringView sData3 = {}, ezStringView sData4 = {});

  /// \brief Copies the string starting at \a pStart up to \a pEnd (exclusive).
  void SetSubString_FromTo(const char* pStart, const char* pEnd);

  /// \brief Copies the string starting at \a pStart with a length of \a uiElementCount bytes.
  void SetSubString_ElementCount(const char* pStart, ezUInt32 uiElementCount);

  /// \brief Copies the string starting at \a pStart with a length of \a uiCharacterCount characters.
  void SetSubString_CharacterCount(const char* pStart, ezUInt32 uiCharacterCount);

  /// \brief Appends a single Utf32 character.
  void Append(ezUInt32 uiChar); // [tested]

  /// \brief Appends all the given strings at the back of this string in one operation.
  void Append(const wchar_t* pData1, const wchar_t* pData2 = nullptr, const wchar_t* pData3 = nullptr, const wchar_t* pData4 = nullptr, const wchar_t* pData5 = nullptr, const wchar_t* pData6 = nullptr); // [tested]

  /// \brief Appends all the given strings to the back of this string in one operation.
  void Append(ezStringView sData1); // [tested]
  /// \brief Appends all the given strings to the back of this string in one operation.
  void Append(ezStringView sData1, ezStringView sData2); // [tested]
  /// \brief Appends all the given strings to the back of this string in one operation.
  void Append(ezStringView sData1, ezStringView sData2, ezStringView sData3); // [tested]
  /// \brief Appends all the given strings to the back of this string in one operation.
  void Append(ezStringView sData1, ezStringView sData2, ezStringView sData3, ezStringView sData4); // [tested]
  /// \brief Appends all the given strings to the back of this string in one operation.
  void Append(ezStringView sData1, ezStringView sData2, ezStringView sData3, ezStringView sData4, ezStringView sData5, ezStringView sData6 = {}); // [tested]

  /// \brief Prepends a single Utf32 character.
  void Prepend(ezUInt32 uiChar); // [tested]

  /// \brief Prepends all the given strings to the front of this string in one operation.
  void Prepend(const wchar_t* pData1, const wchar_t* pData2 = nullptr, const wchar_t* pData3 = nullptr, const wchar_t* pData4 = nullptr,
    const wchar_t* pData5 = nullptr, const wchar_t* pData6 = nullptr); // [tested]

  /// \brief Prepends all the given strings to the front of this string in one operation.
  void Prepend(ezStringView sData1, ezStringView sData2 = {}, ezStringView sData3 = {}, ezStringView sData4 = {},
    ezStringView sData5 = {}, ezStringView sData6 = {}); // [tested]

  /// \brief Sets this string to the formatted string, uses printf-style formatting.
  void SetPrintf(const char* szUtf8Format, ...); // [tested]

  /// \brief Sets this string to the formatted string, uses printf-style formatting.
  void SetPrintfArgs(const char* szUtf8Format, va_list szArgs); // [tested]

  /// \brief Replaces this with a formatted string. Uses '{}' formatting placeholders, see ezFormatString for details.
  void SetFormat(const ezFormatString& string); // [tested]

  /// \brief Replaces this with a formatted string. Uses '{}' formatting placeholders, see ezFormatString for details.
  template <typename... ARGS>
  void SetFormat(const char* szFormat, ARGS&&... args) // [tested]
  {
    SetFormat(ezFormatStringImpl<ARGS...>(szFormat, std::forward<ARGS>(args)...));
  }

  /// \brief Appends a formatted string. Uses '{}' formatting placeholders, see ezFormatString for details.
  void AppendFormat(const ezFormatString& string); // [tested]

  /// \brief Appends a formatted string. Uses '{}' formatting placeholders, see ezFormatString for details.
  template <typename... ARGS>
  void AppendFormat(const char* szFormat, ARGS&&... args) // [tested]
  {
    AppendFormat(ezFormatStringImpl<ARGS...>(szFormat, std::forward<ARGS>(args)...));
  }

  /// \brief Prepends a formatted string. Uses '{}' formatting placeholders, see ezFormatString for details.
  void PrependFormat(const ezFormatString& string); // [tested]

  /// \brief Prepends a formatted string. Uses '{}' formatting placeholders, see ezFormatString for details.
  template <typename... ARGS>
  void PrependFormat(const char* szFormat, ARGS&&... args) // [tested]
  {
    PrependFormat(ezFormatStringImpl<ARGS...>(szFormat, std::forward<ARGS>(args)...));
  }

  /// \brief Removes the first n and last m characters from this string.
  ///
  /// This function will never reallocate data.
  /// Removing characters at the back is very cheap.
  /// Removing characters at the front needs to move data around, so can be quite costly.
  void Shrink(ezUInt32 uiShrinkCharsFront, ezUInt32 uiShrinkCharsBack); // [tested]

  /// \brief Reserves uiNumElements bytes.
  void Reserve(ezUInt32 uiNumElements); // [tested]


  /// \brief Replaces the string that starts at szStartPos and ends at szEndPos with the string szReplaceWith.
  void ReplaceSubString(const char* szStartPos, const char* szEndPos, ezStringView sReplaceWith); // [tested]

  /// \brief A wrapper around ReplaceSubString. Will insert the given string at szInsertAtPos.
  void Insert(const char* szInsertAtPos, ezStringView sTextToInsert); // [tested]

  /// \brief A wrapper around ReplaceSubString. Will remove the substring which starts at szRemoveFromPos and ends at szRemoveToPos.
  void Remove(const char* szRemoveFromPos, const char* szRemoveToPos); // [tested]

  /// \brief Replaces the first occurrence of szSearchFor by szReplacement. Optionally starts searching at szStartSearchAt (or the
  /// beginning).
  ///
  /// Returns the first position where szSearchFor was found, or nullptr if nothing was found (and replaced).
  const char* ReplaceFirst(ezStringView sSearchFor, ezStringView sReplacement, const char* szStartSearchAt = nullptr); // [tested]

  /// \brief Case-insensitive version of ReplaceFirst.
  const char* ReplaceFirst_NoCase(ezStringView sSearchFor, ezStringView sReplacement, const char* szStartSearchAt = nullptr); // [tested]

  /// \brief Replaces the last occurrence of szSearchFor by szReplacement. Optionally starts searching at szStartSearchAt (or the end).
  ///
  /// Returns the last position where szSearchFor was found, or nullptr if nothing was found (and replaced).
  const char* ReplaceLast(ezStringView sSearchFor, ezStringView sReplacement, const char* szStartSearchAt = nullptr); // [tested]

  /// \brief Case-insensitive version of ReplaceLast.
  const char* ReplaceLast_NoCase(ezStringView sSearchFor, ezStringView sReplacement, const char* szStartSearchAt = nullptr); // [tested]

  /// \brief Replaces all occurrences of szSearchFor by szReplacement. Returns the number of replacements.
  ezUInt32 ReplaceAll(ezStringView sSearchFor, ezStringView sReplacement); // [tested]

  /// \brief Case-insensitive version of ReplaceAll.
  ezUInt32 ReplaceAll_NoCase(ezStringView sSearchFor, ezStringView sReplacement); // [tested]

  /// \brief Replaces the first occurrence of szSearchFor by szReplaceWith, if szSearchFor was found to be a 'whole word', as indicated by
  /// the delimiter function IsDelimiterCB.
  ///
  /// Returns the start position of where the word was replaced or nullptr if nothing got replaced.
  const char* ReplaceWholeWord(const char* szSearchFor, ezStringView sReplaceWith, ezStringUtils::EZ_CHARACTER_FILTER isDelimiterCB); // [tested]

  /// \brief Case-insensitive version of ReplaceWholeWord.
  ///
  /// Returns the start position of where the word was replaced or nullptr if nothing got replaced.
  const char* ReplaceWholeWord_NoCase(const char* szSearchFor, ezStringView sReplaceWith, ezStringUtils::EZ_CHARACTER_FILTER isDelimiterCB); // [tested]

  /// \brief Replaces all occurrences of szSearchFor by szReplaceWith, if szSearchFor was found to be a 'whole word', as indicated by the
  /// delimiter function IsDelimiterCB.
  ///
  /// Returns how many words got replaced.
  ezUInt32 ReplaceWholeWordAll(const char* szSearchFor, ezStringView sReplaceWith, ezStringUtils::EZ_CHARACTER_FILTER isDelimiterCB); // [tested]

  /// \brief Case-insensitive version of ReplaceWholeWordAll.
  ///
  /// Returns how many words got replaced.
  ezUInt32 ReplaceWholeWordAll_NoCase(const char* szSearchFor, ezStringView sReplaceWith, ezStringUtils::EZ_CHARACTER_FILTER isDelimiterCB); // [tested]

  /// \brief Replaces the current string with the content from the stream. Reads the stream to its end.
  void ReadAll(ezStreamReader& inout_stream);

  // ******* Path Functions ********

  /// \brief Removes "../" where possible, replaces all path separators with /, removes double slashes.
  ///
  /// All paths use slashes on all platforms. If you need to convert a path to the OS specific representation, use
  /// 'MakePathSeparatorsNative' 'MakeCleanPath' will in rare circumstances grow the string by one character. That means it is quite safe to
  /// assume that it will not waste time on memory allocations. If it is repeatedly called on the same string, it has a minor overhead for
  /// computing the same string over and over, but no memory allocations will be done (everything is in-place).
  ///
  /// Removes all double path separators (slashes and backslashes) in a path, except if the path starts with two (back-)slashes, those are
  /// kept, as they might indicate a UNC path.
  void MakeCleanPath(); // [tested]

  /// \brief Modifies this string to point to the parent directory.
  ///
  /// 'uiLevelsUp' can be used to go several folders upwards. It has to be at least one.
  /// If there are no more folders to go up, "../" is appended as much as needed.
  void PathParentDirectory(ezUInt32 uiLevelsUp = 1); // [tested]

  /// \brief Appends several path pieces. Makes sure they are always properly separated by a slash.
  void AppendPath(ezStringView sPath1, ezStringView sPath2 = {}, ezStringView sPath3 = {}, ezStringView sPath4 = {}); // [tested]

  /// \brief Similar to Append() but the very first argument is a separator that is only appended (once) if the existing string is not empty and does
  /// not already end with the separator.
  ///
  /// This is useful when one wants to append entries that require a separator like a comma in between items. E.g. calling
  /// AppendWithSeparator(", ", "a", "b");
  /// AppendWithSeparator(", ", "c", "d");
  /// results in the string "ab, cd"
  void AppendWithSeparator(ezStringView sSeparator, ezStringView sText1, ezStringView sText2 = ezStringView(), ezStringView sText3 = ezStringView(), ezStringView sText4 = ezStringView(), ezStringView sText5 = ezStringView(), ezStringView sText6 = ezStringView());

  /// \brief Changes the file name part of the path, keeps the extension intact (if there is any).
  void ChangeFileName(ezStringView sNewFileName); // [tested]

  /// \brief Changes the file name and the extension part of the path.
  void ChangeFileNameAndExtension(ezStringView sNewFileNameWithExtension); // [tested]

  /// \brief Only changes the file extension of the path. If there is no extension yet, one is appended (including a dot).
  ///
  /// sNewExtension may or may not start with a dot.
  /// If sNewExtension is empty, the file extension is removed, but the dot remains.
  /// E.g. "file.txt" -> "file."
  /// If you also want to remove the dot, use RemoveFileExtension() instead.
  ///
  /// If bFullExtension is false, a file named "file.a.b.c" will replace only "c".
  /// If bFullExtension is true, a file named "file.a.b.c" will replace all of "a.b.c".
  void ChangeFileExtension(ezStringView sNewExtension, bool bFullExtension = false); // [tested]

  /// \brief If any extension exists, it is removed, including the dot before it.
  ///
  /// If bFullExtension is false, a file named "file.a.b.c" will end up as "file.a.b"
  /// If bFullExtension is true, a file named "file.a.b.c" will end up as "file"
  void RemoveFileExtension(bool bFullExtension = false); // [tested]

  /// \brief Converts this path into a relative path to the path with the awesome variable name 'szAbsolutePathToMakeThisRelativeTo'
  ///
  /// If the method succeeds the StringBuilder's contents are modified in place.
  ezResult MakeRelativeTo(ezStringView sAbsolutePathToMakeThisRelativeTo); // [tested]

  /// \brief Cleans this path up and replaces all path separators by the OS specific separator.
  ///
  /// This can be used, if you want to present paths in the OS specific form to the user in the UI.
  /// In all other cases the internal representation uses slashes, no matter on which operating system.
  void MakePathSeparatorsNative(); // [tested]

  /// \brief Checks whether this path is a sub-path of the given path.
  ///
  /// This function will call 'MakeCleanPath' to be able to compare both paths, thus it might modify the data of this instance.
  bool IsPathBelowFolder(const char* szPathToFolder); // [tested]

  /// \brief Returns the amount of bytes that are currently allocated on the heap.
  ezUInt64 GetHeapMemoryUsage() const { return m_Data.GetHeapMemoryUsage(); }

  /// \brief Removes all characters from the start and end that appear in the given strings.
  ///
  /// The default string removes all standard whitespace characters.
  void Trim(const char* szTrimChars = " \f\n\r\t\v"); // [tested]

  /// \brief Removes all characters from the start and/or end that appear in the given strings.
  void Trim(const char* szTrimCharsStart, const char* szTrimCharsEnd); // [tested]

  /// \brief Removes all characters from the start that appear in the given strings.
  ///
  /// The default string removes all standard whitespace characters.
  void TrimLeft(const char* szTrimChars = " \f\n\r\t\v");

  /// \brief Removes all characters from the end that appear in the given strings.
  ///
  /// The default string removes all standard whitespace characters.
  void TrimRight(const char* szTrimChars = " \f\n\r\t\v");

  /// \brief If the string starts with the given word (case insensitive), it is removed and the function returns true.
  bool TrimWordStart(ezStringView sWord); // [tested]

  /// \brief If the string ends with the given word (case insensitive), it is removed and the function returns true.
  bool TrimWordEnd(ezStringView sWord); // [tested]

#if EZ_ENABLED(EZ_INTEROP_STL_STRINGS)
  /// \brief Copies the given substring into this one. The ezStringView might actually be a substring of this very string.
  /* implicit */ ezStringBuilder(const std::string_view& rhs, ezAllocator* pAllocator = ezFoundation::GetDefaultAllocator());

  /// \brief Copies the given substring into this one. The ezStringView might actually be a substring of this very string.
  /* implicit */ ezStringBuilder(const std::string& rhs, ezAllocator* pAllocator = ezFoundation::GetDefaultAllocator());

  /// \brief Copies the given substring into this one. The ezStringView might actually be a substring of this very string.
  void operator=(const std::string_view& rhs);

  /// \brief Copies the given substring into this one. The ezStringView might actually be a substring of this very string.
  void operator=(const std::string& rhs);
#endif

private:
  /// \brief Will remove all double path separators (slashes and backslashes) in a path, except if the path starts with two (back-)slashes,
  /// those are kept, as they might indicate a UNC path.
  void RemoveDoubleSlashesInPath(); // [tested]

  void ChangeCharacterNonASCII(iterator& it, ezUInt32 uiCharacter);
  void AppendTerminator();

  // needed for better copy construction
  template <ezUInt16 T>
  friend struct ezHybridStringBase;

  friend ezStreamReader;

  ezHybridArray<char, 128> m_Data;
};

#include <Foundation/Strings/Implementation/StringBuilder_inl.h>
