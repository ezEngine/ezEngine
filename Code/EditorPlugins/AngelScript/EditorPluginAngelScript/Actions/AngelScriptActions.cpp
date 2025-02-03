#include <EditorPluginAngelScript/EditorPluginAngelScriptPCH.h>

#include <EditorPluginAngelScript/Actions/AngelScriptActions.h>
#include <EditorPluginAngelScript/AngelScriptAsset/AngelScriptAsset.h>
#include <GuiFoundation/Action/ActionManager.h>
#include <GuiFoundation/Action/ActionMapManager.h>

// clang-format off
EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezAngelScriptAction, 1, ezRTTINoAllocator)
EZ_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

ezActionDescriptorHandle ezAngelScriptActions::s_hCategory;
ezActionDescriptorHandle ezAngelScriptActions::s_hOpenInVSC;
ezActionDescriptorHandle ezAngelScriptActions::s_hSyncExposedParams;

void ezAngelScriptActions::RegisterActions()
{
  s_hCategory = EZ_REGISTER_CATEGORY("AngelScriptCategory");
  s_hOpenInVSC = EZ_REGISTER_ACTION_1("AngelScript.OpenInVSC", ezActionScope::Document, "AngelScript", "", ezAngelScriptAction, ezAngelScriptAction::ActionType::OpenInVSC);
  s_hSyncExposedParams = EZ_REGISTER_ACTION_1("AngelScript.SyncExposedParams", ezActionScope::Document, "AngelScript", "", ezAngelScriptAction, ezAngelScriptAction::ActionType::SyncExposedParameters);
}

void ezAngelScriptActions::UnregisterActions()
{
  ezActionManager::UnregisterAction(s_hCategory);
  ezActionManager::UnregisterAction(s_hOpenInVSC);
  ezActionManager::UnregisterAction(s_hSyncExposedParams);
}

void ezAngelScriptActions::MapActions(ezStringView sMapping)
{
  ezActionMap* pMap = ezActionMapManager::GetActionMap(sMapping);
  EZ_ASSERT_DEV(pMap != nullptr, "The given mapping ('{0}') does not exist, mapping the actions failed!", sMapping);

  pMap->MapAction(s_hCategory, "", 11.0f);

  const char* szSubPath = "AngelScriptCategory";

  pMap->MapAction(s_hOpenInVSC, szSubPath, 1.0f);
  pMap->MapAction(s_hSyncExposedParams, szSubPath, 2.0f);
}

ezAngelScriptAction::ezAngelScriptAction(const ezActionContext& context, const char* szName, ezAngelScriptAction::ActionType type)
  : ezButtonAction(context, szName, false, "")
{
  m_Type = type;

  m_pDocument = const_cast<ezAngelScriptAssetDocument*>(static_cast<const ezAngelScriptAssetDocument*>(context.m_pDocument));

  switch (m_Type)
  {
    case ActionType::OpenInVSC:
      SetIconPath(":/GuiFoundation/Icons/vscode.svg");
      break;
    case ActionType::SyncExposedParameters:
      SetIconPath(":/GuiFoundation/Icons/ReloadResources.svg");
      break;
  }
}


void ezAngelScriptAction::Execute(const ezVariant& value)
{
  switch (m_Type)
  {
    case ActionType::OpenInVSC:
      m_pDocument->OpenExternalEditor();
      return;
    case ActionType::SyncExposedParameters:
      m_pDocument->SyncExposedParameters();
      return;
  }
}
