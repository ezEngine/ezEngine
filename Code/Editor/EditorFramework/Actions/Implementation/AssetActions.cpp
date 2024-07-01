#include <EditorFramework/EditorFrameworkPCH.h>

#include <EditorFramework/Actions/AssetActions.h>
#include <EditorFramework/Assets/AssetCurator.h>

ezActionDescriptorHandle ezAssetActions::s_hAssetCategory;
ezActionDescriptorHandle ezAssetActions::s_hTransformAsset;
ezActionDescriptorHandle ezAssetActions::s_hTransformAllAssets;
ezActionDescriptorHandle ezAssetActions::s_hCheckFileSystem;
ezActionDescriptorHandle ezAssetActions::s_hWriteLookupTable;
ezActionDescriptorHandle ezAssetActions::s_hWriteDependencyDGML;

void ezAssetActions::RegisterActions()
{
  s_hAssetCategory = EZ_REGISTER_CATEGORY("AssetCategory");
  s_hTransformAsset = EZ_REGISTER_ACTION_1("Asset.Transform", ezActionScope::Document, "Assets", "Ctrl+E", ezAssetAction, ezAssetAction::ButtonType::TransformAsset);
  s_hTransformAllAssets = EZ_REGISTER_ACTION_1("Asset.TransformAll", ezActionScope::Global, "Assets", "Ctrl+Shift+E", ezAssetAction, ezAssetAction::ButtonType::TransformAllAssets);
  s_hCheckFileSystem = EZ_REGISTER_ACTION_1("Asset.CheckFilesystem", ezActionScope::Global, "Assets", "", ezAssetAction, ezAssetAction::ButtonType::CheckFileSystem);
  s_hWriteLookupTable = EZ_REGISTER_ACTION_1("Asset.WriteLookupTable", ezActionScope::Global, "Assets", "", ezAssetAction, ezAssetAction::ButtonType::WriteLookupTable);
  s_hWriteDependencyDGML = EZ_REGISTER_ACTION_1("Asset.WriteDependencyDGML", ezActionScope::Document, "Assets", "", ezAssetAction, ezAssetAction::ButtonType::WriteDependencyDGML);
}

void ezAssetActions::UnregisterActions()
{
  ezActionManager::UnregisterAction(s_hAssetCategory);
  ezActionManager::UnregisterAction(s_hTransformAsset);
  ezActionManager::UnregisterAction(s_hTransformAllAssets);
  ezActionManager::UnregisterAction(s_hCheckFileSystem);
  ezActionManager::UnregisterAction(s_hWriteLookupTable);
  ezActionManager::UnregisterAction(s_hWriteDependencyDGML);
}

void ezAssetActions::MapMenuActions(ezStringView sMapping)
{
  const ezStringView sTargetMenu = "G.AssetDoc";

  ezActionMap* pMap = ezActionMapManager::GetActionMap(sMapping);
  EZ_ASSERT_DEV(pMap != nullptr, "The given mapping ('{0}') does not exist, mapping the documents actions failed!", sMapping);

  pMap->MapAction(s_hTransformAsset, sTargetMenu, 1.0f);
  pMap->MapAction(s_hWriteDependencyDGML, sTargetMenu, 10.0f);
}

void ezAssetActions::MapToolBarActions(ezStringView sMapping, bool bDocument)
{
  ezActionMap* pMap = ezActionMapManager::GetActionMap(sMapping);
  EZ_ASSERT_DEV(pMap != nullptr, "The given mapping ('{0}') does not exist, mapping the actions failed!", sMapping);

  pMap->MapAction(s_hAssetCategory, "", 10.0f);

  if (bDocument)
  {
    pMap->MapAction(s_hTransformAsset, "AssetCategory", 1.0f);
  }
  else
  {
    pMap->MapAction(s_hCheckFileSystem, "AssetCategory", 1.0f);
    pMap->MapAction(s_hTransformAllAssets, "AssetCategory", 2.0f);
  }
}

////////////////////////////////////////////////////////////////////////
// ezAssetAction
////////////////////////////////////////////////////////////////////////

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezAssetAction, 1, ezRTTINoAllocator)
EZ_END_DYNAMIC_REFLECTED_TYPE;

ezAssetAction::ezAssetAction(const ezActionContext& context, const char* szName, ButtonType button)
  : ezButtonAction(context, szName, false, "")
{
  m_ButtonType = button;

  switch (m_ButtonType)
  {
    case ezAssetAction::ButtonType::TransformAsset:
      SetIconPath(":/EditorFramework/Icons/TransformAsset.svg");
      break;
    case ezAssetAction::ButtonType::TransformAllAssets:
      SetIconPath(":/EditorFramework/Icons/TransformAllAssets.svg");
      break;
    case ezAssetAction::ButtonType::CheckFileSystem:
      SetIconPath(":/EditorFramework/Icons/CheckFileSystem.svg");
      break;
    case ezAssetAction::ButtonType::WriteLookupTable:
      SetIconPath(":/EditorFramework/Icons/WriteLookupTable.svg");
      break;
    case ezAssetAction::ButtonType::WriteDependencyDGML:
      break;
  }
}

ezAssetAction::~ezAssetAction() = default;

void ezAssetAction::Execute(const ezVariant& value)
{
  switch (m_ButtonType)
  {
    case ezAssetAction::ButtonType::TransformAsset:
    {
      if (m_Context.m_pDocument->IsModified())
      {
        ezStatus res = const_cast<ezDocument*>(m_Context.m_pDocument)->SaveDocument();
        if (res.m_Result.Failed())
        {
          ezLog::Error("Failed to save document '{0}': '{1}'", m_Context.m_pDocument->GetDocumentPath(), res.m_sMessage);
          break;
        }
      }

      ezTransformStatus ret = ezAssetCurator::GetSingleton()->TransformAsset(m_Context.m_pDocument->GetGuid(), ezTransformFlags::ForceTransform | ezTransformFlags::TriggeredManually);

      if (ret.Failed())
      {
        ezLog::Error("Transform failed: '{0}' ({1})", ret.m_sMessage, m_Context.m_pDocument->GetDocumentPath());
      }
      else
      {
        ezAssetCurator::GetSingleton()->WriteAssetTables().IgnoreResult();
      }
    }
    break;

    case ezAssetAction::ButtonType::TransformAllAssets:
    {
      ezAssetCurator::GetSingleton()->CheckFileSystem();
      ezAssetCurator::GetSingleton()->TransformAllAssets(ezTransformFlags::None).IgnoreResult();
    }
    break;

    case ezAssetAction::ButtonType::CheckFileSystem:
    {
      ezAssetCurator::GetSingleton()->CheckFileSystem();
      ezAssetCurator::GetSingleton()->WriteAssetTables().IgnoreResult();
    }
    break;

    case ezAssetAction::ButtonType::WriteLookupTable:
    {
      ezAssetCurator::GetSingleton()->WriteAssetTables().IgnoreResult();
    }
    break;

    case ezAssetAction::ButtonType::WriteDependencyDGML:
    {
      ezStringBuilder sOutput = QFileDialog::getSaveFileName(QApplication::activeWindow(), "Write to DGML", {}, "DGML (*.dgml)", nullptr, QFileDialog::Option::DontResolveSymlinks).toUtf8().data();

      if (sOutput.IsEmpty())
        return;

      ezAssetCurator::GetSingleton()->WriteDependencyDGML(m_Context.m_pDocument->GetGuid(), sOutput);
    }
    break;
  }
}
