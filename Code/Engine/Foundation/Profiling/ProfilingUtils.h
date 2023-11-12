#pragma once

#include <Foundation/Basics.h>

class EZ_FOUNDATION_DLL ezProfilingUtils
{
public:
  /// \brief Captures profiling data via ezProfilingSystem::Capture and saves it to the giben file location.
  static ezResult SaveProfilingCapture(ezStringView sCapturePath);
  /// \brief Reads two profiling captures and merges them into one.
  static ezResult MergeProfilingCaptures(ezStringView sCapturePath1, ezStringView sCapturePath2, ezStringView sMergedCapturePath);
};
