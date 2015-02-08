#pragma once

#include <ThirdParty/utf8/utf8.h>
#include <Foundation/Containers/HybridArray.h>
#include <Foundation/Memory/MemoryUtils.h>
#include <Foundation/Strings/StringUtils.h>
#include <Foundation/Strings/Implementation/StringBase.h>
#include <Foundation/Strings/StringView.h>
#include <Foundation/Strings/PathUtils.h>

template <ezUInt16 Size>
class ezHybridStringBase;

template <ezUInt16 Size, typename AllocatorWrapper>
class ezHybridString;

class ezStreamReaderBase;

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
/// For very convenient string creation, printf functionality is also available via the 'Format', 'AppendFormat' 
/// and 'PrependFormat' functions.
/// Once a string is built and should only be stored for read access, it should be stored in an ezString instance.
class EZ_FOUNDATION_DLL ezStringBuilder : public ezStringBase<ezStringBuilder>
{
public:

  /// \brief Initializes the string to be empty. No data is allocated, but the ezStringBuilder ALWAYS creates an array on the stack.
  ezStringBuilder(ezAllocatorBase* pAllocator = ezFoundation::GetDefaultAllocator()); // [tested]

  /// \brief Copies the given string into this one.
  ezStringBuilder(const ezStringBuilder& rhs); // [tested]

  /// \brief Moves the given string into this one.
  ezStringBuilder(ezStringBuilder&& rhs);

  /// \brief Copies the given string into this one.
  template <ezUInt16 Size>
  ezStringBuilder(const ezHybridStringBase<Size>& rhs);

  /// \brief Copies the given string into this one.
  template <ezUInt16 Size, typename A>
  ezStringBuilder(const ezHybridString<Size, A>& rhs);

  /// \brief Moves the given string into this one.
  template <ezUInt16 Size>
  ezStringBuilder(ezHybridStringBase<Size>&& rhs);

  /// \brief Moves the given string into this one.
  template <ezUInt16 Size, typename A>
  ezStringBuilder(ezHybridString<Size, A>&& rhs);

  /// \brief Constructor that appends all the given strings.
  ezStringBuilder(const char* pData1, const char* pData2, const char* pData3 = nullptr, const char* pData4 = nullptr, const char* pData5 = nullptr, const char* pData6 = nullptr); // [tested]

  /// \brief Copies the given Utf8 string into this one.
  /* implicit */ ezStringBuilder(const char* szUTF8, ezAllocatorBase* pAllocator = ezFoundation::GetDefaultAllocator()); // [tested]

  /// \brief Copies the given wchar_t string into this one.
  /* implicit */ ezStringBuilder(const wchar_t* szWChar, ezAllocatorBase* pAllocator = ezFoundation::GetDefaultAllocator()); // [tested]

  /// \brief Copies the given substring into this one. The ezStringView might actually be a substring of this very string.
  /* implicit */ ezStringBuilder(const ezStringView& rhs, ezAllocatorBase* pAllocator = ezFoundation::GetDefaultAllocator()); // [tested]

  /// \brief Copies the given string into this one.
  void operator=(const ezStringBuilder& rhs); // [tested]

  /// \brief Moves the given string into this one.
  void operator=(ezStringBuilder&& rhs);

  /// \brief Copies the given Utf8 string into this one.
  void operator=(const char* szUTF8); // [tested]

  /// \brief Copies the given wchar_t string into this one.
  void operator=(const wchar_t* szWChar); // [tested]

  /// \brief Copies the given substring into this one. The ezStringView might actually be a substring of this very string.
  void operator=(const ezStringView& rhs); // [tested]

  /// \brief Copies the given string into this one.
  template <ezUInt16 Size>
  void operator=(const ezHybridStringBase<Size>& rhs);

  /// \brief Copies the given string into this one.
  template <ezUInt16 Size, typename A>
  void operator=(const ezHybridString<Size, A>& rhs);

  /// \brief Moves the given string into this one.
  template <ezUInt16 Size>
  void operator=(ezHybridStringBase<Size>&& rhs);

  /// \brief Moves the given string into this one.
  template <ezUInt16 Size, typename A>
  void operator=(ezHybridString<Size, A>&& rhs);

  /// \brief Returns the allocator that is used by this object.
  ezAllocatorBase* GetAllocator() const;

  /// \brief Returns a string view to this string's data.
  operator ezStringView() const; // [tested]

  /// \brief Returns a pointer to the internal Utf8 string.
  operator const char*() const { return GetData(); }

  /// \brief Resets this string to be empty. Does not deallocate any previously allocated data, as it might be reused later again.
  void Clear(); // [tested]

  /// \brief Returns a char pointer to the internal Utf8 data.
  const char* GetData() const; // [tested]

  /// \brief Returns the number of bytes that this string takes up.
  ezUInt32 GetElementCount() const; // [tested]

  /// \brief Returns the number of characters of which this string consists. Might be less than GetElementCount, if it contains Utf8 multi-byte characters.
  ezUInt32 GetCharacterCount() const; // [tested]

  /// \brief Returns whether this string only contains ASCII characters, which means that GetElementCount() == GetCharacterCount()
  bool IsPureASCII() const; // [tested]

  /// \brief Returns an iterator to the entire string, which starts at the first character.
  ezStringView GetIteratorFront() const; // [tested]

  /// \brief Returns an iterator to the entire string, which starts at the last character.
  ezStringView GetIteratorBack() const; // [tested]

  /// \brief Converts all characters to upper case. Might move the string data around, so all iterators to the data will be invalid afterwards.
  void ToUpper(); // [tested]

  /// \brief Converts all characters to lower case. Might move the string data around, so all iterators to the data will be invalid afterwards.
  void ToLower(); // [tested]

  /// \brief Changes the single character in this string, to which the iterator currently points.
  ///
  /// The string might need to be moved around, if its encoding size changes, however the given iterator will be adjusted
  /// so that it will always stay valid.
  /// \note
  /// This can be a very costly operation (unless this string is pure ASCII).
  /// It is only provided for the few rare cases where it is more convenient and performance is not of concern.
  /// If possible, do not use this function, at all.
  void ChangeCharacter(ezStringView& Pos, ezUInt32 uiCharacter); // [tested]

  /// \brief Sets the string by concatenating all given strings.
  void Set(const char* pData1, const char* pData2 = nullptr, const char* pData3 = nullptr, const char* pData4 = nullptr, const char* pData5 = nullptr, const char* pData6 = nullptr);

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

  /// \brief Appends all the given strings at the back of this string in one operation.
  void Append(const char* pData1, const char* pData2 = nullptr, const char* pData3 = nullptr, const char* pData4 = nullptr, const char* pData5 = nullptr, const char* pData6 = nullptr); // [tested]

  /// \brief Appends the formatted string.
  void AppendFormat(const char* szUtf8Format, ...); // [tested]

  /// \brief Appends the formatted string.
  void AppendFormatArgs(const char* szUtf8Format, va_list args); // [tested]

  /// \brief Prepends a single Utf32 character.
  void Prepend(ezUInt32 uiChar); // [tested]

  /// \brief Prepends all the given strings to the front of this string in one operation.
  void Prepend(const wchar_t* pData1, const wchar_t* pData2 = nullptr, const wchar_t* pData3 = nullptr, const wchar_t* pData4 = nullptr, const wchar_t* pData5 = nullptr, const wchar_t* pData6 = nullptr); // [tested]

  /// \brief Prepends all the given strings to the front of this string in one operation.
  void Prepend(const char* pData1, const char* pData2 = nullptr, const char* pData3 = nullptr, const char* pData4 = nullptr, const char* pData5 = nullptr, const char* pData6 = nullptr); // [tested]

  /// \brief Prepends the formatted string.
  void PrependFormat(const char* szUtf8Format, ...); // [tested]

  /// \brief Prepends the formatted string.
  void PrependFormatArgs(const char* szUtf8Format, va_list args); // [tested]

  /// \brief Sets this string to the formatted string.
  void Format(const char* szUtf8Format, ...); // [tested]

  /// \brief Sets this string to the formatted string.
  void FormatArgs(const char* szUtf8Format, va_list args); // [tested]

  /// \brief Removes the first n and last m characters from this string.
  ///
  /// This function will never reallocate data.
  /// Removing characters at the back is very cheap.
  /// Removing characters at the front needs to move data around, so can be quite costly.
  void Shrink(ezUInt32 uiShrinkCharsFront, ezUInt32 uiShrinkCharsBack); // [tested]
  
  /// \brief Reserves uiNumElements bytes.
  void Reserve(ezUInt32 uiNumElements); // [tested]


  /// \brief Replaces the string that starts at szStartPos and ends at szEndPos with the string szReplaceWith.
  void ReplaceSubString(const char* szStartPos, const char* szEndPos, const ezStringView& szReplaceWith); // [tested]

  /// \brief A wrapper around ReplaceSubString. Will insert the given string at szInsertAtPos.
  void Insert(const char* szInsertAtPos, const ezStringView& szTextToInsert); // [tested]

  /// \brief A wrapper around ReplaceSubString. Will remove the substring which starts at szRemoveFromPos and ends at szRemoveToPos.
  void Remove(const char* szRemoveFromPos, const char* szRemoveToPos); // [tested]

  /// \brief Replaces the first occurrence of szSearchFor by szReplacement. Optionally starts searching at szStartSearchAt (or the beginning).
  ///
  /// Returns the first position where szSearchFor was found, or nullptr if nothing was found (and replaced).
  const char* ReplaceFirst(const char* szSearchFor, const ezStringView& szReplacement, const char* szStartSearchAt = nullptr); // [tested]

  /// \brief Case-insensitive version of ReplaceFirst.
  const char* ReplaceFirst_NoCase(const char* szSearchFor, const ezStringView& szReplacement, const char* szStartSearchAt = nullptr); // [tested]

  /// \brief Replaces the last occurrence of szSearchFor by szReplacement. Optionally starts searching at szStartSearchAt (or the end).
  ///
  /// Returns the last position where szSearchFor was found, or nullptr if nothing was found (and replaced).
  const char* ReplaceLast(const char* szSearchFor, const ezStringView& szReplacement, const char* szStartSearchAt = nullptr); // [tested]

  /// \brief Case-insensitive version of ReplaceLast.
  const char* ReplaceLast_NoCase(const char* szSearchFor, const ezStringView& szReplacement, const char* szStartSearchAt = nullptr); // [tested]

  /// \brief Replaces all occurrences of szSearchFor by szReplacement. Returns the number of replacements.
  ezUInt32 ReplaceAll(const char* szSearchFor, const ezStringView& szReplacement); // [tested]

  /// \brief Case-insensitive version of ReplaceAll.
  ezUInt32 ReplaceAll_NoCase(const char* szSearchFor, const ezStringView& szReplacement); // [tested]

  /// \brief Replaces the first occurrence of szSearchFor by szReplaceWith, if szSearchFor was found to be a 'whole word', as indicated by the delimiter function IsDelimiterCB.
  const char* ReplaceWholeWord(const char* szSearchFor, const ezStringView& szReplaceWith, ezStringUtils::EZ_CHARACTER_FILTER IsDelimiterCB); // [tested]

  /// \brief Case-insensitive version of ReplaceWholeWord.
  const char* ReplaceWholeWord_NoCase(const char* szSearchFor, const ezStringView& szReplaceWith, ezStringUtils::EZ_CHARACTER_FILTER IsDelimiterCB); // [tested]

  /// \brief Replaces all occurrences of szSearchFor by szReplaceWith, if szSearchFor was found to be a 'whole word', as indicated by the delimiter function IsDelimiterCB.
  ezUInt32 ReplaceWholeWordAll(const char* szSearchFor, const ezStringView& szReplaceWith, ezStringUtils::EZ_CHARACTER_FILTER IsDelimiterCB); // [tested]

  /// \brief Case-insensitive version of ReplaceWholeWordAll.
  ezUInt32 ReplaceWholeWordAll_NoCase(const char* szSearchFor, const ezStringView& szReplaceWith, ezStringUtils::EZ_CHARACTER_FILTER IsDelimiterCB); // [tested]

  /// \brief Fills the given container with ezStringView's which represent each found substring.
  /// If bReturnEmptyStrings is true, even empty strings between separators are returned.
  /// Output must be a container that stores ezStringView's and provides the functions 'Clear' and 'Append'.
  /// szSeparator1 to szSeparator6 are strings which act as separators and indicate where to split the string.
  /// This string itself will not be modified.
  template <typename Container>
  void Split(bool bReturnEmptyStrings, Container& Output, const char* szSeparator1, const char* szSeparator2 = nullptr, const char* szSeparator3 = nullptr, const char* szSeparator4 = nullptr, const char* szSeparator5 = nullptr, const char* szSeparator6 = nullptr) const; // [tested]

  /// \brief Replaces the current string with the content from the stream. Reads the stream to its end.
  void ReadAll(ezStreamReaderBase& Stream);

  // ******* Path Functions ********

  /// \brief Checks whether the given path has any file extension
  bool HasAnyExtension() const; // [tested]

  /// \brief Checks whether the given path ends with the given extension. szExtension should start with a '.' for performance reasons, but it will work without a '.' too.
  bool HasExtension(const char* szExtension) const; // [tested]

  /// \brief Returns the file extension of the given path. Will be empty, if the path does not end with a proper extension.
  ezStringView GetFileExtension() const; // [tested]

  /// \brief Returns the file name of a path, excluding the path and extension.
  ///
  /// If the path already ends with a path separator, the result will be empty.
  ezStringView GetFileName() const; // [tested]

  /// \brief Returns the substring that represents the file name including the file extension.
  ///
  /// Returns an empty string, if sPath already ends in a path separator, or is empty itself.
  ezStringView GetFileNameAndExtension() const; // [tested]

  /// \brief Returns the directory of the given file, which is the substring up to the last path separator.
  ///
  /// If the path already ends in a path separator, and thus points to a folder, instead of a file, the unchanged path is returned.
  /// "path/to/file" -> "path/to/"
  /// "path/to/folder/" -> "path/to/folder/"
  /// "filename" -> ""
  /// "/file_at_root_level" -> "/"
  ezStringView GetFileDirectory() const; // [tested]

  /// \brief Returns true, if the given path represents an absolute path on the current OS.
  bool IsAbsolutePath() const; // [tested]

  /// \brief Returns true, if the given path represents a relative path on the current OS.
  bool IsRelativePath() const; // [tested]




  /// \brief Removes "../" where possible, replaces all path separators with /, removes double slashes.
  ///
  /// All paths use slashes on all platforms. If you need to convert a path to the OS specific representation, use 'MakePathSeparatorsNative'
  /// 'MakeCleanPath' will in rare circumstances grow the string by one character. That means it is quite safe to assume that
  /// it will not waste time on memory allocations.
  /// If it is repeatedly called on the same string, it has a minor overhead for computing the same string over and over, 
  /// but no memory allocations will be done (everything is in-place).
  ///
  /// Removes all double path separators (slashes and backslashes) in a path, except if the path starts with two (back-)slashes, those are kept, as they might indicate a UNC path.
  void MakeCleanPath(); // [tested]

  /// \brief Modifies this string to point to the parent directory.
  ///
  /// 'uiLevelsUp' can be used to go several folders upwards. It has to be at least one.
  /// If there are no more folders to go up, "../" is appended as much as needed.
  void PathParentDirectory(ezUInt32 uiLevelsUp = 1); // [tested]

  /// \brief Appends several path pieces. Makes sure they are always properly separated by a slash.
  ///
  /// Will call 'MakeCleanPath' internally, so the representation of the path might change.
  void AppendPath(const char* szPath1, const char* szPath2 = nullptr, const char* szPath3 = nullptr, const char* szPath4 = nullptr); // [tested]

  /// \brief Changes the file name part of the path, keeps the extension intact (if there is any).
  void ChangeFileName(const char* szNewFileName); // [tested]

  /// \brief Changes the file name and the extension part of the path.
  void ChangeFileNameAndExtension(const char* szNewFileNameWithExtension); // [tested]

  /// \brief Only changes the file extension of the path. If there is no extension yet, one is appended.
  ///
  /// szNewExtension must not start with a dot.
  void ChangeFileExtension(const char* szNewExtension); // [tested]

  /// \brief If any extension exists, it is removed, including the dot before it.
  void RemoveFileExtension(); // [tested]

  /// \brief Converts this path into a relative path to the path with the awesome variable name 'szAbsolutePathToMakeThisRelativeTo'
  void MakeRelativeTo(const char* szAbsolutePathToMakeThisRelativeTo); // [tested]

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

private:
  /// \brief Will remove all double path separators (slashes and backslashes) in a path, except if the path starts with two (back-)slashes, those are kept, as they might indicate a UNC path.
  void RemoveDoubleSlashesInPath(); // [tested]

  void ChangeCharacterNonASCII(ezStringView& Pos, ezUInt32 uiCharacter);
  void AppendTerminator();

  // needed for better copy construction
  template<ezUInt16 T>
  friend class ezHybridStringBase;

  ezUInt32 m_uiCharacterCount;
  ezHybridArray<char, 256> m_Data;
};

#include <Foundation/Strings/Implementation/StringBuilder_inl.h>

