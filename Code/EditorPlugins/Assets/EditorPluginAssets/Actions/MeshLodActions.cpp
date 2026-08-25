#include <EditorPluginAssets/EditorPluginAssetsPCH.h>

#include <EditorFramework/Assets/AssetBrowserContext.h>
#include <EditorFramework/Assets/AssetCurator.h>
#include <EditorFramework/Assets/AssetDocument.h>
#include <EditorPluginAssets/Actions/MeshLodActions.h>
#include <EditorPluginAssets/Dialogs/CreateMeshLodsDlg.moc.h>
#include <GuiFoundation/Action/ActionManager.h>
#include <GuiFoundation/Action/ActionMapManager.h>
#include <GuiFoundation/UIServices/UIServices.moc.h>

// clang-format off
EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezMeshLodAction, 1, ezRTTINoAllocator)
EZ_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

ezActionDescriptorHandle ezMeshLodActions::s_hCategory;
ezActionDescriptorHandle ezMeshLodActions::s_hCreateLods;
ezActionDescriptorHandle ezMeshLodActions::s_hCreateLodsDoc;

void ezMeshLodActions::RegisterActions()
{
  s_hCategory = EZ_REGISTER_CATEGORY("MeshLodCategory");

  s_hCreateLods = EZ_REGISTER_ACTION_0("Meshes.CreateLods", ezActionScope::Global, "Meshes", "", ezMeshLodAction);
  s_hCreateLodsDoc = EZ_REGISTER_ACTION_0("Meshes.CreateLodsDocument", ezActionScope::Document, "Meshes", "", ezMeshLodAction);
}

void ezMeshLodActions::UnregisterActions()
{
  ezActionManager::UnregisterAction(s_hCategory);
  ezActionManager::UnregisterAction(s_hCreateLods);
  ezActionManager::UnregisterAction(s_hCreateLodsDoc);
}

ezResult ezMeshLodActions::MapActions(ezStringView sActionMap, ezStringView sSubPath, bool bDocumentScope)
{
  ezActionMap* pMap = ezActionMapManager::GetActionMap(sActionMap);
  if (pMap == nullptr)
    return EZ_FAILURE;

  // 9.0f, so that creating LODs sits above creating a prefab (10.0f) and a collider (11.0f)
  pMap->MapAction(s_hCategory, sSubPath, 9.0f);
  pMap->MapAction(bDocumentScope ? s_hCreateLodsDoc : s_hCreateLods, "MeshLodCategory", 1.0f);
  return EZ_SUCCESS;
}

bool ezMeshLodAction::IsLodAsset(const ezUuid& assetGuid)
{
  auto pSubAsset = ezAssetCurator::GetSingleton()->GetSubAsset(assetGuid);
  if (!pSubAsset.isValid() || pSubAsset->m_pAssetInfo == nullptr)
    return false;

  return ezPathUtils::GetFileName(pSubAsset->m_pAssetInfo->m_Path.GetAbsolutePath()).StartsWith_NoCase("LOD-");
}

void ezMeshLodAction::GetTargetAssets(ezDynamicArray<ezUuid>& out_assets) const
{
  out_assets.Clear();

  if (const ezAssetDocument* pAssetDoc = ezDynamicCast<const ezAssetDocument*>(m_Context.m_pDocument))
  {
    if (ezMeshLodCreator::IsMeshAsset(pAssetDoc->GetGuid()) && !IsLodAsset(pAssetDoc->GetGuid()))
    {
      out_assets.PushBack(pAssetDoc->GetGuid());
    }

    return;
  }

  for (const ezUuid& guid : ezAssetBrowserSelection::GetCurrent().m_AssetGuids)
  {
    if (ezMeshLodCreator::IsMeshAsset(guid) && !IsLodAsset(guid))
    {
      out_assets.PushBack(guid);
    }
  }
}

ezMeshLodAction::ezMeshLodAction(const ezActionContext& context, const char* szName)
  : ezButtonAction(context, szName, false, "")
{
  SetIconPath(":/AssetIcons/Mesh.svg");

  RefreshState();
}

void ezMeshLodAction::RefreshState()
{
  ezHybridArray<ezUuid, 16> assets;
  GetTargetAssets(assets);

  SetVisible(!assets.IsEmpty(), false);
  SetEnabled(!assets.IsEmpty(), false);
}

void ezMeshLodAction::Execute(const ezVariant& value)
{
  ezHybridArray<ezUuid, 16> assets;
  GetTargetAssets(assets);

  if (assets.IsEmpty())
    return;

  if (assets.GetCount() == 1)
  {
    ezMeshLodSource source;
    if (ezMeshLodCreator::GatherMeshLodSource(assets[0], source).Failed())
    {
      ezQtUiServices::MessageBoxWarning("The selected asset is not a mesh asset.");
      return;
    }

    ezQtCreateMeshLodsDlg dlg(source, nullptr);
    if (dlg.exec() != QDialog::Accepted)
      return;

    ezUInt32 uiCreated = 0;
    ezUInt32 uiSkipped = 0;
    const ezStatus res = ezMeshLodCreator::CreateMeshLods(source, dlg.GetOptions(), uiCreated, uiSkipped);

    if (res.Failed())
    {
      ezQtUiServices::MessageBoxStatus(res, "Failed to create the LOD assets.", "", true);
      return;
    }

    if (uiSkipped > 0)
    {
      ezQtUiServices::MessageBoxInformation(ezFmt("Created {} LOD asset(s), skipped {}.", uiCreated, uiSkipped));
    }

    return;
  }

  ezQtCreateMeshLodsDlg dlg(assets.GetCount(), nullptr);
  if (dlg.exec() != QDialog::Accepted)
    return;

  ezUInt32 uiCreated = 0;
  ezUInt32 uiSkipped = 0;
  const ezStatus res = ezMeshLodCreator::CreateMeshLodsForAll(assets, dlg.GetOptions(), uiCreated, uiSkipped);

  if (res.Failed())
  {
    ezQtUiServices::MessageBoxStatus(res, "Failed to create the LOD assets.", "", true);
    return;
  }

  // which meshes were skipped and why is in the log
  if (uiSkipped > 0)
  {
    ezQtUiServices::MessageBoxInformation(ezFmt("Created {} LOD asset(s), skipped {}.\n\nSee the log for details.", uiCreated, uiSkipped));
  }
}
