#include <EditorPluginRmlUi/EditorPluginRmlUiPCH.h>

#include <EditorPluginRmlUi/Actions/RmlUiActions.h>
#include <EditorPluginRmlUi/RmlUiAsset/RmlUiAsset.h>
#include <GuiFoundation/Action/ActionManager.h>
#include <GuiFoundation/Action/ActionMapManager.h>

// clang-format off
EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezRmlUiAction, 1, ezRTTINoAllocator)
EZ_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

ezActionDescriptorHandle ezRmlUiActions::s_hCategory;
ezActionDescriptorHandle ezRmlUiActions::s_hOpenInVSC;

void ezRmlUiActions::RegisterActions()
{
  s_hCategory = EZ_REGISTER_CATEGORY("RmlUiCategory");
  s_hOpenInVSC = EZ_REGISTER_ACTION_1("RmlUi.OpenInVSC", ezActionScope::Document, "RmlUi", "", ezRmlUiAction, ezRmlUiAction::ActionType::OpenInVSC);
}

void ezRmlUiActions::UnregisterActions()
{
  ezActionManager::UnregisterAction(s_hCategory);
  ezActionManager::UnregisterAction(s_hOpenInVSC);
}

void ezRmlUiActions::MapActionsMenu(ezStringView sMapping)
{
  ezActionMap* pMap = ezActionMapManager::GetActionMap(sMapping);
  EZ_ASSERT_DEV(pMap != nullptr, "The given mapping ('{0}') does not exist, mapping the actions failed!", sMapping);

  pMap->MapAction(s_hCategory, "G.Asset", 5.0f);

  pMap->MapAction(s_hOpenInVSC, "RmlUiCategory", 1.0f);
}

void ezRmlUiActions::MapActionsToolbar(ezStringView sMapping)
{
  ezActionMap* pMap = ezActionMapManager::GetActionMap(sMapping);
  EZ_ASSERT_DEV(pMap != nullptr, "The given mapping ('{0}') does not exist, mapping the actions failed!", sMapping);

  pMap->MapAction(s_hCategory, "", 11.0f);

  const char* szSubPath = "RmlUiCategory";

  pMap->MapAction(s_hOpenInVSC, szSubPath, 1.0f);
}

ezRmlUiAction::ezRmlUiAction(const ezActionContext& context, const char* szName, ezRmlUiAction::ActionType type)
  : ezButtonAction(context, szName, false, "")
{
  m_Type = type;

  m_pDocument = const_cast<ezRmlUiAssetDocument*>(static_cast<const ezRmlUiAssetDocument*>(context.m_pDocument));

  switch (m_Type)
  {
    case ActionType::OpenInVSC:
      SetIconPath(":/GuiFoundation/Icons/vscode.svg");
      break;
  }
}


void ezRmlUiAction::Execute(const ezVariant& value)
{
  switch (m_Type)
  {
    case ActionType::OpenInVSC:
      m_pDocument->OpenExternalEditor();
      return;
  }
}
