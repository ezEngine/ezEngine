#include <PCH.h>
#include <EditorPluginAssets/TextureAsset/TextureAssetManager.h>
#include <EditorPluginAssets/TextureAsset/TextureAsset.h>
#include <EditorPluginAssets/TextureAsset/TextureAssetWindow.moc.h>
#include "ToolsFoundation/Assets/AssetFileExtensionWhitelist.h"

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezTextureAssetDocumentManager, 1, ezRTTIDefaultAllocator<ezTextureAssetDocumentManager>);
EZ_END_DYNAMIC_REFLECTED_TYPE();

ezTextureAssetDocumentManager::ezTextureAssetDocumentManager()
{
  ezDocumentManager::s_Events.AddEventHandler(ezMakeDelegate(&ezTextureAssetDocumentManager::OnDocumentManagerEvent, this));

  // additional whitelist for non-asset files where an asset may be selected
  ezAssetFileExtensionWhitelist::AddAssetFileExtension("Texture 2D", "dds");
  ezAssetFileExtensionWhitelist::AddAssetFileExtension("Texture 2D", "color");
  ezAssetFileExtensionWhitelist::AddAssetFileExtension("Texture 3D", "dds");
  ezAssetFileExtensionWhitelist::AddAssetFileExtension("Texture Cube", "dds");

  // texture asset source files
  ezAssetFileExtensionWhitelist::AddAssetFileExtension("Image2D", "dds");
  ezAssetFileExtensionWhitelist::AddAssetFileExtension("Image2D", "tga");
}

ezTextureAssetDocumentManager::~ezTextureAssetDocumentManager()
{
  ezDocumentManager::s_Events.RemoveEventHandler(ezMakeDelegate(&ezTextureAssetDocumentManager::OnDocumentManagerEvent, this));
}

void ezTextureAssetDocumentManager::OnDocumentManagerEvent(const ezDocumentManager::Event& e)
{
  switch (e.m_Type)
  {
  case ezDocumentManager::Event::Type::DocumentWindowRequested:
    {
      if (e.m_pDocument->GetDynamicRTTI() == ezGetStaticRTTI<ezTextureAssetDocument>())
      {
        ezTextureAssetDocumentWindow* pDocWnd = new ezTextureAssetDocumentWindow(e.m_pDocument);
      }
    }
    break;
  }
}

ezStatus ezTextureAssetDocumentManager::InternalCanOpenDocument(const char* szDocumentTypeName, const char* szFilePath) const
{
  return ezStatus(EZ_SUCCESS);
}

ezStatus ezTextureAssetDocumentManager::InternalCreateDocument(const char* szDocumentTypeName, const char* szPath, ezDocument*& out_pDocument)
{
  out_pDocument = new ezTextureAssetDocument(szPath);

  return ezStatus(EZ_SUCCESS);
}

void ezTextureAssetDocumentManager::InternalGetSupportedDocumentTypes(ezHybridArray<ezDocumentTypeDescriptor, 4>& out_DocumentTypes) const
{
  {
    ezDocumentTypeDescriptor td;
    td.m_bCanCreate = true;
    td.m_sDocumentTypeName = "Texture Asset";
    td.m_sFileExtensions.PushBack("ezTextureAsset");
    td.m_sIcon = ":/AssetIcons/Texture_2D.png";

    out_DocumentTypes.PushBack(td);
  }
}



