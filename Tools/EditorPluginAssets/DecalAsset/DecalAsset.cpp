#include <PCH.h>
#include <EditorPluginAssets/DecalAsset/DecalAsset.h>
#include <EditorPluginAssets/DecalAsset/DecalAssetManager.h>

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezDecalAssetProperties, 1, ezRTTIDefaultAllocator<ezDecalAssetProperties>)
{
  EZ_BEGIN_PROPERTIES
  {
    EZ_MEMBER_PROPERTY("Diffuse", m_sDiffuse),
    EZ_MEMBER_PROPERTY("Normal", m_sNormal),

  }
  EZ_END_PROPERTIES
}
EZ_END_DYNAMIC_REFLECTED_TYPE


EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezDecalAssetDocument, 1, ezRTTINoAllocator);
EZ_END_DYNAMIC_REFLECTED_TYPE

ezDecalAssetDocument::ezDecalAssetDocument(const char* szDocumentPath)
  : ezSimpleAssetDocument<ezDecalAssetProperties>(szDocumentPath, false)
{
}

ezStatus ezDecalAssetDocument::InternalTransformAsset(const char* szTargetFile, const char* szOutputTag, const char* szPlatform, const ezAssetFileHeader& AssetHeader, bool bTriggeredManually)
{
  EZ_ASSERT_DEV(ezStringUtils::IsEqual(szPlatform, "PC"), "Platform '{0}' is not supported", szPlatform);
  const bool bUpdateThumbnail = ezStringUtils::IsEqual(szPlatform, "PC");


  //ezFileStats stat;
  //if (ezOSFile::GetFileStats(szTargetFile, stat).Succeeded() && stat.m_uiFileSize == 0)
  //{
  //  // if the file was touched, but nothing written to it, delete the file
  //  // might happen if TexConv crashed or had an error
  //  ezOSFile::DeleteFile(szTargetFile);
  //  result.m_Result = EZ_FAILURE;
  //}

  //return result;
  return ezStatus(EZ_SUCCESS);
}

const char* ezDecalAssetDocument::QueryAssetType() const
{
  return "Decal";
}
