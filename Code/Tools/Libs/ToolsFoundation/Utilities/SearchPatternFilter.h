#pragma once

#include <Foundation/Strings/String.h>
#include <ToolsFoundation/ToolsFoundationDLL.h>

/// \brief A helper class to implement a multi-part, case-insensitive search pattern filter with support for exclusions.
///
/// The search text is split into multiple parts by spaces. A text passes the filter if it contains all parts.
/// The check is always case insensitive and the order of the parts does not matter.
/// It is also possible to exclude parts by prefixing them with a minus.
/// E.g. "com mesh" would pass all texts that contain "com" and "mesh" like ezMeshComponent.
/// "com -mesh" would pass ezLightComponent but would fail ezMeshComponent.
class EZ_TOOLSFOUNDATION_DLL ezSearchPatternFilter
{
public:
  /// \brief Sets the search text and splits it into its part for faster checks.
  void SetSearchText(ezStringView sSearchText);

  /// \brief Returns the current search text.
  const ezString& GetSearchText() const { return m_sSearchText; }
  /// \brief Returns true if the search text is empty.
  bool IsEmpty() const { return m_sSearchText.IsEmpty(); }

  /// \brief Returns true if the filter contains any exclusion patterns.
  bool ContainsExclusions() const;

  /// \brief Determines whether the given text matches the filter patterns.
  bool PassesFilters(ezStringView sText) const;

private:
  ezString m_sSearchText;

  struct Part
  {
    ezStringView m_sPart;
    bool m_bExclude = false;
  };

  ezHybridArray<Part, 4> m_Parts;
};
