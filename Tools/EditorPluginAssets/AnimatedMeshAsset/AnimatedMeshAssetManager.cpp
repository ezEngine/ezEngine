#include <PCH.h>

#include <EditorFramework/Assets/AssetCurator.h>
#include <EditorFramework/EditorApp/EditorApp.moc.h>
#include <EditorPluginAssets/AnimatedMeshAsset/AnimatedMeshAsset.h>
#include <EditorPluginAssets/AnimatedMeshAsset/AnimatedMeshAssetManager.h>
#include <EditorPluginAssets/AnimatedMeshAsset/AnimatedMeshAssetWindow.moc.h>
#include <RendererCore/Meshes/MeshComponent.h>
#include <ToolsFoundation/Assets/AssetFileExtensionWhitelist.h>

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezAnimatedMeshAssetDocumentManager, 1, ezRTTIDefaultAllocator<ezAnimatedMeshAssetDocumentManager>);
EZ_END_DYNAMIC_REFLECTED_TYPE;

ezAnimatedMeshAssetDocumentManager::ezAnimatedMeshAssetDocumentManager()
{
  ezDocumentManager::s_Events.AddEventHandler(ezMakeDelegate(&ezAnimatedMeshAssetDocumentManager::OnDocumentManagerEvent, this));

  m_AssetDesc.m_bCanCreate = true;
  m_AssetDesc.m_sDocumentTypeName = "Animated Mesh Asset";
  m_AssetDesc.m_sFileExtension = "ezAnimatedMeshAsset";
  m_AssetDesc.m_sIcon = ":/AssetIcons/Animated_Mesh.png";
  m_AssetDesc.m_pDocumentType = ezGetStaticRTTI<ezAnimatedMeshAssetDocument>();
  m_AssetDesc.m_pManager = this;
}

ezAnimatedMeshAssetDocumentManager::~ezAnimatedMeshAssetDocumentManager()
{
  ezDocumentManager::s_Events.RemoveEventHandler(ezMakeDelegate(&ezAnimatedMeshAssetDocumentManager::OnDocumentManagerEvent, this));
}


ezBitflags<ezAssetDocumentFlags>
ezAnimatedMeshAssetDocumentManager::GetAssetDocumentTypeFlags(const ezDocumentTypeDescriptor* pDescriptor) const
{
  EZ_ASSERT_DEBUG(pDescriptor->m_pManager == this, "Given type descriptor is not part of this document manager!");
  return ezAssetDocumentFlags::SupportsThumbnail;
}

void ezAnimatedMeshAssetDocumentManager::OnDocumentManagerEvent(const ezDocumentManager::Event& e)
{
  switch (e.m_Type)
  {
    case ezDocumentManager::Event::Type::DocumentWindowRequested:
    {
      if (e.m_pDocument->GetDynamicRTTI() == ezGetStaticRTTI<ezAnimatedMeshAssetDocument>())
      {
        ezQtAnimatedMeshAssetDocumentWindow* pDocWnd =
            new ezQtAnimatedMeshAssetDocumentWindow(static_cast<ezAnimatedMeshAssetDocument*>(e.m_pDocument));
      }
    }
    break;
  }
}

ezStatus ezAnimatedMeshAssetDocumentManager::InternalCreateDocument(const char* szDocumentTypeName, const char* szPath,
                                                                    ezDocument*& out_pDocument)
{
  out_pDocument = new ezAnimatedMeshAssetDocument(szPath);

  return ezStatus(EZ_SUCCESS);
}

void ezAnimatedMeshAssetDocumentManager::InternalGetSupportedDocumentTypes(
    ezDynamicArray<const ezDocumentTypeDescriptor*>& inout_DocumentTypes) const
{
  inout_DocumentTypes.PushBack(&m_AssetDesc);
}
