#pragma once

#include <Foundation/Strings/String.h>
#include <ToolsFoundation/ToolsFoundationDLL.h>

/// \brief Describes a single path pattern for filtering file paths.
///
/// A path pattern is something like "*.jpg", "SubFolder/*" or "*/temp/*".
/// It may start or end with a * indicating that it matches paths that start with, end with, or contain the pattern.
/// If no * is present, the pattern has to match exactly.
struct EZ_TOOLSFOUNDATION_DLL ezPathPattern
{
  enum MatchType : ezUInt8
  {
    Exact,
    StartsWith,
    EndsWith,
    Contains
  };

  MatchType m_MatchType = MatchType::Exact;
  ezString m_sString;

  /// \brief Sets up the pattern from the given text. Whitespace is trimmed.
  void Configure(const ezStringView sText);

  /// \brief Returns true if the given text matches this path pattern.
  bool Matches(const ezStringView sText) const;
};

/// \brief A collection of ezPathPatterns for include/exclude filtering of file paths.
struct EZ_TOOLSFOUNDATION_DLL ezPathPatternFilter
{
  ezDynamicArray<ezPathPattern> m_ExcludePatterns;
  ezDynamicArray<ezPathPattern> m_IncludePatterns;

  /// \brief Reads all patterns from the given file.
  ///
  /// The file is parsed with an ezPreprocessor, so may contain #include statements and such.
  /// Custom preprocessor definitions can be provided.
  ///
  /// After preprocessing, every line represents a single pattern.
  /// Lines that contain '[INCLUDE]' or '[EXCLUDE]' are special and change whether the
  /// following lines are considered as include patterns or exclude patterns.
  ezResult ReadConfigFile(ezStringView sFile, const ezDynamicArray<ezString>& preprocessorDefines);

  /// \brief Adds a pattern as either an include or exclude filter.
  void AddFilter(ezStringView sText, bool bIncludeFilter);

  /// \brief Determines whether the given text matches the filter patterns.
  ///
  /// Include patterns take precedence over exclude patterns.
  /// If the text matches any include pattern, it passes the filter.
  /// Otherwise, if it matches any exclude pattern, it does not pass the filter.
  /// Otherwise, if it doesn't match any include or exclude pattern, it passes the filter, since it isn't explicitely ruled out.
  bool PassesFilters(ezStringView sText, ezStringBuilder* pMatchingFilter = nullptr) const;
};
