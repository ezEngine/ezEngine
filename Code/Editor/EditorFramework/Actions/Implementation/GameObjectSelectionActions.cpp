#include <EditorFramework/EditorFrameworkPCH.h>

#include <EditorFramework/Actions/GameObjectSelectionActions.h>
#include <EditorFramework/Document/GameObjectDocument.h>

// clang-format off
EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezGameObjectSelectionAction, 1, ezRTTINoAllocator)
EZ_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

ezActionDescriptorHandle ezGameObjectSelectionActions::s_hSelectionCategory;
ezActionDescriptorHandle ezGameObjectSelectionActions::s_hShowInScenegraph;
ezActionDescriptorHandle ezGameObjectSelectionActions::s_hFocusOnSelection;
ezActionDescriptorHandle ezGameObjectSelectionActions::s_hFocusOnSelectionAllViews;
ezActionDescriptorHandle ezGameObjectSelectionActions::s_hSnapCameraToObject;
ezActionDescriptorHandle ezGameObjectSelectionActions::s_hMoveCameraHere;
ezActionDescriptorHandle ezGameObjectSelectionActions::s_hCreateEmptyGameObjectHere;

void ezGameObjectSelectionActions::RegisterActions()
{
  s_hSelectionCategory = EZ_REGISTER_CATEGORY("G.Selection");
  s_hShowInScenegraph = EZ_REGISTER_ACTION_1("Selection.ShowInScenegraph", ezActionScope::Document, "Scene - Selection", "Ctrl+T",
    ezGameObjectSelectionAction, ezGameObjectSelectionAction::ActionType::ShowInScenegraph);
  s_hFocusOnSelection = EZ_REGISTER_ACTION_1("Selection.FocusSingleView", ezActionScope::Document, "Scene - Selection", "F",
    ezGameObjectSelectionAction, ezGameObjectSelectionAction::ActionType::FocusOnSelection);
  s_hFocusOnSelectionAllViews = EZ_REGISTER_ACTION_1("Selection.FocusAllViews", ezActionScope::Document, "Scene - Selection", "Shift+F",
    ezGameObjectSelectionAction, ezGameObjectSelectionAction::ActionType::FocusOnSelectionAllViews);
  s_hSnapCameraToObject = EZ_REGISTER_ACTION_1("Scene.Camera.SnapCameraToObject", ezActionScope::Document, "Camera", "", ezGameObjectSelectionAction,
    ezGameObjectSelectionAction::ActionType::SnapCameraToObject);
  s_hMoveCameraHere = EZ_REGISTER_ACTION_1("Scene.Camera.MoveCameraHere", ezActionScope::Document, "Camera", "C", ezGameObjectSelectionAction,
    ezGameObjectSelectionAction::ActionType::MoveCameraHere);

  s_hCreateEmptyGameObjectHere = EZ_REGISTER_ACTION_1("Scene.GameObject.CreateEmptyHere", ezActionScope::Document, "Scene", "",
    ezGameObjectSelectionAction, ezGameObjectSelectionAction::ActionType::CreateGameObjectHere);
}

void ezGameObjectSelectionActions::UnregisterActions()
{
  ezActionManager::UnregisterAction(s_hSelectionCategory);
  ezActionManager::UnregisterAction(s_hShowInScenegraph);
  ezActionManager::UnregisterAction(s_hFocusOnSelection);
  ezActionManager::UnregisterAction(s_hFocusOnSelectionAllViews);
  ezActionManager::UnregisterAction(s_hSnapCameraToObject);
  ezActionManager::UnregisterAction(s_hMoveCameraHere);
  ezActionManager::UnregisterAction(s_hCreateEmptyGameObjectHere);
}

void ezGameObjectSelectionActions::MapActions(ezStringView sMapping)
{
  ezActionMap* pMap = ezActionMapManager::GetActionMap(sMapping);
  EZ_ASSERT_DEV(pMap != nullptr, "The given mapping ('{0}') does not exist, mapping the actions failed!", sMapping);

  pMap->MapAction(s_hSelectionCategory, "G.Edit", 5.0f);

  pMap->MapAction(s_hShowInScenegraph, "G.Selection", 2.0f);
  pMap->MapAction(s_hFocusOnSelection, "G.Selection", 3.0f);
  pMap->MapAction(s_hFocusOnSelectionAllViews, "G.Selection", 3.5f);
  pMap->MapAction(s_hSnapCameraToObject, "G.Selection", 8.0f);
  pMap->MapAction(s_hMoveCameraHere, "G.Selection", 10.0f);
}

void ezGameObjectSelectionActions::MapContextMenuActions(ezStringView sMapping)
{
  ezActionMap* pMap = ezActionMapManager::GetActionMap(sMapping);
  EZ_ASSERT_DEV(pMap != nullptr, "The given mapping ('{0}') does not exist, mapping the actions failed!", sMapping);

  pMap->MapAction(s_hSelectionCategory, "", 5.0f);

  pMap->MapAction(s_hFocusOnSelectionAllViews, "G.Selection", 1.0f);
}


void ezGameObjectSelectionActions::MapViewContextMenuActions(ezStringView sMapping)
{
  ezActionMap* pMap = ezActionMapManager::GetActionMap(sMapping);
  EZ_ASSERT_DEV(pMap != nullptr, "The given mapping ('{0}') does not exist, mapping the actions failed!", sMapping);

  pMap->MapAction(s_hSelectionCategory, "", 5.0f);

  pMap->MapAction(s_hFocusOnSelectionAllViews, "G.Selection", 1.0f);
  pMap->MapAction(s_hSnapCameraToObject, "G.Selection", 4.0f);
  pMap->MapAction(s_hMoveCameraHere, "G.Selection", 6.0f);
  pMap->MapAction(s_hCreateEmptyGameObjectHere, "G.Selection", 1.0f);
}

ezGameObjectSelectionAction::ezGameObjectSelectionAction(
  const ezActionContext& context, const char* szName, ezGameObjectSelectionAction::ActionType type)
  : ezButtonAction(context, szName, false, "")
{
  m_Type = type;
  m_pSceneDocument = const_cast<ezGameObjectDocument*>(static_cast<const ezGameObjectDocument*>(context.m_pDocument));

  switch (m_Type)
  {
    case ActionType::ShowInScenegraph:
      SetIconPath(":/EditorFramework/Icons/Scenegraph.svg");
      break;
    case ActionType::FocusOnSelection:
      SetIconPath(":/EditorFramework/Icons/FocusOnSelection.svg");
      break;
    case ActionType::FocusOnSelectionAllViews:
      SetIconPath(":/EditorFramework/Icons/FocusOnSelectionAllViews.svg");
      break;
    case ActionType::SnapCameraToObject:
      // SetIconPath(":/EditorFramework/Icons/SnapToCamera.svg"); // TODO Icon
      break;
    case ActionType::MoveCameraHere:
      SetIconPath(":/EditorFramework/Icons/MoveCameraHere.svg");
      break;
    case ActionType::CreateGameObjectHere:
      SetIconPath(":/EditorFramework/Icons/CreateEmpty.svg");
      break;
  }

  UpdateEnableState();

  m_Context.m_pDocument->GetSelectionManager()->m_Events.AddEventHandler(ezMakeDelegate(&ezGameObjectSelectionAction::SelectionEventHandler, this));
}


ezGameObjectSelectionAction::~ezGameObjectSelectionAction()
{
  m_Context.m_pDocument->GetSelectionManager()->m_Events.RemoveEventHandler(
    ezMakeDelegate(&ezGameObjectSelectionAction::SelectionEventHandler, this));
}

void ezGameObjectSelectionAction::Execute(const ezVariant& value)
{
  switch (m_Type)
  {
    case ActionType::ShowInScenegraph:
      m_pSceneDocument->TriggerShowSelectionInScenegraph();
      return;
    case ActionType::FocusOnSelection:
      m_pSceneDocument->TriggerFocusOnSelection(false);
      return;
    case ActionType::FocusOnSelectionAllViews:
      m_pSceneDocument->TriggerFocusOnSelection(true);
      return;
    case ActionType::SnapCameraToObject:
      m_pSceneDocument->SnapCameraToObject();
      break;
    case ActionType::MoveCameraHere:
      m_pSceneDocument->MoveCameraHere();
      break;
    case ActionType::CreateGameObjectHere:
    {
      auto res = m_pSceneDocument->CreateGameObjectHere();
      ezQtUiServices::GetSingleton()->MessageBoxStatus(res, "Create empty object at picked position failed.");
    }
    break;
  }
}

void ezGameObjectSelectionAction::SelectionEventHandler(const ezSelectionManagerEvent& e)
{
  UpdateEnableState();
}

void ezGameObjectSelectionAction::UpdateEnableState()
{
  if (m_Type == ActionType::FocusOnSelection || m_Type == ActionType::FocusOnSelectionAllViews || m_Type == ActionType::ShowInScenegraph)
  {
    SetEnabled(!m_Context.m_pDocument->GetSelectionManager()->IsSelectionEmpty());
  }

  if (m_Type == ActionType::SnapCameraToObject)
  {
    SetEnabled(m_Context.m_pDocument->GetSelectionManager()->GetSelection().GetCount() == 1);
  }
}
