#include <PCH.h>

#include <EditorFramework/Actions/TransformGizmoActions.h>
#include <EditorFramework/EditTools/StandardGizmoEditTools.h>
#include <EditorFramework/Gizmos/SnapProvider.h>
#include <GuiFoundation/Action/ActionManager.h>
#include <GuiFoundation/Action/ActionMapManager.h>
#include <EditorFramework/Dialogs/SnapSettingsDlg.moc.h>

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezGizmoAction, 0, ezRTTINoAllocator);
EZ_END_DYNAMIC_REFLECTED_TYPE;

ezGizmoAction::ezGizmoAction(const ezActionContext& context, const char* szName, const ezRTTI* pGizmoType)
  : ezButtonAction(context, szName, false, "")
{
  SetCheckable(true);
  m_pGizmoType = pGizmoType;
  m_pGameObjectDocument = static_cast<ezGameObjectDocument*>(context.m_pDocument);
  m_pGameObjectDocument->m_GameObjectEvents.AddEventHandler(ezMakeDelegate(&ezGizmoAction::GameObjectEventHandler, this));

  if (m_pGizmoType)
  {
    ezStringBuilder sIcon(":/TypeIcons/", m_pGizmoType->GetTypeName());
    SetIconPath(sIcon);
  }
  else
  {
    SetIconPath(":/EditorFramework/Icons/GizmoNone24.png");
  }

  UpdateState();
}

ezGizmoAction::~ezGizmoAction()
{
  m_pGameObjectDocument->m_GameObjectEvents.RemoveEventHandler(ezMakeDelegate(&ezGizmoAction::GameObjectEventHandler, this));
}

void ezGizmoAction::Execute(const ezVariant& value)
{
  m_pGameObjectDocument->SetActiveEditTool(m_pGizmoType);
  UpdateState();
}

void ezGizmoAction::UpdateState()
{
  SetChecked(m_pGameObjectDocument->IsActiveEditTool(m_pGizmoType));
}

void ezGizmoAction::GameObjectEventHandler(const ezGameObjectEvent& e)
{
  if (e.m_Type == ezGameObjectEvent::Type::ActiveEditToolChanged)
    UpdateState();
}

//////////////////////////////////////////////////////////////////////////

ezToggleWorldSpaceGizmo::ezToggleWorldSpaceGizmo(const ezActionContext& context, const char* szName, const ezRTTI* pGizmoType)
  : ezGizmoAction(context, szName, pGizmoType)
{
}

void ezToggleWorldSpaceGizmo::Execute(const ezVariant& value)
{
  if (m_pGameObjectDocument->IsActiveEditTool(m_pGizmoType))
  {
    // toggle local/world space if the same tool is selected again
    m_pGameObjectDocument->SetGizmoWorldSpace(!m_pGameObjectDocument->GetGizmoWorldSpace());
  }
  else
  {
    ezGizmoAction::Execute(value);
  }
}

//////////////////////////////////////////////////////////////////////////

ezActionDescriptorHandle ezTransformGizmoActions::s_hGizmoCategory;
ezActionDescriptorHandle ezTransformGizmoActions::s_hGizmoMenu;
ezActionDescriptorHandle ezTransformGizmoActions::s_hNoGizmo;
ezActionDescriptorHandle ezTransformGizmoActions::s_hTranslateGizmo;
ezActionDescriptorHandle ezTransformGizmoActions::s_hRotateGizmo;
ezActionDescriptorHandle ezTransformGizmoActions::s_hScaleGizmo;
ezActionDescriptorHandle ezTransformGizmoActions::s_hDragToPositionGizmo;
ezActionDescriptorHandle ezTransformGizmoActions::s_hWorldSpace;
ezActionDescriptorHandle ezTransformGizmoActions::s_hMoveParentOnly;
ezActionDescriptorHandle ezTransformGizmoActions::s_SnapSettings;

void ezTransformGizmoActions::RegisterActions()
{
  s_hGizmoCategory = EZ_REGISTER_CATEGORY("GizmoCategory");
  s_hGizmoMenu = EZ_REGISTER_MENU("Gizmo.Menu");
  s_hNoGizmo = EZ_REGISTER_ACTION_1("Gizmo.Mode.Select", ezActionScope::Document, "Gizmo", "Q", ezGizmoAction, nullptr);
  s_hTranslateGizmo = EZ_REGISTER_ACTION_1("Gizmo.Mode.Translate", ezActionScope::Document, "Gizmo", "W", ezToggleWorldSpaceGizmo,
    ezGetStaticRTTI<ezTranslateGizmoEditTool>());
  s_hRotateGizmo = EZ_REGISTER_ACTION_1("Gizmo.Mode.Rotate", ezActionScope::Document, "Gizmo", "E", ezToggleWorldSpaceGizmo,
    ezGetStaticRTTI<ezRotateGizmoEditTool>());
  s_hScaleGizmo = EZ_REGISTER_ACTION_1("Gizmo.Mode.Scale", ezActionScope::Document, "Gizmo", "R", ezGizmoAction,
    ezGetStaticRTTI<ezScaleGizmoEditTool>());
  s_hDragToPositionGizmo = EZ_REGISTER_ACTION_1("Gizmo.Mode.DragToPosition", ezActionScope::Document, "Gizmo", "T", ezGizmoAction,
    ezGetStaticRTTI<ezDragToPositionGizmoEditTool>());
  s_hWorldSpace = EZ_REGISTER_ACTION_1("Gizmo.TransformSpace", ezActionScope::Document, "Gizmo", "", ezTransformGizmoAction,
    ezTransformGizmoAction::ActionType::GizmoToggleWorldSpace);
  s_hMoveParentOnly = EZ_REGISTER_ACTION_1("Gizmo.MoveParentOnly", ezActionScope::Document, "Gizmo", "", ezTransformGizmoAction,
    ezTransformGizmoAction::ActionType::GizmoToggleMoveParentOnly);
  s_SnapSettings = EZ_REGISTER_ACTION_1("Gizmo.SnapSettings", ezActionScope::Document, "Gizmo", "", ezTransformGizmoAction,
    ezTransformGizmoAction::ActionType::GizmoSnapSettings);
}

void ezTransformGizmoActions::UnregisterActions()
{
  ezActionManager::UnregisterAction(s_hGizmoCategory);
  ezActionManager::UnregisterAction(s_hGizmoMenu);
  ezActionManager::UnregisterAction(s_hNoGizmo);
  ezActionManager::UnregisterAction(s_hTranslateGizmo);
  ezActionManager::UnregisterAction(s_hRotateGizmo);
  ezActionManager::UnregisterAction(s_hScaleGizmo);
  ezActionManager::UnregisterAction(s_hDragToPositionGizmo);
  ezActionManager::UnregisterAction(s_hWorldSpace);
  ezActionManager::UnregisterAction(s_hMoveParentOnly);
  ezActionManager::UnregisterAction(s_SnapSettings);
}

void ezTransformGizmoActions::MapMenuActions(const char* szMapping, const char* szPath)
{
  ezActionMap* pMap = ezActionMapManager::GetActionMap(szMapping);
  EZ_ASSERT_DEV(pMap != nullptr, "The given mapping ('{0}') does not exist, mapping the actions failed!", szMapping);

  ezStringBuilder sSubPath(szPath, "/Gizmo.Menu");

  pMap->MapAction(s_hGizmoMenu, szPath, 4.0f);
  pMap->MapAction(s_hNoGizmo, sSubPath, 0.0f);
  pMap->MapAction(s_hTranslateGizmo, sSubPath, 1.0f);
  pMap->MapAction(s_hRotateGizmo, sSubPath, 2.0f);
  pMap->MapAction(s_hScaleGizmo, sSubPath, 3.0f);
  pMap->MapAction(s_hDragToPositionGizmo, sSubPath, 4.0f);
  pMap->MapAction(s_hWorldSpace, sSubPath, 6.0f);
  pMap->MapAction(s_hMoveParentOnly, sSubPath, 7.0f);
  pMap->MapAction(s_SnapSettings, sSubPath, 8.0f);
}

void ezTransformGizmoActions::MapToolbarActions(const char* szMapping, const char* szPath)
{
  ezActionMap* pMap = ezActionMapManager::GetActionMap(szMapping);
  EZ_ASSERT_DEV(pMap != nullptr, "The given mapping ('{0}') does not exist, mapping the actions failed!", szMapping);

  ezStringBuilder sSubPath(szPath, "/GizmoCategory");

  pMap->MapAction(s_hGizmoCategory, szPath, 4.0f);
  pMap->MapAction(s_hNoGizmo, sSubPath, 0.0f);
  pMap->MapAction(s_hTranslateGizmo, sSubPath, 1.0f);
  pMap->MapAction(s_hRotateGizmo, sSubPath, 2.0f);
  pMap->MapAction(s_hScaleGizmo, sSubPath, 3.0f);
  pMap->MapAction(s_hDragToPositionGizmo, sSubPath, 4.0f);
  pMap->MapAction(s_hWorldSpace, sSubPath, 6.0f);
  pMap->MapAction(s_SnapSettings, sSubPath, 7.0f);
}

//////////////////////////////////////////////////////////////////////////

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezTransformGizmoAction, 0, ezRTTINoAllocator);
EZ_END_DYNAMIC_REFLECTED_TYPE;

ezTransformGizmoAction::ezTransformGizmoAction(const ezActionContext& context, const char* szName, ActionType type)
  : ezButtonAction(context, szName, false, "")
{
  SetCheckable(true);
  m_Type = type;
  m_pGameObjectDocument = static_cast<ezGameObjectDocument*>(context.m_pDocument);

  switch (m_Type)
  {
  case ActionType::GizmoToggleWorldSpace:
    SetIconPath(":/EditorFramework/Icons/WorldSpace16.png");
    break;
  case ActionType::GizmoToggleMoveParentOnly:
    SetIconPath(":/EditorFramework/Icons/TransformParent16.png");
    break;
  case ActionType::GizmoSnapSettings:
    SetCheckable(false);
    SetIconPath(":/EditorFramework/Icons/SnapSettings16.png");
    break;
  }

  m_pGameObjectDocument->m_GameObjectEvents.AddEventHandler(ezMakeDelegate(&ezTransformGizmoAction::GameObjectEventHandler, this));
  UpdateState();
}

ezTransformGizmoAction::~ezTransformGizmoAction()
{
  m_pGameObjectDocument->m_GameObjectEvents.RemoveEventHandler(ezMakeDelegate(&ezTransformGizmoAction::GameObjectEventHandler, this));
}

void ezTransformGizmoAction::Execute(const ezVariant& value)
{
  if (m_Type == ActionType::GizmoToggleWorldSpace)
  {
    m_pGameObjectDocument->SetGizmoWorldSpace(value.ConvertTo<bool>());
  }
  else if (m_Type == ActionType::GizmoToggleMoveParentOnly)
  {
    m_pGameObjectDocument->SetGizmoMoveParentOnly(value.ConvertTo<bool>());
  }
  else if (m_Type == ActionType::GizmoSnapSettings)
  {
    ezQtSnapSettingsDlg dlg(nullptr);
    dlg.exec();
  }

  UpdateState();
}

void ezTransformGizmoAction::GameObjectEventHandler(const ezGameObjectEvent& e)
{
  if (e.m_Type == ezGameObjectEvent::Type::ActiveEditToolChanged)
    UpdateState();
}

void ezTransformGizmoAction::UpdateState()
{
  if (m_Type == ActionType::GizmoToggleWorldSpace)
  {
    ezGameObjectEditTool* pTool = m_pGameObjectDocument->GetActiveEditTool();
    SetEnabled(pTool != nullptr && pTool->GetSupportedSpaces() == ezEditToolSupportedSpaces::LocalAndWorldSpace);

    if (pTool != nullptr)
    {
      switch (pTool->GetSupportedSpaces())
      {
      case ezEditToolSupportedSpaces::LocalSpaceOnly:
        SetChecked(false);
        break;
      case ezEditToolSupportedSpaces::WorldSpaceOnly:
        SetChecked(true);
        break;
      case ezEditToolSupportedSpaces::LocalAndWorldSpace:
        SetChecked(m_pGameObjectDocument->GetGizmoWorldSpace());
        break;
      }
    }
  }
  else if (m_Type == ActionType::GizmoToggleMoveParentOnly)
  {
    ezGameObjectEditTool* pTool = m_pGameObjectDocument->GetActiveEditTool();
    const bool bSupported = pTool != nullptr && pTool->GetSupportsMoveParentOnly();

    SetEnabled(bSupported);
    SetChecked(bSupported && m_pGameObjectDocument->GetGizmoMoveParentOnly());
  }
}

//////////////////////////////////////////////////////////////////////////

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezTranslateGizmoAction, 1, ezRTTINoAllocator);
EZ_END_DYNAMIC_REFLECTED_TYPE;

ezActionDescriptorHandle ezTranslateGizmoAction::s_hSnappingValueMenu;
ezActionDescriptorHandle ezTranslateGizmoAction::s_hSnapPivotToGrid;
ezActionDescriptorHandle ezTranslateGizmoAction::s_hSnapObjectsToGrid;

void ezTranslateGizmoAction::RegisterActions()
{
  s_hSnappingValueMenu = EZ_REGISTER_CATEGORY("Gizmo.Translate.Snap.Menu");
  s_hSnapPivotToGrid = EZ_REGISTER_ACTION_1("Gizmo.Translate.Snap.PivotToGrid", ezActionScope::Document, "Gizmo - Position Snap", "Ctrl+End",
    ezTranslateGizmoAction, ezTranslateGizmoAction::ActionType::SnapSelectionPivotToGrid);
  s_hSnapObjectsToGrid =
    EZ_REGISTER_ACTION_1("Gizmo.Translate.Snap.ObjectsToGrid", ezActionScope::Document, "Gizmo - Position Snap", "",
      ezTranslateGizmoAction, ezTranslateGizmoAction::ActionType::SnapEachSelectedObjectToGrid);
}

void ezTranslateGizmoAction::UnregisterActions()
{
  ezActionManager::UnregisterAction(s_hSnappingValueMenu);
  ezActionManager::UnregisterAction(s_hSnapPivotToGrid);
  ezActionManager::UnregisterAction(s_hSnapObjectsToGrid);
}

void ezTranslateGizmoAction::MapActions(const char* szMapping, const char* szPath)
{
  ezActionMap* pMap = ezActionMapManager::GetActionMap(szMapping);
  EZ_ASSERT_DEV(pMap != nullptr, "The given mapping ('{0}') does not exist, mapping the actions failed!", szMapping);

  ezStringBuilder sSubPath(szPath, "/Gizmo.Translate.Snap.Menu");

  pMap->MapAction(s_hSnappingValueMenu, szPath, 8.0f);

  pMap->MapAction(s_hSnapPivotToGrid, sSubPath, 0.0f);
  pMap->MapAction(s_hSnapObjectsToGrid, sSubPath, 1.0f);
}

ezTranslateGizmoAction::ezTranslateGizmoAction(const ezActionContext& context, const char* szName, ActionType type)
  : ezButtonAction(context, szName, false, "")
{
  m_pSceneDocument = static_cast<const ezGameObjectDocument*>(context.m_pDocument);
  m_Type = type;
}

void ezTranslateGizmoAction::Execute(const ezVariant& value)
{
  if (m_Type == ActionType::SnapSelectionPivotToGrid)
    m_pSceneDocument->TriggerSnapPivotToGrid();

  if (m_Type == ActionType::SnapEachSelectedObjectToGrid)
    m_pSceneDocument->TriggerSnapEachObjectToGrid();
}
