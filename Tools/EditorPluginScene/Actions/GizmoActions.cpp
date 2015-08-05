#include <PCH.h>
#include <GuiFoundation/Action/ActionManager.h>
#include <GuiFoundation/Action/ActionMapManager.h>
#include <EditorPluginScene/Actions/GizmoActions.h>
#include <EditorPluginScene/Scene/SceneDocument.h>

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezGizmoAction, ezButtonAction, 0, ezRTTINoAllocator);
EZ_END_DYNAMIC_REFLECTED_TYPE();

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
  s_hNoGizmo = EZ_REGISTER_ACTION_1("Select", "Select", ezActionScope::Document, "Document", "Q", ezGizmoAction, ezGizmoAction::ActionType::GizmoNone);
  s_hTranslateGizmo = EZ_REGISTER_ACTION_1("Translate", "Translate", ezActionScope::Document, "Document", "W", ezGizmoAction, ezGizmoAction::ActionType::GizmoTranslate);
  s_hRotateGizmo = EZ_REGISTER_ACTION_1("Rotate", "Rotate", ezActionScope::Document, "Document", "E", ezGizmoAction, ezGizmoAction::ActionType::GizmoRotate);
  s_hScaleGizmo = EZ_REGISTER_ACTION_1("Scale", "Scale", ezActionScope::Document, "Document", "R", ezGizmoAction, ezGizmoAction::ActionType::GizmoScale);
  s_hDragToPositionGizmo = EZ_REGISTER_ACTION_1("DragToPosition", "Drag To Position", ezActionScope::Document, "Document", "T", ezGizmoAction, ezGizmoAction::ActionType::GizmoDragToPosition);
  s_hWorldSpace = EZ_REGISTER_ACTION_1("TransformSpace", "Transform in World Space", ezActionScope::Document, "Document", "", ezGizmoAction, ezGizmoAction::ActionType::GizmoToggleWorldSpace);
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
    SetIconPath(":/GuiFoundation/Icons/GizmoNone24.png");
    break;
  case ActionType::GizmoTranslate:
    SetIconPath(":/GuiFoundation/Icons/GizmoTranslate24.png");
    break;
  case ActionType::GizmoRotate:
    SetIconPath(":/GuiFoundation/Icons/GizmoRotate24.png");
    break;
  case ActionType::GizmoScale:
    SetIconPath(":/GuiFoundation/Icons/GizmoScale24.png");
    break;
  case ActionType::GizmoDragToPosition:
    SetIconPath(":/GuiFoundation/Icons/GizmoDragPosition24.png");
    break;
  case ActionType::GizmoToggleWorldSpace:
    SetIconPath(":/GuiFoundation/Icons/WorldSpace16.png");
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

void ezGizmoAction::SceneEventHandler(const ezSceneDocument::SceneEvent& e)
{
  if (e.m_Type == ezSceneDocument::SceneEvent::Type::ActiveGizmoChanged)
    UpdateState();

}






EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezRotateGizmoAction, ezButtonAction, 0, ezRTTINoAllocator);
EZ_END_DYNAMIC_REFLECTED_TYPE();

ezActionDescriptorHandle ezRotateGizmoAction::s_hSnappingValueMenu;
ezActionDescriptorHandle ezRotateGizmoAction::s_hSnappingValues[11];
float ezRotateGizmoAction::s_fCurrentSnappingValue = 10.0f;
ezEvent<const ezRotateGizmoAction::Event&> ezRotateGizmoAction::s_Events;

void ezRotateGizmoAction::RegisterActions()
{
  s_hSnappingValueMenu = EZ_REGISTER_MENU_WITH_ICON("RotationSnapCategory", "Rotation Snap", ":/GuiFoundation/Icons/GizmoRotate24.png");
  s_hSnappingValues[0] = EZ_REGISTER_ACTION_2("Snap_0_Degree", "No Rotation Snap", ezActionScope::Document, "Rotation Snap", "", ezRotateGizmoAction, ezRotateGizmoAction::ActionType::SetSnappingAngle, 0.0f);
  s_hSnappingValues[1] = EZ_REGISTER_ACTION_2("Snap_1_Degree", "1 Degree", ezActionScope::Document, "Rotation Snap", "", ezRotateGizmoAction, ezRotateGizmoAction::ActionType::SetSnappingAngle, 1.0f);
  s_hSnappingValues[2] = EZ_REGISTER_ACTION_2("Snap_5_Degree", "5 Degree", ezActionScope::Document, "Rotation Snap", "", ezRotateGizmoAction, ezRotateGizmoAction::ActionType::SetSnappingAngle, 5.0f);
  s_hSnappingValues[3] = EZ_REGISTER_ACTION_2("Snap_10_Degree", "10 Degree", ezActionScope::Document, "Rotation Snap", "", ezRotateGizmoAction, ezRotateGizmoAction::ActionType::SetSnappingAngle, 10.0f);
  s_hSnappingValues[4] = EZ_REGISTER_ACTION_2("Snap_15_Degree", "15 Degree", ezActionScope::Document, "Rotation Snap", "", ezRotateGizmoAction, ezRotateGizmoAction::ActionType::SetSnappingAngle, 15.0f);
  s_hSnappingValues[5] = EZ_REGISTER_ACTION_2("Snap_30_Degree", "30 Degree", ezActionScope::Document, "Rotation Snap", "", ezRotateGizmoAction, ezRotateGizmoAction::ActionType::SetSnappingAngle, 30.0f);

  s_hSnappingValues[6] = EZ_REGISTER_ACTION_2("Snap_2_8125_Degree", "2.8125 Degree (1/128)", ezActionScope::Document, "Rotation Snap", "", ezRotateGizmoAction, ezRotateGizmoAction::ActionType::SetSnappingAngle, 2.8125f);
  s_hSnappingValues[7] = EZ_REGISTER_ACTION_2("Snap_5_625_Degree", "5.625 Degree (1/64)", ezActionScope::Document, "Rotation Snap", "", ezRotateGizmoAction, ezRotateGizmoAction::ActionType::SetSnappingAngle, 5.625f);
  s_hSnappingValues[8] = EZ_REGISTER_ACTION_2("Snap_11_25_Degree", "11.25 Degree (1/32)", ezActionScope::Document, "Rotation Snap", "", ezRotateGizmoAction, ezRotateGizmoAction::ActionType::SetSnappingAngle, 11.25f);
  s_hSnappingValues[9] = EZ_REGISTER_ACTION_2("Snap_22_5_Degree", "22.5 Degree (1/16)", ezActionScope::Document, "Rotation Snap", "", ezRotateGizmoAction, ezRotateGizmoAction::ActionType::SetSnappingAngle, 22.5f);
  s_hSnappingValues[10] = EZ_REGISTER_ACTION_2("Snap_45_Degree", "45 Degree (1/8)", ezActionScope::Document, "Rotation Snap", "", ezRotateGizmoAction, ezRotateGizmoAction::ActionType::SetSnappingAngle, 45.0f);
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

  ezStringBuilder sSubPath(szPath, "/RotationSnapCategory");

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







EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezScaleGizmoAction, ezButtonAction, 0, ezRTTINoAllocator);
EZ_END_DYNAMIC_REFLECTED_TYPE();

ezActionDescriptorHandle ezScaleGizmoAction::s_hSnappingValueMenu;
ezActionDescriptorHandle ezScaleGizmoAction::s_hSnappingValues[8];
float ezScaleGizmoAction::s_fCurrentSnappingValue = 0.0f;
ezEvent<const ezScaleGizmoAction::Event&> ezScaleGizmoAction::s_Events;

void ezScaleGizmoAction::RegisterActions()
{
  s_hSnappingValueMenu = EZ_REGISTER_MENU_WITH_ICON("ScalingSnapCategory", "Scaling Factor", ":/GuiFoundation/Icons/GizmoScale24.png");
  s_hSnappingValues[0] = EZ_REGISTER_ACTION_2("SnapFactor_0", "No Scale Snap", ezActionScope::Document, "Scale Snap", "", ezScaleGizmoAction, ezScaleGizmoAction::ActionType::SetSnappingValue, 0.0f);
  s_hSnappingValues[1] = EZ_REGISTER_ACTION_2("SnapFactor_8", "8", ezActionScope::Document, "Scale Snap", "", ezScaleGizmoAction, ezScaleGizmoAction::ActionType::SetSnappingValue, 8.0f);
  s_hSnappingValues[2] = EZ_REGISTER_ACTION_2("SnapFactor_4", "4", ezActionScope::Document, "Scale Snap", "", ezScaleGizmoAction, ezScaleGizmoAction::ActionType::SetSnappingValue, 4.0f);
  s_hSnappingValues[3] = EZ_REGISTER_ACTION_2("SnapFactor_2", "2", ezActionScope::Document, "Scale Snap", "", ezScaleGizmoAction, ezScaleGizmoAction::ActionType::SetSnappingValue, 2.0f);
  s_hSnappingValues[4] = EZ_REGISTER_ACTION_2("SnapFactor_1", "1", ezActionScope::Document, "Scale Snap", "", ezScaleGizmoAction, ezScaleGizmoAction::ActionType::SetSnappingValue, 1.0f);
  s_hSnappingValues[5] = EZ_REGISTER_ACTION_2("SnapFactor_0_5", "0.5", ezActionScope::Document, "Scale Snap", "", ezScaleGizmoAction, ezScaleGizmoAction::ActionType::SetSnappingValue, 0.5f);
  s_hSnappingValues[6] = EZ_REGISTER_ACTION_2("SnapFactor_0_25", "0.25", ezActionScope::Document, "Scale Snap", "", ezScaleGizmoAction, ezScaleGizmoAction::ActionType::SetSnappingValue, 0.25f);
  s_hSnappingValues[7] = EZ_REGISTER_ACTION_2("SnapFactor_0_125", "0.125", ezActionScope::Document, "Scale Snap", "", ezScaleGizmoAction, ezScaleGizmoAction::ActionType::SetSnappingValue, 0.125f);
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

  ezStringBuilder sSubPath(szPath, "/ScalingSnapCategory");

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






EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezTranslateGizmoAction, ezButtonAction, 0, ezRTTINoAllocator);
EZ_END_DYNAMIC_REFLECTED_TYPE();

ezActionDescriptorHandle ezTranslateGizmoAction::s_hSnappingValueMenu;
ezActionDescriptorHandle ezTranslateGizmoAction::s_hSnapPivotToGrid;
ezActionDescriptorHandle ezTranslateGizmoAction::s_hSnapObjectsToGrid;
ezActionDescriptorHandle ezTranslateGizmoAction::s_hSnappingValues[9];
float ezTranslateGizmoAction::s_fCurrentSnappingValue = 0.1f;
ezEvent<const ezTranslateGizmoAction::Event&> ezTranslateGizmoAction::s_Events;

void ezTranslateGizmoAction::RegisterActions()
{
  s_hSnappingValueMenu = EZ_REGISTER_MENU_WITH_ICON("PositionSnapCategory", "Snap Position", ":/GuiFoundation/Icons/GizmoTranslate24.png");
  s_hSnapPivotToGrid   = EZ_REGISTER_ACTION_2("SnapPivotToGrid", "Snap Pivot To Grid", ezActionScope::Document, "Position Snap", "Ctrl+G", ezTranslateGizmoAction, ezTranslateGizmoAction::ActionType::SnapSelectionPivotToGrid, 0.0f);
  s_hSnapObjectsToGrid = EZ_REGISTER_ACTION_2("SnapObjectsToGrid", "Snap Each Object To Grid", ezActionScope::Document, "Position Snap", "Shift+G", ezTranslateGizmoAction, ezTranslateGizmoAction::ActionType::SnapEachSelectedObjectToGrid, 0.0f);
  s_hSnappingValues[0] = EZ_REGISTER_ACTION_2("SnapPos_0", "No Position Snap", ezActionScope::Document, "Position Snap", "", ezTranslateGizmoAction, ezTranslateGizmoAction::ActionType::SetSnappingValue, 0.0f);
  s_hSnappingValues[1] = EZ_REGISTER_ACTION_2("SnapPos_10",       "10", ezActionScope::Document, "Position Snap", "", ezTranslateGizmoAction, ezTranslateGizmoAction::ActionType::SetSnappingValue, 10.0f);
  s_hSnappingValues[2] = EZ_REGISTER_ACTION_2("SnapPos_5",         "5", ezActionScope::Document, "Position Snap", "", ezTranslateGizmoAction, ezTranslateGizmoAction::ActionType::SetSnappingValue, 5.0f);
  s_hSnappingValues[3] = EZ_REGISTER_ACTION_2("SnapPos_1",         "1", ezActionScope::Document, "Position Snap", "", ezTranslateGizmoAction, ezTranslateGizmoAction::ActionType::SetSnappingValue, 1.0f);
  s_hSnappingValues[4] = EZ_REGISTER_ACTION_2("SnapPos_0_5",     "0.5", ezActionScope::Document, "Position Snap", "", ezTranslateGizmoAction, ezTranslateGizmoAction::ActionType::SetSnappingValue, 0.5f);
  s_hSnappingValues[5] = EZ_REGISTER_ACTION_2("SnapPos_0_25",   "0.25", ezActionScope::Document, "Position Snap", "", ezTranslateGizmoAction, ezTranslateGizmoAction::ActionType::SetSnappingValue, 0.25f);
  s_hSnappingValues[6] = EZ_REGISTER_ACTION_2("SnapPos_0_2",     "0.2", ezActionScope::Document, "Position Snap", "", ezTranslateGizmoAction, ezTranslateGizmoAction::ActionType::SetSnappingValue, 0.2f);
  s_hSnappingValues[7] = EZ_REGISTER_ACTION_2("SnapPos_0_125", "0.125", ezActionScope::Document, "Position Snap", "", ezTranslateGizmoAction, ezTranslateGizmoAction::ActionType::SetSnappingValue, 0.125f);
  s_hSnappingValues[8] = EZ_REGISTER_ACTION_2("SnapPos_0_1",     "0.1", ezActionScope::Document, "Position Snap", "", ezTranslateGizmoAction, ezTranslateGizmoAction::ActionType::SetSnappingValue, 0.1f);
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

  ezStringBuilder sSubPath(szPath, "/PositionSnapCategory");

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