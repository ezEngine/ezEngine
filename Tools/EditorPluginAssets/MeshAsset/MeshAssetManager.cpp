#include <PCH.h>
#include <EditorPluginAssets/MeshAsset/MeshAssetManager.h>
#include <EditorPluginAssets/MeshAsset/MeshAsset.h>
#include <EditorPluginAssets/MeshAsset/MeshAssetWindow.moc.h>

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezMeshAssetDocumentManager, ezAssetDocumentManager, 1, ezRTTIDefaultAllocator<ezMeshAssetDocumentManager>);
EZ_END_DYNAMIC_REFLECTED_TYPE();

ezMeshAssetDocumentManager::ezMeshAssetDocumentManager()
{
  ezDocumentManagerBase::s_Events.AddEventHandler(ezMakeDelegate(&ezMeshAssetDocumentManager::OnDocumentManagerEvent, this));
}

ezMeshAssetDocumentManager::~ezMeshAssetDocumentManager()
{
  ezDocumentManagerBase::s_Events.RemoveEventHandler(ezMakeDelegate(&ezMeshAssetDocumentManager::OnDocumentManagerEvent, this));
}

void ezMeshAssetDocumentManager::OnDocumentManagerEvent(const ezDocumentManagerBase::Event& e)
{
  switch (e.m_Type)
  {
  case ezDocumentManagerBase::Event::Type::DocumentWindowRequested:
    {
      if (e.m_pDocument->GetDynamicRTTI() == ezGetStaticRTTI<ezMeshAssetDocument>())
      {
        ezMeshAssetDocumentWindow* pDocWnd = new ezMeshAssetDocumentWindow(e.m_pDocument);
      }
    }
    break;
  }
}

ezStatus ezMeshAssetDocumentManager::InternalCanOpenDocument(const char* szDocumentTypeName, const char* szFilePath) const
{
  return EZ_SUCCESS;
}

ezStatus ezMeshAssetDocumentManager::InternalCreateDocument(const char* szDocumentTypeName, const char* szPath, ezDocumentBase*& out_pDocument)
{
  out_pDocument = new ezMeshAssetDocument(szPath);

  return ezStatus(EZ_SUCCESS);
}

void ezMeshAssetDocumentManager::InternalGetSupportedDocumentTypes(ezHybridArray<ezDocumentTypeDescriptor, 4>& out_DocumentTypes) const
{
  {
    ezDocumentTypeDescriptor td;
    td.m_bCanCreate = true;
    td.m_sDocumentTypeName = "Mesh Asset";
    td.m_sFileExtensions.PushBack("ezMeshAsset");
    td.m_sIcon = ":/AssetIcons/Mesh.png";

    out_DocumentTypes.PushBack(td);
  }
}



