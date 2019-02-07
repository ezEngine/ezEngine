#include <EditorFrameworkPCH.h>

#include <Core/World/GameObject.h>
#include <EditorFramework/Actions/GameObjectSelectionActions.h>
#include <EditorFramework/Assets/AssetCurator.h>
#include <EditorFramework/Document/GameObjectDocument.h>
#include <EditorFramework/EditorApp/EditorApp.moc.h>
#include <Foundation/IO/OSFile.h>
#include <GuiFoundation/Action/ActionManager.h>
#include <GuiFoundation/Action/ActionMapManager.h>
#include <QFileDialog>
#include <ToolsFoundation/Command/TreeCommands.h>
#include <ToolsFoundation/Project/ToolsProject.h>

// clang-format off
EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezGameObjectSelectionAction, 1, ezRTTINoAllocator);
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
  s_hSelectionCategory = EZ_REGISTER_CATEGORY("SelectionCategory");
  s_hShowInScenegraph = EZ_REGISTER_ACTION_1("Selection.ShowInScenegraph", ezActionScope::Document, "Scene - Selection", "Ctrl+T",
                                             ezGameObjectSelectionAction, ezGameObjectSelectionAction::ActionType::ShowInScenegraph);
  s_hFocusOnSelection = EZ_REGISTER_ACTION_1("Selection.FocusSingleView", ezActionScope::Document, "Scene - Selection", "F",
                                             ezGameObjectSelectionAction, ezGameObjectSelectionAction::ActionType::FocusOnSelection);
  s_hFocusOnSelectionAllViews =
      EZ_REGISTER_ACTION_1("Selection.FocusAllViews", ezActionScope::Document, "Scene - Selection", "Shift+F",
                           ezGameObjectSelectionAction, ezGameObjectSelectionAction::ActionType::FocusOnSelectionAllViews);
  s_hSnapCameraToObject = EZ_REGISTER_ACTION_1("Scene.Camera.SnapCameraToObject", ezActionScope::Document, "Camera", "",
                                               ezGameObjectSelectionAction, ezGameObjectSelectionAction::ActionType::SnapCameraToObject);
  s_hMoveCameraHere = EZ_REGISTER_ACTION_1("Scene.Camera.MoveCameraHere", ezActionScope::Document, "Camera", "C",
                                           ezGameObjectSelectionAction, ezGameObjectSelectionAction::ActionType::MoveCameraHere);

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

void ezGameObjectSelectionActions::MapActions(const char* szMapping, const char* szPath)
{
  ezActionMap* pMap = ezActionMapManager::GetActionMap(szMapping);
  EZ_ASSERT_DEV(pMap != nullptr, "The given mapping ('{0}') does not exist, mapping the actions failed!", szMapping);

  ezStringBuilder sSubPath(szPath, "/SelectionCategory");

  pMap->MapAction(s_hSelectionCategory, szPath, 5.0f);

  pMap->MapAction(s_hShowInScenegraph, sSubPath, 2.0f);
  pMap->MapAction(s_hFocusOnSelection, sSubPath, 3.0f);
  pMap->MapAction(s_hFocusOnSelectionAllViews, sSubPath, 3.5f);
  pMap->MapAction(s_hSnapCameraToObject, sSubPath, 8.0f);
  pMap->MapAction(s_hMoveCameraHere, sSubPath, 10.0f);
}

void ezGameObjectSelectionActions::MapContextMenuActions(const char* szMapping, const char* szPath)
{
  ezActionMap* pMap = ezActionMapManager::GetActionMap(szMapping);
  EZ_ASSERT_DEV(pMap != nullptr, "The given mapping ('{0}') does not exist, mapping the actions failed!", szMapping);

  ezStringBuilder sSubPath(szPath, "/SelectionCategory");

  pMap->MapAction(s_hSelectionCategory, szPath, 5.0f);
  pMap->MapAction(s_hFocusOnSelectionAllViews, sSubPath, 1.0f);
}


void ezGameObjectSelectionActions::MapViewContextMenuActions(const char* szMapping, const char* szPath)
{
  ezActionMap* pMap = ezActionMapManager::GetActionMap(szMapping);
  EZ_ASSERT_DEV(pMap != nullptr, "The given mapping ('{0}') does not exist, mapping the actions failed!", szMapping);

  ezStringBuilder sSubPath(szPath, "/SelectionCategory");

  pMap->MapAction(s_hSelectionCategory, szPath, 5.0f);

  pMap->MapAction(s_hFocusOnSelectionAllViews, sSubPath, 1.0f);
  pMap->MapAction(s_hSnapCameraToObject, sSubPath, 4.0f);
  pMap->MapAction(s_hMoveCameraHere, sSubPath, 6.0f);
  pMap->MapAction(s_hCreateEmptyGameObjectHere, sSubPath, 1.0f);
}

ezGameObjectSelectionAction::ezGameObjectSelectionAction(const ezActionContext& context, const char* szName,
                                                         ezGameObjectSelectionAction::ActionType type)
    : ezButtonAction(context, szName, false, "")
{
  m_Type = type;
  // TODO const cast
  m_pSceneDocument = const_cast<ezGameObjectDocument*>(static_cast<const ezGameObjectDocument*>(context.m_pDocument));

  switch (m_Type)
  {
    case ActionType::ShowInScenegraph:
      SetIconPath(":/EditorFramework/Icons/Scenegraph16.png");
      break;
    case ActionType::FocusOnSelection:
      SetIconPath(":/EditorFramework/Icons/FocusOnSelection16.png");
      break;
    case ActionType::FocusOnSelectionAllViews:
      SetIconPath(":/EditorFramework/Icons/FocusOnSelectionAllViews16.png");
      break;
    case ActionType::SnapCameraToObject:
      // SetIconPath(":/EditorFramework/Icons/Duplicate16.png"); // TODO Icon
      break;
    case ActionType::MoveCameraHere:
      // SetIconPath(":/EditorFramework/Icons/Duplicate16.png"); // TODO Icon
      break;
    case ActionType::CreateGameObjectHere:
      SetIconPath(":/EditorFramework/Icons/CreateEmpty16.png");
      break;
  }

  UpdateEnableState();

  m_Context.m_pDocument->GetSelectionManager()->m_Events.AddEventHandler(
      ezMakeDelegate(&ezGameObjectSelectionAction::SelectionEventHandler, this));
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
