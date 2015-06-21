#include <PCH.h>
#include <EditorPluginAssets/TextureAsset/TextureAssetManager.h>
#include <EditorPluginAssets/TextureAsset/TextureAsset.h>
#include <EditorPluginAssets/TextureAsset/TextureAssetWindow.moc.h>

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezTextureAssetDocumentManager, ezAssetDocumentManager, 1, ezRTTIDefaultAllocator<ezTextureAssetDocumentManager>);
EZ_END_DYNAMIC_REFLECTED_TYPE();

ezTextureAssetDocumentManager::ezTextureAssetDocumentManager()
{
  ezDocumentManagerBase::s_Events.AddEventHandler(ezMakeDelegate(&ezTextureAssetDocumentManager::OnDocumentManagerEvent, this));
}

ezTextureAssetDocumentManager::~ezTextureAssetDocumentManager()
{
  ezDocumentManagerBase::s_Events.RemoveEventHandler(ezMakeDelegate(&ezTextureAssetDocumentManager::OnDocumentManagerEvent, this));
}

void ezTextureAssetDocumentManager::OnDocumentManagerEvent(const ezDocumentManagerBase::Event& e)
{
  switch (e.m_Type)
  {
  case ezDocumentManagerBase::Event::Type::DocumentWindowRequested:
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
  return EZ_SUCCESS;
}

ezStatus ezTextureAssetDocumentManager::InternalCreateDocument(const char* szDocumentTypeName, const char* szPath, ezDocumentBase*& out_pDocument)
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



