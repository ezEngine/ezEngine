#include <PCH.h>
#include <GuiFoundation/Action/ActionManager.h>
#include <GuiFoundation/Action/ActionMapManager.h>
#include <EditorPluginScene/Actions/GizmoActions.h>
#include <EditorPluginScene/Scene/SceneDocument.h>

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezGizmoAction, 0, ezRTTINoAllocator);
EZ_END_DYNAMIC_REFLECTED_TYPE

ezActionDescriptorHandle ezGizmoActions::s_hGizmoCategory;
ezActionDescriptorHandle ezGizmoActions::s_hNoGizmo;
ezActionDescriptorHandle ezGizmoActions::s_hTranslateGizmo;
ezActionDescriptorHandle ezGizmoActions::s_hRotateGizmo;
ezActionDescriptorHandle ezGizmoActions::s_hScaleGizmo;
ezActionDescriptorHandle ezGizmoActions::s_hDragToPositionGizmo;
ezActionDescriptorHandle ezGizmoActions::s_hWorldSpace;

void ezGizmoActions::RegisterActions()
{
  s_hGizmoCategory = EZ_REGISTER_CATEGORY("GizmoCategory");
  s_hNoGizmo = EZ_REGISTER_ACTION_1("Gizmo.Mode.Select", ezActionScope::Document, "Gizmo", "Q", ezGizmoAction, ezGizmoAction::ActionType::GizmoNone);
  s_hTranslateGizmo = EZ_REGISTER_ACTION_1("Gizmo.Mode.Translate", ezActionScope::Document, "Gizmo", "W", ezGizmoAction, ezGizmoAction::ActionType::GizmoTranslate);
  s_hRotateGizmo = EZ_REGISTER_ACTION_1("Gizmo.Mode.Rotate", ezActionScope::Document, "Gizmo", "E", ezGizmoAction, ezGizmoAction::ActionType::GizmoRotate);
  s_hScaleGizmo = EZ_REGISTER_ACTION_1("Gizmo.Mode.Scale", ezActionScope::Document, "Gizmo", "R", ezGizmoAction, ezGizmoAction::ActionType::GizmoScale);
  s_hDragToPositionGizmo = EZ_REGISTER_ACTION_1("Gizmo.Mode.DragToPosition", ezActionScope::Document, "Gizmo", "T", ezGizmoAction, ezGizmoAction::ActionType::GizmoDragToPosition);
  s_hWorldSpace = EZ_REGISTER_ACTION_1("Gizmo.TransformSpace", ezActionScope::Document, "Gizmo", "", ezGizmoAction, ezGizmoAction::ActionType::GizmoToggleWorldSpace);
}

void ezGizmoActions::UnregisterActions()
{
  ezActionManager::UnregisterAction(s_hGizmoCategory);
  ezActionManager::UnregisterAction(s_hNoGizmo);
  ezActionManager::UnregisterAction(s_hTranslateGizmo);
  ezActionManager::UnregisterAction(s_hRotateGizmo);
  ezActionManager::UnregisterAction(s_hScaleGizmo);
  ezActionManager::UnregisterAction(s_hDragToPositionGizmo);
  ezActionManager::UnregisterAction(s_hWorldSpace);
}

void ezGizmoActions::MapActions(const char* szMapping, const char* szPath)
{
  ezActionMap* pMap = ezActionMapManager::GetActionMap(szMapping);
  EZ_ASSERT_DEV(pMap != nullptr, "The given mapping ('%s') does not exist, mapping the actions failed!", szMapping);

  ezStringBuilder sSubPath(szPath, "/GizmoCategory");

  pMap->MapAction(s_hGizmoCategory, szPath, 4.0f);
  pMap->MapAction(s_hNoGizmo, sSubPath, 0.0f);
  pMap->MapAction(s_hTranslateGizmo, sSubPath, 1.0f);
  pMap->MapAction(s_hRotateGizmo, sSubPath, 2.0f);
  pMap->MapAction(s_hScaleGizmo, sSubPath, 3.0f);
  pMap->MapAction(s_hDragToPositionGizmo, sSubPath, 4.0f);
  pMap->MapAction(s_hWorldSpace, sSubPath, 5.0f);
}

ezGizmoAction::ezGizmoAction(const ezActionContext& context, const char* szName, ActionType type) : ezButtonAction(context, szName, false, "")
{
  SetCheckable(true);
  m_Type = type;
  m_pSceneDocument = static_cast<ezSceneDocument*>(context.m_pDocument);
  m_pSceneDocument->m_SceneEvents.AddEventHandler(ezMakeDelegate(&ezGizmoAction::SceneEventHandler, this));

  switch (m_Type)
  {
  case ActionType::GizmoNone:
    SetIconPath(":/EditorPluginScene/Icons/GizmoNone24.png");
    break;
  case ActionType::GizmoTranslate:
    SetIconPath(":/EditorPluginScene/Icons/GizmoTranslate24.png");
    break;
  case ActionType::GizmoRotate:
    SetIconPath(":/EditorPluginScene/Icons/GizmoRotate24.png");
    break;
  case ActionType::GizmoScale:
    SetIconPath(":/EditorPluginScene/Icons/GizmoScale24.png");
    break;
  case ActionType::GizmoDragToPosition:
    SetIconPath(":/EditorPluginScene/Icons/GizmoDragPosition24.png");
    break;
  case ActionType::GizmoToggleWorldSpace:
    SetIconPath(":/EditorPluginScene/Icons/WorldSpace16.png");
    break;
  }

  UpdateState();
}

ezGizmoAction::~ezGizmoAction()
{
  m_pSceneDocument->m_SceneEvents.RemoveEventHandler(ezMakeDelegate(&ezGizmoAction::SceneEventHandler, this));
}

void ezGizmoAction::Execute(const ezVariant& value)
{
  if (m_Type == ActionType::GizmoToggleWorldSpace)
  {
    m_pSceneDocument->SetGizmoWorldSpace(value.ConvertTo<bool>());
  }
  else
  {
    if ((m_Type == ActionType::GizmoTranslate || m_Type == ActionType::GizmoRotate) && m_pSceneDocument->GetActiveGizmo() == (ActiveGizmo)((int)m_Type))
      m_pSceneDocument->SetGizmoWorldSpace(!m_pSceneDocument->GetGizmoWorldSpace());
    else
      m_pSceneDocument->SetActiveGizmo((ActiveGizmo)((int)m_Type));
  }

  UpdateState();
}

void ezGizmoAction::UpdateState()
{
  if (m_Type == ActionType::GizmoToggleWorldSpace)
  {
    SetEnabled(m_pSceneDocument->GetActiveGizmo() == ActiveGizmo::Translate ||
               m_pSceneDocument->GetActiveGizmo() == ActiveGizmo::Rotate);

    SetChecked(m_pSceneDocument->GetGizmoWorldSpace() && m_pSceneDocument->GetActiveGizmo() != ActiveGizmo::Scale);
  }
  else
  {
    SetChecked((int)m_pSceneDocument->GetActiveGizmo() == (int)m_Type);
  }
}

void ezGizmoAction::SceneEventHandler(const ezSceneDocumentEvent& e)
{
  if (e.m_Type == ezSceneDocumentEvent::Type::ActiveGizmoChanged)
    UpdateState();

}






EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezRotateGizmoAction, 1, ezRTTINoAllocator);
EZ_END_DYNAMIC_REFLECTED_TYPE

ezActionDescriptorHandle ezRotateGizmoAction::s_hSnappingValueMenu;
ezActionDescriptorHandle ezRotateGizmoAction::s_hSnappingValues[11];
float ezRotateGizmoAction::s_fCurrentSnappingValue = 15.0f;
ezEvent<const ezRotateGizmoAction::Event&> ezRotateGizmoAction::s_Events;

void ezRotateGizmoAction::RegisterActions()
{
  s_hSnappingValueMenu = EZ_REGISTER_MENU_WITH_ICON("Gizmo.Rotation.Snap.Menu", ":/EditorPluginScene/Icons/GizmoRotate24.png");
  s_hSnappingValues[0] = EZ_REGISTER_ACTION_2("Gizmo.Rotation.Snap.0_Degree", ezActionScope::Document, "Gizmo - Rotation Snap", "", ezRotateGizmoAction, ezRotateGizmoAction::ActionType::SetSnappingAngle, 0.0f);
  s_hSnappingValues[1] = EZ_REGISTER_ACTION_2("Gizmo.Rotation.Snap.1_Degree", ezActionScope::Document, "Gizmo - Rotation Snap", "", ezRotateGizmoAction, ezRotateGizmoAction::ActionType::SetSnappingAngle, 1.0f);
  s_hSnappingValues[2] = EZ_REGISTER_ACTION_2("Gizmo.Rotation.Snap.5_Degree", ezActionScope::Document, "Gizmo - Rotation Snap", "", ezRotateGizmoAction, ezRotateGizmoAction::ActionType::SetSnappingAngle, 5.0f);
  s_hSnappingValues[3] = EZ_REGISTER_ACTION_2("Gizmo.Rotation.Snap.10_Degree", ezActionScope::Document, "Gizmo - Rotation Snap", "", ezRotateGizmoAction, ezRotateGizmoAction::ActionType::SetSnappingAngle, 10.0f);
  s_hSnappingValues[4] = EZ_REGISTER_ACTION_2("Gizmo.Rotation.Snap.15_Degree", ezActionScope::Document, "Gizmo - Rotation Snap", "", ezRotateGizmoAction, ezRotateGizmoAction::ActionType::SetSnappingAngle, 15.0f);
  s_hSnappingValues[5] = EZ_REGISTER_ACTION_2("Gizmo.Rotation.Snap.30_Degree", ezActionScope::Document, "Gizmo - Rotation Snap", "", ezRotateGizmoAction, ezRotateGizmoAction::ActionType::SetSnappingAngle, 30.0f);

  s_hSnappingValues[6] = EZ_REGISTER_ACTION_2("Gizmo.Rotation.Snap.2_8125_Degree", ezActionScope::Document, "Gizmo - Rotation Snap", "", ezRotateGizmoAction, ezRotateGizmoAction::ActionType::SetSnappingAngle, 2.8125f);
  s_hSnappingValues[7] = EZ_REGISTER_ACTION_2("Gizmo.Rotation.Snap.5_625_Degree", ezActionScope::Document, "Gizmo - Rotation Snap", "", ezRotateGizmoAction, ezRotateGizmoAction::ActionType::SetSnappingAngle, 5.625f);
  s_hSnappingValues[8] = EZ_REGISTER_ACTION_2("Gizmo.Rotation.Snap.11_25_Degree", ezActionScope::Document, "Gizmo - Rotation Snap", "", ezRotateGizmoAction, ezRotateGizmoAction::ActionType::SetSnappingAngle, 11.25f);
  s_hSnappingValues[9] = EZ_REGISTER_ACTION_2("Gizmo.Rotation.Snap.22_5_Degree", ezActionScope::Document, "Gizmo - Rotation Snap", "", ezRotateGizmoAction, ezRotateGizmoAction::ActionType::SetSnappingAngle, 22.5f);
  s_hSnappingValues[10]= EZ_REGISTER_ACTION_2("Gizmo.Rotation.Snap.45_Degree", ezActionScope::Document, "Gizmo - Rotation Snap", "", ezRotateGizmoAction, ezRotateGizmoAction::ActionType::SetSnappingAngle, 45.0f);
}

void ezRotateGizmoAction::UnregisterActions()
{
  ezActionManager::UnregisterAction(s_hSnappingValueMenu);

  for (ezUInt32 i = 0; i < EZ_ARRAY_SIZE(s_hSnappingValues); ++i)
    ezActionManager::UnregisterAction(s_hSnappingValues[i]);
}

void ezRotateGizmoAction::MapActions(const char* szMapping, const char* szPath)
{
  ezActionMap* pMap = ezActionMapManager::GetActionMap(szMapping);
  EZ_ASSERT_DEV(pMap != nullptr, "The given mapping ('%s') does not exist, mapping the actions failed!", szMapping);

  ezStringBuilder sSubPath(szPath, "/Gizmo.Rotation.Snap.Menu");

  pMap->MapAction(s_hSnappingValueMenu, szPath, 6.0f);

  for (ezUInt32 i = 0; i < EZ_ARRAY_SIZE(s_hSnappingValues); ++i)
    pMap->MapAction(s_hSnappingValues[i], sSubPath, i + 1.0f);
}

ezRotateGizmoAction::ezRotateGizmoAction(const ezActionContext& context, const char* szName, ActionType type, float fSnappingValue) : ezButtonAction(context, szName, false, "")
{
  SetCheckable(true);
  m_Type = type;
  m_fSnappingValue = fSnappingValue;

  SetChecked(m_fSnappingValue == GetCurrentSnappingValue());

  s_Events.AddEventHandler(ezMakeDelegate(&ezRotateGizmoAction::EventHandler, this));
}

ezRotateGizmoAction::~ezRotateGizmoAction()
{
  s_Events.RemoveEventHandler(ezMakeDelegate(&ezRotateGizmoAction::EventHandler, this));
}

void ezRotateGizmoAction::EventHandler(const Event& e)
{
  switch (e.m_Type)
  {
  case Event::Type::SnapppingAngleChanged:
    {
      SetChecked(m_fSnappingValue == GetCurrentSnappingValue());
    }
    break;
  }
}

void ezRotateGizmoAction::SetCurrentSnappingValue(float f)
{
  s_fCurrentSnappingValue = f;

  Event e;
  e.m_Type = Event::Type::SnapppingAngleChanged;

  s_Events.Broadcast(e);
}

void ezRotateGizmoAction::Execute(const ezVariant& value)
{
  if (m_Type == ActionType::SetSnappingAngle)
  {
    SetCurrentSnappingValue(m_fSnappingValue);
  }
}







EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezScaleGizmoAction, 1, ezRTTINoAllocator);
EZ_END_DYNAMIC_REFLECTED_TYPE

ezActionDescriptorHandle ezScaleGizmoAction::s_hSnappingValueMenu;
ezActionDescriptorHandle ezScaleGizmoAction::s_hSnappingValues[8];
float ezScaleGizmoAction::s_fCurrentSnappingValue = 0.0f;
ezEvent<const ezScaleGizmoAction::Event&> ezScaleGizmoAction::s_Events;

void ezScaleGizmoAction::RegisterActions()
{
  s_hSnappingValueMenu = EZ_REGISTER_MENU_WITH_ICON("Gizmo.Scale.Snap.Menu", ":/EditorPluginScene/Icons/GizmoScale24.png");
  s_hSnappingValues[0] = EZ_REGISTER_ACTION_2("Gizmo.Scale.Snap.0", ezActionScope::Document, "Gizmo - Scale Snap", "", ezScaleGizmoAction, ezScaleGizmoAction::ActionType::SetSnappingValue, 0.0f);
  s_hSnappingValues[1] = EZ_REGISTER_ACTION_2("Gizmo.Scale.Snap.8", ezActionScope::Document, "Gizmo - Scale Snap", "", ezScaleGizmoAction, ezScaleGizmoAction::ActionType::SetSnappingValue, 8.0f);
  s_hSnappingValues[2] = EZ_REGISTER_ACTION_2("Gizmo.Scale.Snap.4", ezActionScope::Document, "Gizmo - Scale Snap", "", ezScaleGizmoAction, ezScaleGizmoAction::ActionType::SetSnappingValue, 4.0f);
  s_hSnappingValues[3] = EZ_REGISTER_ACTION_2("Gizmo.Scale.Snap.2", ezActionScope::Document, "Gizmo - Scale Snap", "", ezScaleGizmoAction, ezScaleGizmoAction::ActionType::SetSnappingValue, 2.0f);
  s_hSnappingValues[4] = EZ_REGISTER_ACTION_2("Gizmo.Scale.Snap.1", ezActionScope::Document, "Gizmo - Scale Snap", "", ezScaleGizmoAction, ezScaleGizmoAction::ActionType::SetSnappingValue, 1.0f);
  s_hSnappingValues[5] = EZ_REGISTER_ACTION_2("Gizmo.Scale.Snap.0_5", ezActionScope::Document, "Gizmo - Scale Snap", "", ezScaleGizmoAction, ezScaleGizmoAction::ActionType::SetSnappingValue, 0.5f);
  s_hSnappingValues[6] = EZ_REGISTER_ACTION_2("Gizmo.Scale.Snap.0_25", ezActionScope::Document, "Gizmo - Scale Snap", "", ezScaleGizmoAction, ezScaleGizmoAction::ActionType::SetSnappingValue, 0.25f);
  s_hSnappingValues[7] = EZ_REGISTER_ACTION_2("Gizmo.Scale.Snap.0_125", ezActionScope::Document, "Gizmo - Scale Snap", "", ezScaleGizmoAction, ezScaleGizmoAction::ActionType::SetSnappingValue, 0.125f);
}

void ezScaleGizmoAction::UnregisterActions()
{
  ezActionManager::UnregisterAction(s_hSnappingValueMenu);

  for (ezUInt32 i = 0; i < EZ_ARRAY_SIZE(s_hSnappingValues); ++i)
    ezActionManager::UnregisterAction(s_hSnappingValues[i]);
}

void ezScaleGizmoAction::MapActions(const char* szMapping, const char* szPath)
{
  ezActionMap* pMap = ezActionMapManager::GetActionMap(szMapping);
  EZ_ASSERT_DEV(pMap != nullptr, "The given mapping ('%s') does not exist, mapping the actions failed!", szMapping);

  ezStringBuilder sSubPath(szPath, "/Gizmo.Scale.Snap.Menu");

  pMap->MapAction(s_hSnappingValueMenu, szPath, 7.0f);

  for (ezUInt32 i = 0; i < EZ_ARRAY_SIZE(s_hSnappingValues); ++i)
    pMap->MapAction(s_hSnappingValues[i], sSubPath, i + 1.0f);
}

ezScaleGizmoAction::ezScaleGizmoAction(const ezActionContext& context, const char* szName, ActionType type, float fSnappingValue) : ezButtonAction(context, szName, false, "")
{
  SetCheckable(true);
  m_Type = type;
  m_fSnappingValue = fSnappingValue;

  SetChecked(m_fSnappingValue == GetCurrentSnappingValue());

  s_Events.AddEventHandler(ezMakeDelegate(&ezScaleGizmoAction::EventHandler, this));
}

ezScaleGizmoAction::~ezScaleGizmoAction()
{
  s_Events.RemoveEventHandler(ezMakeDelegate(&ezScaleGizmoAction::EventHandler, this));
}

void ezScaleGizmoAction::EventHandler(const Event& e)
{
  switch (e.m_Type)
  {
  case Event::Type::SnapppingValueChanged:
    {
      SetChecked(m_fSnappingValue == GetCurrentSnappingValue());
    }
    break;
  }
}

void ezScaleGizmoAction::SetCurrentSnappingValue(float f)
{
  s_fCurrentSnappingValue = f;

  Event e;
  e.m_Type = Event::Type::SnapppingValueChanged;

  s_Events.Broadcast(e);
}

void ezScaleGizmoAction::Execute(const ezVariant& value)
{
  if (m_Type == ActionType::SetSnappingValue)
  {
    SetCurrentSnappingValue(m_fSnappingValue);
  }
}






EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezTranslateGizmoAction, 1, ezRTTINoAllocator);
EZ_END_DYNAMIC_REFLECTED_TYPE

ezActionDescriptorHandle ezTranslateGizmoAction::s_hSnappingValueMenu;
ezActionDescriptorHandle ezTranslateGizmoAction::s_hSnapPivotToGrid;
ezActionDescriptorHandle ezTranslateGizmoAction::s_hSnapObjectsToGrid;
ezActionDescriptorHandle ezTranslateGizmoAction::s_hSnappingValues[9];
float ezTranslateGizmoAction::s_fCurrentSnappingValue = 0.1f;
ezEvent<const ezTranslateGizmoAction::Event&> ezTranslateGizmoAction::s_Events;

void ezTranslateGizmoAction::RegisterActions()
{
  s_hSnappingValueMenu = EZ_REGISTER_MENU_WITH_ICON("Gizmo.Translate.Snap.Menu", ":/EditorPluginScene/Icons/GizmoTranslate24.png");
  s_hSnapPivotToGrid   = EZ_REGISTER_ACTION_2("Gizmo.Translate.Snap.PivotToGrid", ezActionScope::Document, "Gizmo - Position Snap", "Ctrl+G", ezTranslateGizmoAction, ezTranslateGizmoAction::ActionType::SnapSelectionPivotToGrid, 0.0f);
  s_hSnapObjectsToGrid = EZ_REGISTER_ACTION_2("Gizmo.Translate.Snap.ObjectsToGrid", ezActionScope::Document, "Gizmo - Position Snap", "Shift+G", ezTranslateGizmoAction, ezTranslateGizmoAction::ActionType::SnapEachSelectedObjectToGrid, 0.0f);
  s_hSnappingValues[0] = EZ_REGISTER_ACTION_2("Gizmo.Translate.Snap.0", ezActionScope::Document, "Gizmo - Position Snap", "", ezTranslateGizmoAction, ezTranslateGizmoAction::ActionType::SetSnappingValue, 0.0f);
  s_hSnappingValues[1] = EZ_REGISTER_ACTION_2("Gizmo.Translate.Snap.10", ezActionScope::Document, "Gizmo - Position Snap", "", ezTranslateGizmoAction, ezTranslateGizmoAction::ActionType::SetSnappingValue, 10.0f);
  s_hSnappingValues[2] = EZ_REGISTER_ACTION_2("Gizmo.Translate.Snap.5", ezActionScope::Document, "Gizmo - Position Snap", "", ezTranslateGizmoAction, ezTranslateGizmoAction::ActionType::SetSnappingValue, 5.0f);
  s_hSnappingValues[3] = EZ_REGISTER_ACTION_2("Gizmo.Translate.Snap.1", ezActionScope::Document, "Gizmo - Position Snap", "", ezTranslateGizmoAction, ezTranslateGizmoAction::ActionType::SetSnappingValue, 1.0f);
  s_hSnappingValues[4] = EZ_REGISTER_ACTION_2("Gizmo.Translate.Snap.0_5", ezActionScope::Document, "Gizmo - Position Snap", "", ezTranslateGizmoAction, ezTranslateGizmoAction::ActionType::SetSnappingValue, 0.5f);
  s_hSnappingValues[5] = EZ_REGISTER_ACTION_2("Gizmo.Translate.Snap.0_25", ezActionScope::Document, "Gizmo - Position Snap", "", ezTranslateGizmoAction, ezTranslateGizmoAction::ActionType::SetSnappingValue, 0.25f);
  s_hSnappingValues[6] = EZ_REGISTER_ACTION_2("Gizmo.Translate.Snap.0_2", ezActionScope::Document, "Gizmo - Position Snap", "", ezTranslateGizmoAction, ezTranslateGizmoAction::ActionType::SetSnappingValue, 0.2f);
  s_hSnappingValues[7] = EZ_REGISTER_ACTION_2("Gizmo.Translate.Snap.0_125", ezActionScope::Document, "Gizmo - Position Snap", "", ezTranslateGizmoAction, ezTranslateGizmoAction::ActionType::SetSnappingValue, 0.125f);
  s_hSnappingValues[8] = EZ_REGISTER_ACTION_2("Gizmo.Translate.Snap.0_1", ezActionScope::Document, "Gizmo - Position Snap", "", ezTranslateGizmoAction, ezTranslateGizmoAction::ActionType::SetSnappingValue, 0.1f);
}

void ezTranslateGizmoAction::UnregisterActions()
{
  ezActionManager::UnregisterAction(s_hSnappingValueMenu);

  for (ezUInt32 i = 0; i < EZ_ARRAY_SIZE(s_hSnappingValues); ++i)
    ezActionManager::UnregisterAction(s_hSnappingValues[i]);
}

void ezTranslateGizmoAction::MapActions(const char* szMapping, const char* szPath)
{
  ezActionMap* pMap = ezActionMapManager::GetActionMap(szMapping);
  EZ_ASSERT_DEV(pMap != nullptr, "The given mapping ('%s') does not exist, mapping the actions failed!", szMapping);

  ezStringBuilder sSubPath(szPath, "/Gizmo.Translate.Snap.Menu");

  pMap->MapAction(s_hSnappingValueMenu, szPath, 5.0f);

  pMap->MapAction(s_hSnapPivotToGrid, sSubPath, 0.0f);
  pMap->MapAction(s_hSnapObjectsToGrid, sSubPath, 1.0f);

  for (ezUInt32 i = 0; i < EZ_ARRAY_SIZE(s_hSnappingValues); ++i)
    pMap->MapAction(s_hSnappingValues[i], sSubPath, i + 2.0f);
}

ezTranslateGizmoAction::ezTranslateGizmoAction(const ezActionContext& context, const char* szName, ActionType type, float fSnappingValue) : ezButtonAction(context, szName, false, "")
{
  m_pSceneDocument = static_cast<ezSceneDocument*>(context.m_pDocument);
  m_Type = type;
  m_fSnappingValue = fSnappingValue;

  if (m_Type == ActionType::SetSnappingValue)
  {
    SetCheckable(true);
    SetChecked(m_fSnappingValue == GetCurrentSnappingValue());
  }

  s_Events.AddEventHandler(ezMakeDelegate(&ezTranslateGizmoAction::EventHandler, this));
}

ezTranslateGizmoAction::~ezTranslateGizmoAction()
{
  s_Events.RemoveEventHandler(ezMakeDelegate(&ezTranslateGizmoAction::EventHandler, this));
}

void ezTranslateGizmoAction::EventHandler(const Event& e)
{
  switch (e.m_Type)
  {
  case Event::Type::SnapppingValueChanged:
    {
      SetChecked(m_fSnappingValue == GetCurrentSnappingValue());
    }
    break;
  }
}

void ezTranslateGizmoAction::SetCurrentSnappingValue(float f)
{
  s_fCurrentSnappingValue = f;

  Event e;
  e.m_Type = Event::Type::SnapppingValueChanged;

  s_Events.Broadcast(e);
}

void ezTranslateGizmoAction::Execute(const ezVariant& value)
{
  if (m_Type == ActionType::SetSnappingValue)
    SetCurrentSnappingValue(m_fSnappingValue);

  if (m_Type == ActionType::SnapSelectionPivotToGrid)
    m_pSceneDocument->TriggerSnapPivotToGrid();

  if (m_Type == ActionType::SnapEachSelectedObjectToGrid)
    m_pSceneDocument->TriggerSnapEachObjectToGrid();
}