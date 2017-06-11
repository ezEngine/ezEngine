#include <PCH.h>
#include <EditorPluginAssets/DecalAsset/DecalAsset.h>
#include <EditorPluginAssets/DecalAsset/DecalAssetManager.h>

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezDecalAssetProperties, 1, ezRTTIDefaultAllocator<ezDecalAssetProperties>)
{
  EZ_BEGIN_PROPERTIES
  {
    EZ_MEMBER_PROPERTY("Diffuse", m_sDiffuse)->AddAttributes(new ezFileBrowserAttribute("Select Diffuse Map", "*.dds;*.tga;*.png;*.jpg;*.jpeg")),
    EZ_MEMBER_PROPERTY("Normal", m_sNormal)->AddAttributes(new ezFileBrowserAttribute("Select Normal Map", "*.dds;*.tga;*.png;*.jpg;*.jpeg")),

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

const char* ezDecalAssetDocument::QueryAssetType() const
{
  return "Decal";
}

ezStatus ezDecalAssetDocument::InternalTransformAsset(ezStreamWriter& stream, const char* szOutputTag, const char* szPlatform, const ezAssetFileHeader& AssetHeader, bool bTriggeredManually)
{
  return static_cast<ezDecalAssetDocumentManager*>(GetAssetDocumentManager())->GenerateDecalTexture(szPlatform);
}
