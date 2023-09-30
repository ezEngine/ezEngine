#include <ToolsFoundation/ToolsFoundationPCH.h>

#include <ToolsFoundation/Assets/AssetFileExtensionWhitelist.h>

ezMap<ezString, ezSet<ezString>> ezAssetFileExtensionWhitelist::s_ExtensionWhitelist;

void ezAssetFileExtensionWhitelist::AddAssetFileExtension(ezStringView sAssetType, ezStringView sAllowedFileExtension)
{
  ezStringBuilder sLowerType = sAssetType;
  sLowerType.ToLower();

  ezStringBuilder sLowerExt = sAllowedFileExtension;
  sLowerExt.ToLower();

  s_ExtensionWhitelist[sLowerType].Insert(sLowerExt);
}


bool ezAssetFileExtensionWhitelist::IsFileOnAssetWhitelist(ezStringView sAssetType, ezStringView sFile)
{
  ezStringBuilder sLowerExt = ezPathUtils::GetFileExtension(sFile);
  sLowerExt.ToLower();

  ezStringBuilder sLowerType = sAssetType;
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

const ezSet<ezString>& ezAssetFileExtensionWhitelist::GetAssetFileExtensions(ezStringView sAssetType)
{
  return s_ExtensionWhitelist[sAssetType];
}
