#pragma once

#include <Foundation/Containers/Map.h>
#include <Foundation/Containers/Set.h>
#include <Foundation/Strings/String.h>
#include <ToolsFoundation/ToolsFoundationDLL.h>

/// \brief A global whitelist for file extension that may be used as certain asset types
///
/// UI elements etc. may use this whitelist to detect whether a selected file is a valid candidate for an asset slot
class EZ_TOOLSFOUNDATION_DLL ezAssetFileExtensionWhitelist
{
public:
  static void AddAssetFileExtension(const char* szAssetType, const char* szAllowedFileExtension);

  static bool IsFileOnAssetWhitelist(const char* szAssetType, const char* szFile);

  static const ezSet<ezString>& GetAssetFileExtensions(const char* szAssetType);

private:
  static ezMap<ezString, ezSet<ezString>> s_ExtensionWhitelist;
};
