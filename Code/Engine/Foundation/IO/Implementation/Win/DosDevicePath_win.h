#pragma once

#include <Foundation/FoundationInternal.h>
EZ_FOUNDATION_INTERNAL_HEADER

#include <Foundation/Strings/StringBuilder.h>
#include <Foundation/Strings/StringConversion.h>

/// \brief Converts an absolute path to a 'DOS device path'
///
/// https://docs.microsoft.com/en-us/dotnet/standard/io/file-path-formats#dos-device-paths
///
/// This is necessary to support very long file paths, ie. more than 260 characters.
class ezDosDevicePath
{
public:
  ezDosDevicePath(const char* szPath)
  {
    ezStringBuilder tmp("\\\\?\\", szPath);
    tmp.ReplaceAll("/", "\\");
    m_Data = tmp.GetData();
  }

  const wchar_t* GetData() const
  {
    return m_Data.GetData();
  }

  operator const wchar_t*() const
  {
    return m_Data.GetData();
  }

  ezStringWChar m_Data;
};
