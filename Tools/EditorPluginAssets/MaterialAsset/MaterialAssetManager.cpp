#include <PCH.h>
#include <EditorPluginAssets/MaterialAsset/MaterialAssetManager.h>
#include <EditorPluginAssets/MaterialAsset/MaterialAsset.h>
#include <EditorPluginAssets/MaterialAsset/MaterialAssetWindow.moc.h>

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezMaterialAssetDocumentManager, ezAssetDocumentManager, 1, ezRTTIDefaultAllocator<ezMaterialAssetDocumentManager>);
EZ_END_DYNAMIC_REFLECTED_TYPE();

ezMaterialAssetDocumentManager::ezMaterialAssetDocumentManager()
{
  ezDocumentManagerBase::s_Events.AddEventHandler(ezMakeDelegate(&ezMaterialAssetDocumentManager::OnDocumentManagerEvent, this));
}

ezMaterialAssetDocumentManager::~ezMaterialAssetDocumentManager()
{
  ezDocumentManagerBase::s_Events.RemoveEventHandler(ezMakeDelegate(&ezMaterialAssetDocumentManager::OnDocumentManagerEvent, this));
}

void ezMaterialAssetDocumentManager::OnDocumentManagerEvent(const ezDocumentManagerBase::Event& e)
{
  switch (e.m_Type)
  {
  case ezDocumentManagerBase::Event::Type::DocumentWindowRequested:
    {
      if (e.m_pDocument->GetDynamicRTTI() == ezGetStaticRTTI<ezMaterialAssetDocument>())
      {
        ezMaterialAssetDocumentWindow* pDocWnd = new ezMaterialAssetDocumentWindow(e.m_pDocument);
      }
    }
    break;
  }
}

ezStatus ezMaterialAssetDocumentManager::InternalCanOpenDocument(const char* szDocumentTypeName, const char* szFilePath) const
{
  return EZ_SUCCESS;
}

ezStatus ezMaterialAssetDocumentManager::InternalCreateDocument(const char* szDocumentTypeName, const char* szPath, ezDocumentBase*& out_pDocument)
{
  out_pDocument = new ezMaterialAssetDocument(szPath);

  return ezStatus(EZ_SUCCESS);
}

void ezMaterialAssetDocumentManager::InternalGetSupportedDocumentTypes(ezHybridArray<ezDocumentTypeDescriptor, 4>& out_DocumentTypes) const
{
  {
    ezDocumentTypeDescriptor td;
    td.m_bCanCreate = true;
    td.m_sDocumentTypeName = "Material Asset";
    td.m_sFileExtensions.PushBack("ezMaterialAsset");
    td.m_sIcon = ":/AssetIcons/Material.png";

    out_DocumentTypes.PushBack(td);
  }
}



