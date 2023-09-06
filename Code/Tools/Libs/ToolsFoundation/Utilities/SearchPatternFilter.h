#pragma once

#include <Foundation/Strings/String.h>
#include <ToolsFoundation/ToolsFoundationDLL.h>

/// \brief
class EZ_TOOLSFOUNDATION_DLL ezSearchPatternFilter
{
public:
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
