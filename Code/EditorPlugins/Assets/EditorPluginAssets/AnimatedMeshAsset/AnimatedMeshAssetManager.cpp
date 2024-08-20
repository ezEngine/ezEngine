#include <EditorPluginAssets/EditorPluginAssetsPCH.h>

#include <EditorPluginAssets/AnimatedMeshAsset/AnimatedMeshAssetManager.h>
#include <EditorPluginAssets/AnimatedMeshAsset/AnimatedMeshAssetWindow.moc.h>

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezAnimatedMeshAssetDocumentManager, 1, ezRTTIDefaultAllocator<ezAnimatedMeshAssetDocumentManager>)
EZ_END_DYNAMIC_REFLECTED_TYPE;

ezAnimatedMeshAssetDocumentManager::ezAnimatedMeshAssetDocumentManager()
{
  ezDocumentManager::s_Events.AddEventHandler(ezMakeDelegate(&ezAnimatedMeshAssetDocumentManager::OnDocumentManagerEvent, this));

  m_DocTypeDesc.m_sDocumentTypeName = "Animated Mesh";
  m_DocTypeDesc.m_sFileExtension = "ezAnimatedMeshAsset";
  m_DocTypeDesc.m_sIcon = ":/AssetIcons/Animated_Mesh.svg";
  m_DocTypeDesc.m_sAssetCategory = "Rendering";
  m_DocTypeDesc.m_pDocumentType = ezGetStaticRTTI<ezAnimatedMeshAssetDocument>();
  m_DocTypeDesc.m_pManager = this;
  m_DocTypeDesc.m_CompatibleTypes.PushBack("CompatibleAsset_Mesh_Static");
  m_DocTypeDesc.m_CompatibleTypes.PushBack("CompatibleAsset_Mesh_Skinned");

  m_DocTypeDesc.m_AssetDocumentFlags = ezAssetDocumentFlags::SupportsThumbnail;
  m_DocTypeDesc.m_sResourceFileExtension = "ezBinAnimatedMesh";
}

ezAnimatedMeshAssetDocumentManager::~ezAnimatedMeshAssetDocumentManager()
{
  ezDocumentManager::s_Events.RemoveEventHandler(ezMakeDelegate(&ezAnimatedMeshAssetDocumentManager::OnDocumentManagerEvent, this));
}

void ezAnimatedMeshAssetDocumentManager::OnDocumentManagerEvent(const ezDocumentManager::Event& e)
{
  switch (e.m_Type)
  {
    case ezDocumentManager::Event::Type::DocumentWindowRequested:
    {
      if (e.m_pDocument->GetDynamicRTTI() == ezGetStaticRTTI<ezAnimatedMeshAssetDocument>())
      {
        new ezQtAnimatedMeshAssetDocumentWindow(static_cast<ezAnimatedMeshAssetDocument*>(e.m_pDocument)); // NOLINT
      }
    }
    break;

    default:
      break;
  }
}

void ezAnimatedMeshAssetDocumentManager::InternalCreateDocument(ezStringView sDocumentTypeName, ezStringView sPath, bool bCreateNewDocument, ezDocument*& out_pDocument, const ezDocumentObject* pOpenContext)
{
  out_pDocument = new ezAnimatedMeshAssetDocument(sPath);
}

void ezAnimatedMeshAssetDocumentManager::InternalGetSupportedDocumentTypes(ezDynamicArray<const ezDocumentTypeDescriptor*>& inout_DocumentTypes) const
{
  inout_DocumentTypes.PushBack(&m_DocTypeDesc);
}
