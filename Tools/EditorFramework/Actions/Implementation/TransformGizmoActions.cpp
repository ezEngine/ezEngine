#include <PCH.h>
#include <EditorFramework/Actions/TransformGizmoActions.h>
#include <GuiFoundation/Action/ActionManager.h>
#include <GuiFoundation/Action/ActionMapManager.h>
#include <EditorFramework/Gizmos/SnapProvider.h>
#include <EditorFramework/EditTools/StandardGizmoEditTools.h>

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezGizmoAction, 0, ezRTTINoAllocator);
EZ_END_DYNAMIC_REFLECTED_TYPE

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

void ezTransformGizmoActions::RegisterActions()
{
  s_hGizmoCategory = EZ_REGISTER_CATEGORY("GizmoCategory");
  s_hGizmoMenu = EZ_REGISTER_MENU("Gizmo.Menu");
  s_hNoGizmo = EZ_REGISTER_ACTION_1("Gizmo.Mode.Select", ezActionScope::Document, "Gizmo", "Q", ezGizmoAction, nullptr);
  s_hTranslateGizmo = EZ_REGISTER_ACTION_1("Gizmo.Mode.Translate", ezActionScope::Document, "Gizmo", "W", ezToggleWorldSpaceGizmo, ezGetStaticRTTI<ezTranslateGizmoEditTool>());
  s_hRotateGizmo = EZ_REGISTER_ACTION_1("Gizmo.Mode.Rotate", ezActionScope::Document, "Gizmo", "E", ezToggleWorldSpaceGizmo, ezGetStaticRTTI<ezRotateGizmoEditTool>());
  s_hScaleGizmo = EZ_REGISTER_ACTION_1("Gizmo.Mode.Scale", ezActionScope::Document, "Gizmo", "R", ezGizmoAction, ezGetStaticRTTI<ezScaleGizmoEditTool>());
  s_hDragToPositionGizmo = EZ_REGISTER_ACTION_1("Gizmo.Mode.DragToPosition", ezActionScope::Document, "Gizmo", "T", ezGizmoAction, ezGetStaticRTTI<ezDragToPositionGizmoEditTool>());
  s_hWorldSpace = EZ_REGISTER_ACTION_1("Gizmo.TransformSpace", ezActionScope::Document, "Gizmo", "", ezTransformGizmoAction, ezTransformGizmoAction::ActionType::GizmoToggleWorldSpace);
  s_hMoveParentOnly = EZ_REGISTER_ACTION_1("Gizmo.MoveParentOnly", ezActionScope::Document, "Gizmo", "P", ezTransformGizmoAction, ezTransformGizmoAction::ActionType::GizmoToggleMoveParentOnly);
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
}

//////////////////////////////////////////////////////////////////////////

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezTransformGizmoAction, 0, ezRTTINoAllocator);
EZ_END_DYNAMIC_REFLECTED_TYPE

ezTransformGizmoAction::ezTransformGizmoAction(const ezActionContext& context, const char* szName, ActionType type)
  : ezButtonAction(context, szName, false, "")
{
  SetCheckable(true);
  m_Type = type;
  m_pSceneDocument = static_cast<ezGameObjectDocument*>(context.m_pDocument);

  switch (m_Type)
  {
  case ActionType::GizmoToggleWorldSpace:
    SetIconPath(":/EditorFramework/Icons/WorldSpace16.png");
    break;
  case ActionType::GizmoToggleMoveParentOnly:
    SetIconPath(":/EditorFramework/Icons/TransformParent16.png");
    break;
  }

  UpdateState();
}

ezTransformGizmoAction::~ezTransformGizmoAction()
{
}

void ezTransformGizmoAction::Execute(const ezVariant& value)
{
  if (m_Type == ActionType::GizmoToggleWorldSpace)
  {
    m_pSceneDocument->SetGizmoWorldSpace(value.ConvertTo<bool>());
  }
  else if (m_Type == ActionType::GizmoToggleMoveParentOnly)
  {
    m_pSceneDocument->SetGizmoMoveParentOnly(value.ConvertTo<bool>());
  }

  UpdateState();
}

void ezTransformGizmoAction::UpdateState()
{
  if (m_Type == ActionType::GizmoToggleWorldSpace)
  {
    ezGameObjectEditTool* pTool = m_pSceneDocument->GetActiveEditTool();
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
        SetChecked(m_pSceneDocument->GetGizmoWorldSpace());
        break;
      }
    }
  }
  else if (m_Type == ActionType::GizmoToggleMoveParentOnly)
  {
    ezGameObjectEditTool* pTool = m_pSceneDocument->GetActiveEditTool();
    const bool bSupported = pTool != nullptr && pTool->GetSupportsMoveParentOnly();

    SetEnabled(bSupported);
    SetChecked(bSupported && m_pSceneDocument->GetGizmoMoveParentOnly());
  }
}


//////////////////////////////////////////////////////////////////////////

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezRotateGizmoAction, 1, ezRTTINoAllocator);
EZ_END_DYNAMIC_REFLECTED_TYPE

ezActionDescriptorHandle ezRotateGizmoAction::s_hSnappingValueMenu;
ezActionDescriptorHandle ezRotateGizmoAction::s_hSnappingValues[11];

void ezRotateGizmoAction::RegisterActions()
{
  s_hSnappingValueMenu = EZ_REGISTER_MENU_WITH_ICON("Gizmo.Rotation.Snap.Menu", ":/TypeIcons/ezRotateGizmoEditTool.png");
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
  EZ_ASSERT_DEV(pMap != nullptr, "The given mapping ('{0}') does not exist, mapping the actions failed!", szMapping);

  ezStringBuilder sSubPath(szPath, "/Gizmo.Rotation.Snap.Menu");

  pMap->MapAction(s_hSnappingValueMenu, szPath, 9.0f);

  for (ezUInt32 i = 0; i < EZ_ARRAY_SIZE(s_hSnappingValues); ++i)
    pMap->MapAction(s_hSnappingValues[i], sSubPath, i + 1.0f);
}

ezRotateGizmoAction::ezRotateGizmoAction(const ezActionContext& context, const char* szName, ActionType type, float fSnappingValue) : ezButtonAction(context, szName, false, "")
{
  SetCheckable(true);
  m_Type = type;
  m_fSnappingValue = fSnappingValue;

  SetChecked(m_fSnappingValue == ezSnapProvider::GetRotationSnapValue().GetDegree());

  ezSnapProvider::s_Events.AddEventHandler(ezMakeDelegate(&ezRotateGizmoAction::EventHandler, this));
}

ezRotateGizmoAction::~ezRotateGizmoAction()
{
  ezSnapProvider::s_Events.RemoveEventHandler(ezMakeDelegate(&ezRotateGizmoAction::EventHandler, this));
}

void ezRotateGizmoAction::EventHandler(const ezSnapProviderEvent& e)
{
  switch (e.m_Type)
  {
  case ezSnapProviderEvent::Type::RotationSnapChanged:
    {
      SetChecked(m_fSnappingValue == ezSnapProvider::GetRotationSnapValue().GetDegree());
    }
    break;
  }
}

void ezRotateGizmoAction::Execute(const ezVariant& value)
{
  if (m_Type == ActionType::SetSnappingAngle)
  {
    ezSnapProvider::SetRotationSnapValue(ezAngle::Degree(m_fSnappingValue));
  }
}

//////////////////////////////////////////////////////////////////////////

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezScaleGizmoAction, 1, ezRTTINoAllocator);
EZ_END_DYNAMIC_REFLECTED_TYPE

ezActionDescriptorHandle ezScaleGizmoAction::s_hSnappingValueMenu;
ezActionDescriptorHandle ezScaleGizmoAction::s_hSnappingValues[8];

void ezScaleGizmoAction::RegisterActions()
{
  s_hSnappingValueMenu = EZ_REGISTER_MENU_WITH_ICON("Gizmo.Scale.Snap.Menu", ":/TypeIcons/ezScaleGizmoEditTool.png");
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
  EZ_ASSERT_DEV(pMap != nullptr, "The given mapping ('{0}') does not exist, mapping the actions failed!", szMapping);

  ezStringBuilder sSubPath(szPath, "/Gizmo.Scale.Snap.Menu");

  pMap->MapAction(s_hSnappingValueMenu, szPath, 10.0f);

  for (ezUInt32 i = 0; i < EZ_ARRAY_SIZE(s_hSnappingValues); ++i)
    pMap->MapAction(s_hSnappingValues[i], sSubPath, i + 1.0f);
}

ezScaleGizmoAction::ezScaleGizmoAction(const ezActionContext& context, const char* szName, ActionType type, float fSnappingValue) : ezButtonAction(context, szName, false, "")
{
  SetCheckable(true);
  m_Type = type;
  m_fSnappingValue = fSnappingValue;

  SetChecked(m_fSnappingValue == ezSnapProvider::GetScaleSnapValue());

  ezSnapProvider::s_Events.AddEventHandler(ezMakeDelegate(&ezScaleGizmoAction::EventHandler, this));
}

ezScaleGizmoAction::~ezScaleGizmoAction()
{
  ezSnapProvider::s_Events.RemoveEventHandler(ezMakeDelegate(&ezScaleGizmoAction::EventHandler, this));
}

void ezScaleGizmoAction::EventHandler(const ezSnapProviderEvent& e)
{
  switch (e.m_Type)
  {
  case ezSnapProviderEvent::Type::ScaleSnapChanged:
    {
      SetChecked(m_fSnappingValue == ezSnapProvider::GetScaleSnapValue());
    }
    break;
  }
}

void ezScaleGizmoAction::Execute(const ezVariant& value)
{
  if (m_Type == ActionType::SetSnappingValue)
  {
    ezSnapProvider::SetScaleSnapValue(m_fSnappingValue);
  }
}

//////////////////////////////////////////////////////////////////////////

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezTranslateGizmoAction, 1, ezRTTINoAllocator);
EZ_END_DYNAMIC_REFLECTED_TYPE

ezActionDescriptorHandle ezTranslateGizmoAction::s_hSnappingValueMenu;
ezActionDescriptorHandle ezTranslateGizmoAction::s_hSnapPivotToGrid;
ezActionDescriptorHandle ezTranslateGizmoAction::s_hSnapObjectsToGrid;
ezActionDescriptorHandle ezTranslateGizmoAction::s_hSnappingValues[9];

void ezTranslateGizmoAction::RegisterActions()
{
  s_hSnappingValueMenu = EZ_REGISTER_MENU_WITH_ICON("Gizmo.Translate.Snap.Menu", ":/TypeIcons/ezTranslateGizmoEditTool.png");
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
  EZ_ASSERT_DEV(pMap != nullptr, "The given mapping ('{0}') does not exist, mapping the actions failed!", szMapping);

  ezStringBuilder sSubPath(szPath, "/Gizmo.Translate.Snap.Menu");

  pMap->MapAction(s_hSnappingValueMenu, szPath, 8.0f);

  pMap->MapAction(s_hSnapPivotToGrid, sSubPath, 0.0f);
  pMap->MapAction(s_hSnapObjectsToGrid, sSubPath, 1.0f);

  for (ezUInt32 i = 0; i < EZ_ARRAY_SIZE(s_hSnappingValues); ++i)
    pMap->MapAction(s_hSnappingValues[i], sSubPath, i + 2.0f);
}

ezTranslateGizmoAction::ezTranslateGizmoAction(const ezActionContext& context, const char* szName, ActionType type, float fSnappingValue) : ezButtonAction(context, szName, false, "")
{
  m_pSceneDocument = static_cast<const ezGameObjectDocument*>(context.m_pDocument);
  m_Type = type;
  m_fSnappingValue = fSnappingValue;

  if (m_Type == ActionType::SetSnappingValue)
  {
    SetCheckable(true);
    SetChecked(m_fSnappingValue == ezSnapProvider::GetTranslationSnapValue());
  }

  ezSnapProvider::s_Events.AddEventHandler(ezMakeDelegate(&ezTranslateGizmoAction::EventHandler, this));
}

ezTranslateGizmoAction::~ezTranslateGizmoAction()
{
  ezSnapProvider::s_Events.RemoveEventHandler(ezMakeDelegate(&ezTranslateGizmoAction::EventHandler, this));
}

void ezTranslateGizmoAction::EventHandler(const ezSnapProviderEvent& e)
{
  switch (e.m_Type)
  {
  case ezSnapProviderEvent::Type::TranslationSnapChanged:
    {
      SetChecked(m_fSnappingValue == ezSnapProvider::GetTranslationSnapValue());
    }
    break;
  }
}

void ezTranslateGizmoAction::Execute(const ezVariant& value)
{
  if (m_Type == ActionType::SetSnappingValue)
    ezSnapProvider::SetTranslationSnapValue(m_fSnappingValue);

  if (m_Type == ActionType::SnapSelectionPivotToGrid)
    m_pSceneDocument->TriggerSnapPivotToGrid();

  if (m_Type == ActionType::SnapEachSelectedObjectToGrid)
    m_pSceneDocument->TriggerSnapEachObjectToGrid();
}
