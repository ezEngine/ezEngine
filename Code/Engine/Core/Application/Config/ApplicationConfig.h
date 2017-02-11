#pragma once

#include <Core/Basics.h>
#include <Foundation/Strings/String.h>
#include <Foundation/Reflection/Reflection.h>

/// \todo Put into ezApplication ?

class EZ_CORE_DLL ezApplicationConfig
{
public:

  static ezResult DetectSdkRootDirectory();

  static void SetSdkRootDirectory(const char* szSdkDir);
  static const char* GetSdkRootDirectory();

  static void SetProjectDirectory(const char* szProjectDir);
  static const char* GetProjectDirectory();

  /// \brief Returns the absolute path to directory.
  ///
  /// If \a szDirectory starts with ':sdk/" the path will be relative to the Sdk root directory.
  /// If \a szDirectory starts with ':project/' the path will be relative to the project directory.
  static ezResult GetSpecialDirectory(const char* szDirectory, ezStringBuilder& out_Path);

private:
  static ezString s_sSdkRootDir;
  static ezString s_sProjectDir;
};

EZ_DECLARE_REFLECTABLE_TYPE(EZ_CORE_DLL, ezApplicationConfig);
