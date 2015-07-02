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