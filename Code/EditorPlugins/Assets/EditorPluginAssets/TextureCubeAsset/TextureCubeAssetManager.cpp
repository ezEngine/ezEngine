#include <EditorPluginAssetsPCH.h>

#include "ToolsFoundation/Assets/AssetFileExtensionWhitelist.h"
#include <EditorPluginAssets/TextureCubeAsset/TextureCubeAsset.h>
#include <EditorPluginAssets/TextureCubeAsset/TextureCubeAssetManager.h>
#include <EditorPluginAssets/TextureCubeAsset/TextureCubeAssetWindow.moc.h>

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezTextureCubeAssetDocumentManager, 1, ezRTTIDefaultAllocator<ezTextureCubeAssetDocumentManager>);
EZ_END_DYNAMIC_REFLECTED_TYPE;

ezTextureCubeAssetDocumentManager::ezTextureCubeAssetDocumentManager()
{
  ezDocumentManager::s_Events.AddEventHandler(ezMakeDelegate(&ezTextureCubeAssetDocumentManager::OnDocumentManagerEvent, this));

  // additional whitelist for non-asset files where an asset may be selected
  ezAssetFileExtensionWhitelist::AddAssetFileExtension("Texture Cube", "dds");

  m_AssetDesc.m_bCanCreate = true;
  m_AssetDesc.m_sDocumentTypeName = "TextureCube Asset";
  m_AssetDesc.m_sFileExtension = "ezTextureCubeAsset";
  m_AssetDesc.m_sIcon = ":/AssetIcons/Texture_Cube.png";
  m_AssetDesc.m_pDocumentType = ezGetStaticRTTI<ezTextureCubeAssetDocument>();
  m_AssetDesc.m_pManager = this;
}

ezTextureCubeAssetDocumentManager::~ezTextureCubeAssetDocumentManager()
{
  ezDocumentManager::s_Events.RemoveEventHandler(ezMakeDelegate(&ezTextureCubeAssetDocumentManager::OnDocumentManagerEvent, this));
}


ezBitflags<ezAssetDocumentFlags>
ezTextureCubeAssetDocumentManager::GetAssetDocumentTypeFlags(const ezDocumentTypeDescriptor* pDescriptor) const
{
  return ezAssetDocumentFlags::AutoThumbnailOnTransform;
}

void ezTextureCubeAssetDocumentManager::OnDocumentManagerEvent(const ezDocumentManager::Event& e)
{
  switch (e.m_Type)
  {
    case ezDocumentManager::Event::Type::DocumentWindowRequested:
    {
      if (e.m_pDocument->GetDynamicRTTI() == ezGetStaticRTTI<ezTextureCubeAssetDocument>())
      {
        ezQtTextureCubeAssetDocumentWindow* pDocWnd =
            new ezQtTextureCubeAssetDocumentWindow(static_cast<ezTextureCubeAssetDocument*>(e.m_pDocument));
      }
    }
    break;
  }
}

ezStatus ezTextureCubeAssetDocumentManager::InternalCreateDocument(const char* szDocumentTypeName, const char* szPath,
                                                                   bool bCreateNewDocument, ezDocument*& out_pDocument)
{
  out_pDocument = new ezTextureCubeAssetDocument(szPath);

  return ezStatus(EZ_SUCCESS);
}

void ezTextureCubeAssetDocumentManager::InternalGetSupportedDocumentTypes(
    ezDynamicArray<const ezDocumentTypeDescriptor*>& inout_DocumentTypes) const
{
  inout_DocumentTypes.PushBack(&m_AssetDesc);
}

ezUInt64 ezTextureCubeAssetDocumentManager::ComputeAssetProfileHashImpl(const ezPlatformProfile* pAssetProfile) const
{
  // don't have any settings yet, but assets that generate profile specific output must not return 0 here
  return 1;
}
