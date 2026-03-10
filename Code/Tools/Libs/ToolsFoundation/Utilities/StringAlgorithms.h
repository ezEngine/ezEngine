#pragma once

#include <Foundation/Strings/StringView.h>
#include <ToolsFoundation/ToolsFoundationDLL.h>

class ezStringBuilder;

namespace ezStringAlgorithms
{
  /// Computes an ordered name for an item inserted between sLeft and sRight.
  ///
  /// Uses the Dewey decimal scheme to produce a human-readable name that conveys the
  /// item's position relative to its neighbors without renaming any existing items.
  /// Integer midpoints are preferred when the gap allows (e.g. "3" between "2" and "4",
  /// "15" between "10" and "20"), falling back to dot-separated sub-levels for adjacent
  /// values (e.g. "3.5" between "3" and "4", "3.7" between "3.5" and "4").
  ///
  /// Either neighbor may be empty: an empty sLeft means prepend (goes one below sRight's
  /// root integer, potentially negative), an empty sRight means append (goes one above
  /// sLeft's root integer).
  EZ_TOOLSFOUNDATION_DLL void ComputeNameBetween(ezStringView sLeft, ezStringView sRight, ezStringBuilder& ref_sName);
} // namespace ezStringAlgorithms
