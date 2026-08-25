#include <EditorPluginScene/EditorPluginScenePCH.h>

#include <EditorFramework/Assets/AssetBrowserContext.h>
#include <EditorFramework/Assets/AssetDocument.h>
#include <EditorPluginScene/Actions/MeshPrefabActions.h>
#include <EditorPluginScene/Dialogs/CreateMeshPrefabDlg.moc.h>
#include <EditorPluginScene/Utils/MeshPrefabCreator.h>
#include <GuiFoundation/Action/ActionManager.h>
#include <GuiFoundation/Action/ActionMapManager.h>
#include <GuiFoundation/UIServices/UIServices.moc.h>

// clang-format off
EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezMeshPrefabAction, 1, ezRTTINoAllocator)
EZ_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

ezActionDescriptorHandle ezMeshPrefabActions::s_hCategory;
ezActionDescriptorHandle ezMeshPrefabActions::s_hCreatePrefabFromMesh;
ezActionDescriptorHandle ezMeshPrefabActions::s_hCreatePrefabFromMeshDoc;

void ezMeshPrefabActions::RegisterActions()
{
  s_hCategory = EZ_REGISTER_CATEGORY("MeshPrefabCategory");

  s_hCreatePrefabFromMesh = EZ_REGISTER_ACTION_1("Prefabs.CreateFromMesh", ezActionScope::Global, "Prefabs", "", ezMeshPrefabAction, ezMeshPrefabAction::ActionType::CreatePrefabFromMesh);
  s_hCreatePrefabFromMeshDoc = EZ_REGISTER_ACTION_1("Prefabs.CreateFromMeshDocument", ezActionScope::Document, "Prefabs", "", ezMeshPrefabAction, ezMeshPrefabAction::ActionType::CreatePrefabFromMesh);
}

void ezMeshPrefabActions::UnregisterActions()
{
  ezActionManager::UnregisterAction(s_hCategory);
  ezActionManager::UnregisterAction(s_hCreatePrefabFromMesh);
  ezActionManager::UnregisterAction(s_hCreatePrefabFromMeshDoc);
}

ezResult ezMeshPrefabActions::MapActions(ezStringView sActionMap, ezStringView sSubPath, bool bDocumentScope)
{
  // Not an assert: the mesh document's action maps belong to EditorPluginAssets, which is not
  // guaranteed to have been loaded first.
  ezActionMap* pMap = ezActionMapManager::GetActionMap(sActionMap);
  if (pMap == nullptr)
    return EZ_FAILURE;

  pMap->MapAction(s_hCategory, sSubPath, 10.0f);
  pMap->MapAction(bDocumentScope ? s_hCreatePrefabFromMeshDoc : s_hCreatePrefabFromMesh, "MeshPrefabCategory", 1.0f);
  return EZ_SUCCESS;
}

void ezMeshPrefabAction::GetTargetAssets(ezDynamicArray<ezUuid>& out_assets) const
{
  out_assets.Clear();

  if (const ezAssetDocument* pAssetDoc = ezDynamicCast<const ezAssetDocument*>(m_Context.m_pDocument))
  {
    if (ezMeshPrefabCreator::IsMeshAsset(pAssetDoc->GetGuid()))
    {
      out_assets.PushBack(pAssetDoc->GetGuid());
    }

    return;
  }

  for (const ezUuid& guid : ezAssetBrowserSelection::GetCurrent().m_AssetGuids)
  {
    if (ezMeshPrefabCreator::IsMeshAsset(guid))
    {
      out_assets.PushBack(guid);
    }
  }
}

ezMeshPrefabAction::ezMeshPrefabAction(const ezActionContext& context, const char* szName, ezMeshPrefabAction::ActionType type)
  : ezButtonAction(context, szName, false, "")
{
  m_Type = type;

  switch (m_Type)
  {
    case ActionType::CreatePrefabFromMesh:
      SetIconPath(":/AssetIcons/Prefab.svg");
      break;
  }

  RefreshState();
}

void ezMeshPrefabAction::RefreshState()
{
  switch (m_Type)
  {
    case ActionType::CreatePrefabFromMesh:
    {
      ezHybridArray<ezUuid, 16> assets;
      GetTargetAssets(assets);

      SetVisible(!assets.IsEmpty(), false);
      SetEnabled(!assets.IsEmpty(), false);
      break;
    }
  }
}

void ezMeshPrefabAction::Execute(const ezVariant& value)
{
  switch (m_Type)
  {
    case ActionType::CreatePrefabFromMesh:
    {
      ezHybridArray<ezUuid, 16> assets;
      GetTargetAssets(assets);

      if (assets.IsEmpty())
        return;

      if (assets.GetCount() == 1)
      {
        ezMeshPrefabSource source;
        if (ezMeshPrefabCreator::GatherMeshPrefabSource(assets[0], source).Failed())
        {
          ezQtUiServices::MessageBoxWarning("The selected asset is not a mesh asset.");
          return;
        }

        ezQtCreateMeshPrefabDlg dlg(source, nullptr);
        if (dlg.exec() != QDialog::Accepted)
          return;

        const ezStatus res = ezMeshPrefabCreator::CreateMeshPrefab(source, dlg.GetOptions());
        ezQtUiServices::MessageBoxStatus(res, "Failed to create the prefab.", "", true);
        return;
      }

      ezQtCreateMeshPrefabDlg dlg(assets.GetCount(), nullptr);
      if (dlg.exec() != QDialog::Accepted)
        return;

      ezUInt32 uiCreated = 0;
      ezUInt32 uiSkipped = 0;
      const ezStatus res = ezMeshPrefabCreator::CreateMeshPrefabs(assets, dlg.GetOptions(), uiCreated, uiSkipped);

      if (res.Failed())
      {
        ezQtUiServices::MessageBoxStatus(res, "Failed to create the prefabs.", "", true);
        return;
      }

      // which meshes were skipped and why is in the log
      if (uiSkipped > 0)
      {
        ezQtUiServices::MessageBoxInformation(ezFmt("Created {} prefab(s), skipped {}.\n\nSee the log for details.", uiCreated, uiSkipped));
      }
      break;
    }
  }
}
