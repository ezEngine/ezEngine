#include <EditorPluginJolt/EditorPluginJoltPCH.h>

#include <EditorFramework/Assets/AssetBrowserContext.h>
#include <EditorFramework/Assets/AssetDocument.h>
#include <EditorPluginJolt/Actions/MeshColliderActions.h>
#include <EditorPluginJolt/Dialogs/CreateMeshColliderDlg.moc.h>
#include <GuiFoundation/Action/ActionManager.h>
#include <GuiFoundation/Action/ActionMapManager.h>
#include <GuiFoundation/UIServices/UIServices.moc.h>

// clang-format off
EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezMeshColliderAction, 1, ezRTTINoAllocator)
EZ_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

ezActionDescriptorHandle ezMeshColliderActions::s_hCategory;
ezActionDescriptorHandle ezMeshColliderActions::s_hCreateCollider;
ezActionDescriptorHandle ezMeshColliderActions::s_hCreateColliderDoc;

void ezMeshColliderActions::RegisterActions()
{
  s_hCategory = EZ_REGISTER_CATEGORY("MeshColliderCategory");

  // Two descriptors, because the scope decides both the action's context and how its proxy is cached:
  // Global once overall for the asset browser, Document once per mesh document.
  s_hCreateCollider = EZ_REGISTER_ACTION_0("Jolt.CreateColliderFromMesh", ezActionScope::Global, "Jolt", "", ezMeshColliderAction);
  s_hCreateColliderDoc = EZ_REGISTER_ACTION_0("Jolt.CreateColliderFromMeshDocument", ezActionScope::Document, "Jolt", "", ezMeshColliderAction);
}

void ezMeshColliderActions::UnregisterActions()
{
  ezActionManager::UnregisterAction(s_hCategory);
  ezActionManager::UnregisterAction(s_hCreateCollider);
  ezActionManager::UnregisterAction(s_hCreateColliderDoc);
}

ezResult ezMeshColliderActions::MapActions(ezStringView sActionMap, ezStringView sSubPath, bool bDocumentScope)
{
  // Not an assert: the mesh document's action maps belong to EditorPluginAssets, which is not
  // guaranteed to have been loaded first.
  ezActionMap* pMap = ezActionMapManager::GetActionMap(sActionMap);
  if (pMap == nullptr)
    return EZ_FAILURE;

  pMap->MapAction(s_hCategory, sSubPath, 11.0f);
  pMap->MapAction(bDocumentScope ? s_hCreateColliderDoc : s_hCreateCollider, "MeshColliderCategory", 1.0f);
  return EZ_SUCCESS;
}

void ezMeshColliderAction::GetTargetAssets(ezDynamicArray<ezUuid>& out_assets) const
{
  out_assets.Clear();

  if (const ezAssetDocument* pAssetDoc = ezDynamicCast<const ezAssetDocument*>(m_Context.m_pDocument))
  {
    if (ezMeshColliderCreator::IsMeshAsset(pAssetDoc->GetGuid()))
    {
      out_assets.PushBack(pAssetDoc->GetGuid());
    }

    return;
  }

  for (const ezUuid& guid : ezAssetBrowserSelection::GetCurrent().m_AssetGuids)
  {
    if (ezMeshColliderCreator::IsMeshAsset(guid))
    {
      out_assets.PushBack(guid);
    }
  }
}

ezMeshColliderAction::ezMeshColliderAction(const ezActionContext& context, const char* szName)
  : ezButtonAction(context, szName, false, "")
{
  SetIconPath(":/AssetIcons/Jolt_Collision_Mesh.svg");

  RefreshState();
}

void ezMeshColliderAction::RefreshState()
{
  ezHybridArray<ezUuid, 16> assets;
  GetTargetAssets(assets);

  SetVisible(!assets.IsEmpty(), false);
  SetEnabled(!assets.IsEmpty(), false);
}

void ezMeshColliderAction::Execute(const ezVariant& value)
{
  ezHybridArray<ezUuid, 16> assets;
  GetTargetAssets(assets);

  if (assets.IsEmpty())
    return;

  if (assets.GetCount() == 1)
  {
    ezMeshColliderSource source;
    if (ezMeshColliderCreator::GatherMeshColliderSource(assets[0], source).Failed())
    {
      ezQtUiServices::MessageBoxWarning("The selected asset is not a mesh asset.");
      return;
    }

    ezQtCreateMeshColliderDlg dlg(source, nullptr);
    if (dlg.exec() != QDialog::Accepted)
      return;

    const ezStatus res = ezMeshColliderCreator::CreateMeshCollider(source, dlg.GetOptions());
    ezQtUiServices::MessageBoxStatus(res, "Failed to create the collision mesh asset.", "", true);
    return;
  }

  ezQtCreateMeshColliderDlg dlg(assets.GetCount(), nullptr);
  if (dlg.exec() != QDialog::Accepted)
    return;

  ezUInt32 uiCreated = 0;
  ezUInt32 uiSkipped = 0;
  const ezStatus res = ezMeshColliderCreator::CreateMeshColliders(assets, dlg.GetOptions(), uiCreated, uiSkipped);

  if (res.Failed())
  {
    ezQtUiServices::MessageBoxStatus(res, "Failed to create the collision mesh assets.", "", true);
    return;
  }

  if (uiSkipped > 0)
  {
    // which meshes were skipped and why is in the log
    ezQtUiServices::MessageBoxInformation(ezFmt("Created {} collision mesh asset(s), skipped {}.\n\nSee the log for details.", uiCreated, uiSkipped));
  }
}
