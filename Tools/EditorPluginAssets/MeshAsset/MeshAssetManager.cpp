#include <PCH.h>
#include <EditorPluginAssets/MeshAsset/MeshAssetManager.h>
#include <EditorPluginAssets/MeshAsset/MeshAsset.h>
#include <EditorPluginAssets/MeshAsset/MeshAssetWindow.moc.h>
#include "ToolsFoundation/Assets/AssetFileExtensionWhitelist.h"

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezMeshAssetDocumentManager, 1, ezRTTIDefaultAllocator<ezMeshAssetDocumentManager>);
EZ_END_DYNAMIC_REFLECTED_TYPE

ezMeshAssetDocumentManager::ezMeshAssetDocumentManager()
{
  ezDocumentManager::s_Events.AddEventHandler(ezMakeDelegate(&ezMeshAssetDocumentManager::OnDocumentManagerEvent, this));

  // additional whitelist for non-asset files where an asset may be selected
  ezAssetFileExtensionWhitelist::AddAssetFileExtension("Mesh", "ezMesh");

  m_AssetDesc.m_bCanCreate = true;
  m_AssetDesc.m_sDocumentTypeName = "Mesh Asset";
  m_AssetDesc.m_sFileExtension = "ezMeshAsset";
  m_AssetDesc.m_sIcon = ":/AssetIcons/Mesh.png";
  m_AssetDesc.m_pDocumentType = ezGetStaticRTTI<ezMeshAssetDocument>();
  m_AssetDesc.m_pManager = this;
}

ezMeshAssetDocumentManager::~ezMeshAssetDocumentManager()
{
  ezDocumentManager::s_Events.RemoveEventHandler(ezMakeDelegate(&ezMeshAssetDocumentManager::OnDocumentManagerEvent, this));
}


ezBitflags<ezAssetDocumentFlags> ezMeshAssetDocumentManager::GetAssetDocumentTypeFlags(const ezDocumentTypeDescriptor* pDescriptor) const
{
  EZ_ASSERT_DEBUG(pDescriptor->m_pManager == this, "Given type descriptor is not part of this document manager!");
  return ezAssetDocumentFlags::SupportsThumbnail;
}

void ezMeshAssetDocumentManager::OnDocumentManagerEvent(const ezDocumentManager::Event& e)
{
  switch (e.m_Type)
  {
  case ezDocumentManager::Event::Type::DocumentWindowRequested:
    {
      if (e.m_pDocument->GetDynamicRTTI() == ezGetStaticRTTI<ezMeshAssetDocument>())
      {
        ezQtMeshAssetDocumentWindow* pDocWnd = new ezQtMeshAssetDocumentWindow(static_cast<ezMeshAssetDocument*>(e.m_pDocument));
      }
    }
    break;
  }
}

ezStatus ezMeshAssetDocumentManager::InternalCanOpenDocument(const char* szDocumentTypeName, const char* szFilePath) const
{
  return ezStatus(EZ_SUCCESS);
}

ezStatus ezMeshAssetDocumentManager::InternalCreateDocument(const char* szDocumentTypeName, const char* szPath, ezDocument*& out_pDocument)
{
  out_pDocument = new ezMeshAssetDocument(szPath);

  return ezStatus(EZ_SUCCESS);
}

void ezMeshAssetDocumentManager::InternalGetSupportedDocumentTypes(ezDynamicArray<const ezDocumentTypeDescriptor*>& inout_DocumentTypes) const
{
  inout_DocumentTypes.PushBack(&m_AssetDesc);
}



