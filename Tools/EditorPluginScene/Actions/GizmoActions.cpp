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

void ezGizmoActions::RegisterActions()
{
  s_hGizmoCategory = EZ_REGISTER_CATEGORY("GizmoCategory");
  s_hNoGizmo = EZ_REGISTER_ACTION_1("Select", "Select", ezActionScope::Document, "Document", "Q", ezGizmoAction, ActiveGizmo::None);
  s_hTranslateGizmo = EZ_REGISTER_ACTION_1("Translate", "Translate", ezActionScope::Document, "Document", "W", ezGizmoAction, ActiveGizmo::Translate);
  s_hRotateGizmo = EZ_REGISTER_ACTION_1("Rotate", "Rotate", ezActionScope::Document, "Document", "E", ezGizmoAction, ActiveGizmo::Rotate);
  s_hScaleGizmo = EZ_REGISTER_ACTION_1("Scale", "Scale", ezActionScope::Document, "Document", "R", ezGizmoAction, ActiveGizmo::Scale);

}

void ezGizmoActions::UnregisterActions()
{
  ezActionManager::UnregisterAction(s_hGizmoCategory);
  ezActionManager::UnregisterAction(s_hNoGizmo);
  ezActionManager::UnregisterAction(s_hTranslateGizmo);
  ezActionManager::UnregisterAction(s_hRotateGizmo);
  ezActionManager::UnregisterAction(s_hScaleGizmo);
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
}

ezGizmoAction::ezGizmoAction(const ezActionContext& context, const char* szName, ActiveGizmo button) : ezButtonAction(context, szName, false, "")
{
  SetCheckable(true);
  m_ButtonType = button;
  m_pSceneDocument = static_cast<ezSceneDocument*>(context.m_pDocument);
  m_pSceneDocument->m_SceneEvents.AddEventHandler(ezMakeDelegate(&ezGizmoAction::SceneEventHandler, this));

  switch (m_ButtonType)
  {
  case ActiveGizmo::None:
    SetIconPath(":/GuiFoundation/Icons/GizmoNone24.png");
    break;
  case ActiveGizmo::Translate:
    SetIconPath(":/GuiFoundation/Icons/GizmoTranslate24.png");
    break;
  case ActiveGizmo::Rotate:
    SetIconPath(":/GuiFoundation/Icons/GizmoRotate24.png");
    break;
  case ActiveGizmo::Scale:
    SetIconPath(":/GuiFoundation/Icons/GizmoScale24.png");
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
  m_pSceneDocument->SetActiveGizmo(m_ButtonType);
}

void ezGizmoAction::UpdateState()
{
  SetChecked(m_pSceneDocument->GetActiveGizmo() == m_ButtonType);
}

void ezGizmoAction::SceneEventHandler(const ezSceneDocument::SceneEvent& e)
{
  if (e.m_Type == ezSceneDocument::SceneEvent::Type::ActiveGizmoChanged)
    UpdateState();

}