#include <PCH.h>

#include <EditorFramework/Assets/AssetCurator.h>
#include <EditorFramework/EditorApp/EditorApp.moc.h>
#include <EditorPluginAssets/MeshAsset/MeshAsset.h>
#include <EditorPluginAssets/MeshAsset/MeshAssetManager.h>
#include <EditorPluginAssets/MeshAsset/MeshAssetWindow.moc.h>
#include <RendererCore/Meshes/MeshComponent.h>
#include <ToolsFoundation/Assets/AssetFileExtensionWhitelist.h>

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezMeshAssetDocumentManager, 1, ezRTTIDefaultAllocator<ezMeshAssetDocumentManager>);
EZ_END_DYNAMIC_REFLECTED_TYPE;

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

ezResult ezMeshAssetDocumentManager::OpenPickedDocument(const ezDocumentObject* pPickedComponent, ezUInt32 uiPartIndex)
{
  // check that we actually picked a mesh component
  if (!pPickedComponent->GetTypeAccessor().GetType()->IsDerivedFrom<ezMeshComponent>())
    return EZ_FAILURE;

  // first try the materials array on the component itself, and see if we have a material override to pick
  if ((ezInt32)uiPartIndex < pPickedComponent->GetTypeAccessor().GetCount("Materials"))
  {
    // access the material at the given index
    // this might be empty, though, in which case we still need to check the mesh asset
    const ezVariant varMatGuid = pPickedComponent->GetTypeAccessor().GetValue("Materials", uiPartIndex);

    // if it were anything else than a string that would be weird
    EZ_ASSERT_DEV(varMatGuid.IsA<ezString>(), "Material override property is not a string type");

    if (varMatGuid.IsA<ezString>())
    {
      if (TryOpenAssetDocument(varMatGuid.Get<ezString>()).Succeeded())
        return EZ_SUCCESS;
    }
  }

  // couldn't open it through the override, so we now need to inspect the mesh asset
  const ezVariant varMeshGuid = pPickedComponent->GetTypeAccessor().GetValue("Mesh");

  EZ_ASSERT_DEV(varMeshGuid.IsA<ezString>(), "Mesh property is not a string type");

  if (!varMeshGuid.IsA<ezString>())
    return EZ_FAILURE;

  // we don't support non-guid mesh asset references, because I'm too lazy
  if (!ezConversionUtils::IsStringUuid(varMeshGuid.Get<ezString>()))
    return EZ_FAILURE;

  const ezUuid meshGuid = ezConversionUtils::ConvertStringToUuid(varMeshGuid.Get<ezString>());

  auto pSubAsset = ezAssetCurator::GetSingleton()->GetSubAsset(meshGuid);

  // unknown mesh asset
  if (!pSubAsset)
    return EZ_FAILURE;

  // now we need to open the mesh and we cannot wait for it (usually that is queued for GUI reasons)
  // though we do not want a window
  ezMeshAssetDocument* pMeshDoc = static_cast<ezMeshAssetDocument*>(
      ezQtEditorApp::GetSingleton()->OpenDocumentImmediate(pSubAsset->m_pAssetInfo->m_sAbsolutePath, false, false));

  if (!pMeshDoc)
    return EZ_FAILURE;

  ezResult result = EZ_FAILURE;

  // if we are outside the stored index, tough luck
  if (uiPartIndex < pMeshDoc->GetProperties()->m_Slots.GetCount())
  {
    result = TryOpenAssetDocument(pMeshDoc->GetProperties()->m_Slots[uiPartIndex].m_sResource);
  }

  // make sure to close the document again, if we were the ones to open it
  // otherwise keep it open
  if (!pMeshDoc->HasWindowBeenRequested())
    pMeshDoc->GetDocumentManager()->CloseDocument(pMeshDoc);

  return result;
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

ezStatus ezMeshAssetDocumentManager::InternalCreateDocument(const char* szDocumentTypeName, const char* szPath, ezDocument*& out_pDocument)
{
  out_pDocument = new ezMeshAssetDocument(szPath);

  return ezStatus(EZ_SUCCESS);
}

void ezMeshAssetDocumentManager::InternalGetSupportedDocumentTypes(
    ezDynamicArray<const ezDocumentTypeDescriptor*>& inout_DocumentTypes) const
{
  inout_DocumentTypes.PushBack(&m_AssetDesc);
}
