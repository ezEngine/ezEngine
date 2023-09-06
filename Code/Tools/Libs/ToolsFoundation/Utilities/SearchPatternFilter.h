#pragma once

#include <Foundation/Strings/String.h>
#include <ToolsFoundation/ToolsFoundationDLL.h>

/// \brief A small helper class to implement a simple search pattern filter that can contain multiple parts.
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

  const ezString& GetSearchText() const { return m_sSearchText; }
  bool IsEmpty() const { return m_sSearchText.IsEmpty(); }

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
