#include <PCH.h>
#include <GuiFoundation/Action/ActionManager.h>
#include <GuiFoundation/Action/ActionMapManager.h>
#include <EditorPluginScene/Actions/SelectionActions.h>
#include <EditorPluginScene/Scene/SceneDocument.h>

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezSelectionAction, ezButtonAction, 0, ezRTTINoAllocator);
EZ_END_DYNAMIC_REFLECTED_TYPE();

ezActionDescriptorHandle ezSelectionActions::s_hSelectionCategory;
ezActionDescriptorHandle ezSelectionActions::s_hShowInScenegraph;
ezActionDescriptorHandle ezSelectionActions::s_hFocusOnSelection;

void ezSelectionActions::RegisterActions()
{
  s_hSelectionCategory = EZ_REGISTER_CATEGORY("SelectionCategory");
  s_hShowInScenegraph = EZ_REGISTER_ACTION_1("ShowInScenegraph", "Show in Scenegraph", ezActionScope::Document, "Document", "Ctrl+T", ezSelectionAction, ezSelectionAction::ActionType::ShowInScenegraph);
  s_hFocusOnSelection = EZ_REGISTER_ACTION_1("FocusOnSelection", "Focus on Selection", ezActionScope::Document, "Document", "F", ezSelectionAction, ezSelectionAction::ActionType::FocusOnSelection);
}

void ezSelectionActions::UnregisterActions()
{
  ezActionManager::UnregisterAction(s_hSelectionCategory);
  ezActionManager::UnregisterAction(s_hShowInScenegraph);
  ezActionManager::UnregisterAction(s_hFocusOnSelection);
}

void ezSelectionActions::MapActions(const char* szMapping, const char* szPath)
{
  ezActionMap* pMap = ezActionMapManager::GetActionMap(szMapping);
  EZ_ASSERT_DEV(pMap != nullptr, "The given mapping ('%s') does not exist, mapping the actions failed!", szMapping);

  ezStringBuilder sSubPath(szPath, "/SelectionCategory");

  pMap->MapAction(s_hSelectionCategory, szPath, 5.0f);
  pMap->MapAction(s_hShowInScenegraph, sSubPath, 1.0f);
  pMap->MapAction(s_hFocusOnSelection, sSubPath, 2.0f);
}

ezSelectionAction::ezSelectionAction(const ezActionContext& context, const char* szName, ezSelectionAction::ActionType type) : ezButtonAction(context, szName, false, "")
{
  m_Type = type;
  m_pSceneDocument = static_cast<ezSceneDocument*>(context.m_pDocument);

  switch (m_Type)
  {
  case ActionType::ShowInScenegraph:
    SetIconPath(":/GuiFoundation/Icons/Scenegraph16.png");
    break;
  case ActionType::FocusOnSelection:
    SetIconPath(":/GuiFoundation/Icons/FocusOnSelection16.png");
    break;
  }
}

void ezSelectionAction::Execute(const ezVariant& value)
{
  switch (m_Type)
  {
  case ActionType::ShowInScenegraph:
    m_pSceneDocument->TriggerShowSelectionInScenegraph();
    return;
  case ActionType::FocusOnSelection:
    m_pSceneDocument->TriggerFocusOnSelection();
    return;
  }
}

