#include <PCH.h>
#include <EditorFramework/Actions/AssetActions.h>
#include <EditorFramework/Assets/AssetDocument.h>
#include <EditorFramework/Assets/AssetCurator.h>
#include <GuiFoundation/Action/ActionMapManager.h>
#include <GuiFoundation/Action/ActionManager.h>

ezActionDescriptorHandle ezAssetActions::s_hAssetCategory;
ezActionDescriptorHandle ezAssetActions::s_hTransformAsset;
ezActionDescriptorHandle ezAssetActions::s_hTransformAllAssets;
ezActionDescriptorHandle ezAssetActions::s_hCheckFileSystem;
ezActionDescriptorHandle ezAssetActions::s_hWriteLookupTable;
ezActionDescriptorHandle ezAssetActions::s_hRetrieveAssetInfo;


void ezAssetActions::RegisterActions()
{
  s_hAssetCategory = EZ_REGISTER_CATEGORY("AssetCategory");
  s_hTransformAsset = EZ_REGISTER_ACTION_1("Asset.Transform", ezActionScope::Document, "Assets", "", ezAssetAction, ezAssetAction::ButtonType::TransformAsset);
  s_hTransformAllAssets = EZ_REGISTER_ACTION_1("Asset.TransformAll", ezActionScope::Global, "Assets", "", ezAssetAction, ezAssetAction::ButtonType::TransformAllAssets);
  s_hCheckFileSystem = EZ_REGISTER_ACTION_1("Asset.CheckFilesystem", ezActionScope::Global, "Assets", "", ezAssetAction, ezAssetAction::ButtonType::CheckFileSystem);
  s_hWriteLookupTable = EZ_REGISTER_ACTION_1("Asset.WriteLookupTable", ezActionScope::Global, "Assets", "", ezAssetAction, ezAssetAction::ButtonType::WriteLookupTable);
  s_hRetrieveAssetInfo = EZ_REGISTER_ACTION_1("Asset.RetrieveInfo", ezActionScope::Document, "Assets", "", ezAssetAction, ezAssetAction::ButtonType::RetrieveAssetInfo);
}

void ezAssetActions::UnregisterActions()
{
  ezActionManager::UnregisterAction(s_hAssetCategory);
  ezActionManager::UnregisterAction(s_hTransformAsset);
  ezActionManager::UnregisterAction(s_hTransformAllAssets);
  ezActionManager::UnregisterAction(s_hCheckFileSystem);
  ezActionManager::UnregisterAction(s_hWriteLookupTable);
  ezActionManager::UnregisterAction(s_hRetrieveAssetInfo);
}

void ezAssetActions::MapActions(const char* szMapping, bool bDocument)
{
  ezActionMap* pMap = ezActionMapManager::GetActionMap(szMapping);
  EZ_ASSERT_DEV(pMap != nullptr, "The given mapping ('%s') does not exist, mapping the actions failed!", szMapping);

  pMap->MapAction(s_hAssetCategory, "", 10.0f);

  if (bDocument)
  {
    pMap->MapAction(s_hTransformAsset, "AssetCategory", 1.0f);
    pMap->MapAction(s_hRetrieveAssetInfo, "AssetCategory", 2.0f);
  }
  else
  {
    pMap->MapAction(s_hTransformAllAssets, "AssetCategory", 1.0f);
  }

  pMap->MapAction(s_hCheckFileSystem, "AssetCategory", 3.0f);
  //pMap->MapAction(s_hWriteLookupTable, "AssetCategory", 4.0f);
}

////////////////////////////////////////////////////////////////////////
// ezAssetAction
////////////////////////////////////////////////////////////////////////

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezAssetAction, 1, ezRTTINoAllocator);
EZ_END_DYNAMIC_REFLECTED_TYPE

ezAssetAction::ezAssetAction(const ezActionContext& context, const char* szName, ButtonType button)
  : ezButtonAction(context, szName, false, "")
{
  m_ButtonType = button;

  switch (m_ButtonType)
  {
  case ezAssetAction::ButtonType::TransformAsset:
    SetIconPath(":/EditorFramework/Icons/TransformAssets16.png");
    break;
  case ezAssetAction::ButtonType::TransformAllAssets:
    SetIconPath(":/EditorFramework/Icons/TransformAllAssets16.png");
    break;
  case ezAssetAction::ButtonType::CheckFileSystem:
    SetIconPath(":/EditorFramework/Icons/CheckFileSystem16.png");
    break;
  case ezAssetAction::ButtonType::WriteLookupTable:
    SetIconPath(":/EditorFramework/Icons/WriteLookupTable16.png");
    break;
  case ezAssetAction::ButtonType::RetrieveAssetInfo:
    SetIconPath(":/EditorFramework/Icons/RetriveAssetInfo16.png");
    break;
  }
}

ezAssetAction::~ezAssetAction()
{
}

void ezAssetAction::Execute(const ezVariant& value)
{
  switch (m_ButtonType)
  {
  case ezAssetAction::ButtonType::TransformAsset:
    {
      ezAssetDocument* pDoc = static_cast<ezAssetDocument*>(m_Context.m_pDocument);

      pDoc->TransformAsset();

      ezAssetCurator::GetSingleton()->WriteAssetTables();
    }
    break;

  case ezAssetAction::ButtonType::TransformAllAssets:
    {
      ezAssetCurator::GetSingleton()->TransformAllAssets();
    }
    break;

  case ezAssetAction::ButtonType::CheckFileSystem:
    {
      ezAssetCurator::GetSingleton()->CheckFileSystem();
      ezAssetCurator::GetSingleton()->WriteAssetTables();
    }
    break;

  case ezAssetAction::ButtonType::WriteLookupTable:
    {
      ezAssetCurator::GetSingleton()->WriteAssetTables();
    }
    break;

  case ezAssetAction::ButtonType::RetrieveAssetInfo:
    {
      ezAssetDocument* pDoc = static_cast<ezAssetDocument*>(m_Context.m_pDocument);

      pDoc->RetrieveAssetInfo();
    }
    break;
  }
}

