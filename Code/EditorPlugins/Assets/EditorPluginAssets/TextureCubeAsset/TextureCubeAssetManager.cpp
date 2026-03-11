#include <EditorPluginAssets/EditorPluginAssetsPCH.h>

#include "ToolsFoundation/Assets/AssetFileExtensionWhitelist.h"
#include <EditorPluginAssets/TextureCubeAsset/TextureCubeAsset.h>
#include <EditorPluginAssets/TextureCubeAsset/TextureCubeAssetManager.h>
#include <EditorPluginAssets/TextureCubeAsset/TextureCubeAssetWindow.moc.h>

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezTextureCubeAssetDocumentManager, 1, ezRTTIDefaultAllocator<ezTextureCubeAssetDocumentManager>)
EZ_END_DYNAMIC_REFLECTED_TYPE;

ezTextureCubeAssetDocumentManager::ezTextureCubeAssetDocumentManager()
{
  ezDocumentManager::s_Events.AddEventHandler(ezMakeDelegate(&ezTextureCubeAssetDocumentManager::OnDocumentManagerEvent, this));

  // additional whitelist for non-asset files where an asset may be selected
  ezAssetFileExtensionWhitelist::AddAssetFileExtension("CompatibleAsset_Texture_Cube", "dds");

  m_DocTypeDesc.m_sDocumentTypeName = "Texture Cube";
  m_DocTypeDesc.m_sFileExtension = "ezTextureCubeAsset";
  m_DocTypeDesc.m_sIcon = ":/AssetIcons/Texture_Cube.svg";
  m_DocTypeDesc.m_sAssetCategory = "Rendering";
  m_DocTypeDesc.m_pDocumentType = ezGetStaticRTTI<ezTextureCubeAssetDocument>();
  m_DocTypeDesc.m_pManager = this;
  m_DocTypeDesc.m_CompatibleTypes.PushBack("CompatibleAsset_Texture_Cube");

  m_DocTypeDesc.m_sResourceFileExtension = "ezBinTextureCube";
  m_DocTypeDesc.m_AssetDocumentFlags = ezAssetDocumentFlags::AutoThumbnailOnTransform;
}

ezTextureCubeAssetDocumentManager::~ezTextureCubeAssetDocumentManager()
{
  ezDocumentManager::s_Events.RemoveEventHandler(ezMakeDelegate(&ezTextureCubeAssetDocumentManager::OnDocumentManagerEvent, this));
}

void ezTextureCubeAssetDocumentManager::OnDocumentManagerEvent(const ezDocumentManager::Event& e)
{
  switch (e.m_Type)
  {
    case ezDocumentManager::Event::Type::DocumentWindowRequested:
    {
      if (e.m_pDocument->GetDynamicRTTI() == ezGetStaticRTTI<ezTextureCubeAssetDocument>())
      {
        new ezQtTextureCubeAssetDocumentWindow(static_cast<ezTextureCubeAssetDocument*>(e.m_pDocument)); // NOLINT: Not a memory leak
      }
    }
    break;

    default:
      break;
  }
}

void ezTextureCubeAssetDocumentManager::InternalCreateDocument(
  ezStringView sDocumentTypeName, ezStringView sPath, bool bCreateNewDocument, ezDocument*& out_pDocument, const ezDocumentObject* pOpenContext)
{
  out_pDocument = new ezTextureCubeAssetDocument(sPath);
}

void ezTextureCubeAssetDocumentManager::InternalGetSupportedDocumentTypes(ezDynamicArray<const ezDocumentTypeDescriptor*>& inout_DocumentTypes) const
{
  inout_DocumentTypes.PushBack(&m_DocTypeDesc);
}

ezUInt64 ezTextureCubeAssetDocumentManager::ComputeAssetProfileHashImpl(const ezPlatformProfile* pAssetProfile) const
{
  // don't have any settings yet, but assets that generate profile specific output must not return 0 here
  return 1;
}
