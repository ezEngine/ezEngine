#include <ToolsFoundationPCH.h>

#include <ToolsFoundation/Assets/AssetFileExtensionWhitelist.h>

ezMap<ezString, ezSet<ezString>> ezAssetFileExtensionWhitelist::s_ExtensionWhitelist;

void ezAssetFileExtensionWhitelist::AddAssetFileExtension(const char* szAssetType, const char* szAllowedFileExtension)
{
  ezStringBuilder sLowerType = szAssetType;
  sLowerType.ToLower();

  ezStringBuilder sLowerExt = szAllowedFileExtension;
  sLowerExt.ToLower();

  s_ExtensionWhitelist[sLowerType].Insert(sLowerExt);
}


bool ezAssetFileExtensionWhitelist::IsFileOnAssetWhitelist(const char* szAssetType, const char* szFile)
{
  ezStringBuilder sLowerExt = ezPathUtils::GetFileExtension(szFile);
  sLowerExt.ToLower();

  ezStringBuilder sLowerType = szAssetType;
  sLowerType.ToLower();

  ezHybridArray<ezString, 16> Types;
  sLowerType.Split(false, Types, ";");

  for (const auto& filter : Types)
  {
    if (s_ExtensionWhitelist[filter].Contains(sLowerExt))
      return true;
  }

  return false;
}

const ezSet<ezString>& ezAssetFileExtensionWhitelist::GetAssetFileExtensions(const char* szAssetType)
{
  return s_ExtensionWhitelist[szAssetType];
}
