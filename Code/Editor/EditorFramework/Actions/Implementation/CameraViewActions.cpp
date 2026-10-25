#include <EditorFramework/EditorFrameworkPCH.h>

#include <EditorFramework/Actions/CameraViewActions.h>
#include <EditorFramework/DocumentWindow/EngineDocumentWindow.moc.h>
#include <EditorFramework/DocumentWindow/GameObjectViewWidget.moc.h>
#include <GuiFoundation/Action/ActionManager.h>
#include <GuiFoundation/Action/ActionMapManager.h>

// clang-format off
EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezCameraViewAction, 1, ezRTTINoAllocator)
EZ_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

ezActionDescriptorHandle ezCameraViewActions::s_hCameraViewMenu;
ezActionDescriptorHandle ezCameraViewActions::s_hViewFront;
ezActionDescriptorHandle ezCameraViewActions::s_hViewBack;
ezActionDescriptorHandle ezCameraViewActions::s_hViewRight;
ezActionDescriptorHandle ezCameraViewActions::s_hViewLeft;
ezActionDescriptorHandle ezCameraViewActions::s_hViewTop;
ezActionDescriptorHandle ezCameraViewActions::s_hViewBottom;
ezActionDescriptorHandle ezCameraViewActions::s_hFlipView;
ezActionDescriptorHandle ezCameraViewActions::s_hOrbitLeft;
ezActionDescriptorHandle ezCameraViewActions::s_hOrbitRight;
ezActionDescriptorHandle ezCameraViewActions::s_hOrbitUp;
ezActionDescriptorHandle ezCameraViewActions::s_hOrbitDown;

void ezCameraViewActions::RegisterActions()
{
  s_hCameraViewMenu = EZ_REGISTER_MENU("Scene.Camera.View.Menu");
  s_hViewFront = EZ_REGISTER_ACTION_1("Scene.Camera.View.Front", ezActionScope::Document, "Camera", "Num+1", ezCameraViewAction, ezCameraViewAction::ActionType::ViewFront);
  s_hViewBack = EZ_REGISTER_ACTION_1("Scene.Camera.View.Back", ezActionScope::Document, "Camera", "Ctrl+Num+1", ezCameraViewAction, ezCameraViewAction::ActionType::ViewBack);
  s_hViewRight = EZ_REGISTER_ACTION_1("Scene.Camera.View.Right", ezActionScope::Document, "Camera", "Num+3", ezCameraViewAction, ezCameraViewAction::ActionType::ViewRight);
  s_hViewLeft = EZ_REGISTER_ACTION_1("Scene.Camera.View.Left", ezActionScope::Document, "Camera", "Ctrl+Num+3", ezCameraViewAction, ezCameraViewAction::ActionType::ViewLeft);
  s_hViewTop = EZ_REGISTER_ACTION_1("Scene.Camera.View.Top", ezActionScope::Document, "Camera", "Num+7", ezCameraViewAction, ezCameraViewAction::ActionType::ViewTop);
  s_hViewBottom = EZ_REGISTER_ACTION_1("Scene.Camera.View.Bottom", ezActionScope::Document, "Camera", "Ctrl+Num+7", ezCameraViewAction, ezCameraViewAction::ActionType::ViewBottom);
  s_hFlipView = EZ_REGISTER_ACTION_1("Scene.Camera.View.Flip", ezActionScope::Document, "Camera", "Num+9", ezCameraViewAction, ezCameraViewAction::ActionType::FlipView);
  s_hOrbitLeft = EZ_REGISTER_ACTION_1("Scene.Camera.View.OrbitLeft", ezActionScope::Document, "Camera", "Num+4", ezCameraViewAction, ezCameraViewAction::ActionType::OrbitLeft);
  s_hOrbitRight = EZ_REGISTER_ACTION_1("Scene.Camera.View.OrbitRight", ezActionScope::Document, "Camera", "Num+6", ezCameraViewAction, ezCameraViewAction::ActionType::OrbitRight);
  s_hOrbitUp = EZ_REGISTER_ACTION_1("Scene.Camera.View.OrbitUp", ezActionScope::Document, "Camera", "Num+8", ezCameraViewAction, ezCameraViewAction::ActionType::OrbitUp);
  s_hOrbitDown = EZ_REGISTER_ACTION_1("Scene.Camera.View.OrbitDown", ezActionScope::Document, "Camera", "Num+2", ezCameraViewAction, ezCameraViewAction::ActionType::OrbitDown);
}

void ezCameraViewActions::UnregisterActions()
{
  ezActionManager::UnregisterAction(s_hCameraViewMenu);
  ezActionManager::UnregisterAction(s_hViewFront);
  ezActionManager::UnregisterAction(s_hViewBack);
  ezActionManager::UnregisterAction(s_hViewRight);
  ezActionManager::UnregisterAction(s_hViewLeft);
  ezActionManager::UnregisterAction(s_hViewTop);
  ezActionManager::UnregisterAction(s_hViewBottom);
  ezActionManager::UnregisterAction(s_hFlipView);
  ezActionManager::UnregisterAction(s_hOrbitLeft);
  ezActionManager::UnregisterAction(s_hOrbitRight);
  ezActionManager::UnregisterAction(s_hOrbitUp);
  ezActionManager::UnregisterAction(s_hOrbitDown);
}

void ezCameraViewActions::MapMenuActions(ezStringView sMapping)
{
  ezActionMap* pMap = ezActionMapManager::GetActionMap(sMapping);
  EZ_ASSERT_DEV(pMap != nullptr, "The given mapping ('{0}') does not exist, mapping the actions failed!", sMapping);

  pMap->MapAction(s_hCameraViewMenu, "G.View", 1.0f);

  const ezStringView sSubPath = "Scene.Camera.View.Menu";
  pMap->MapAction(s_hViewFront, "G.View", sSubPath, 1.0f);
  pMap->MapAction(s_hViewBack, "G.View", sSubPath, 2.0f);
  pMap->MapAction(s_hViewRight, "G.View", sSubPath, 3.0f);
  pMap->MapAction(s_hViewLeft, "G.View", sSubPath, 4.0f);
  pMap->MapAction(s_hViewTop, "G.View", sSubPath, 5.0f);
  pMap->MapAction(s_hViewBottom, "G.View", sSubPath, 6.0f);
  pMap->MapAction(s_hFlipView, "G.View", sSubPath, 7.0f);
  pMap->MapAction(s_hOrbitLeft, "G.View", sSubPath, 8.0f);
  pMap->MapAction(s_hOrbitRight, "G.View", sSubPath, 9.0f);
  pMap->MapAction(s_hOrbitUp, "G.View", sSubPath, 10.0f);
  pMap->MapAction(s_hOrbitDown, "G.View", sSubPath, 11.0f);
}

ezCameraViewAction::ezCameraViewAction(const ezActionContext& context, const char* szName, ActionType type)
  : ezButtonAction(context, szName, false, "")
{
  m_Type = type;
}

void ezCameraViewAction::Execute(const ezVariant& value)
{
  auto* pWindow = qobject_cast<ezQtEngineDocumentWindow*>(ezQtDocumentWindow::FindWindowByDocument(m_Context.m_pDocument));
  if (pWindow == nullptr)
    return;

  ezQtEngineViewWidget* pEngineView = pWindow->GetHoveredViewWidget();
  if (pEngineView == nullptr)
    pEngineView = pWindow->GetFocusedViewWidget();

  auto* pView = qobject_cast<ezQtGameObjectViewWidget*>(pEngineView);
  if (pView == nullptr)
    return;

  ezEngineViewConfig* pViewConfig = pView->m_pViewConfig;

  if (pViewConfig->m_Perspective != ezSceneViewPerspective::Perspective)
  {
    // orthographic views have a fixed direction, so switch between them instead
    // there are no orthographic modes for the opposite directions, those actions do nothing here
    ezSceneViewPerspective::Enum perspective;
    switch (m_Type)
    {
      case ActionType::ViewFront:
        perspective = ezSceneViewPerspective::Orthogonal_Front;
        break;
      case ActionType::ViewRight:
        perspective = ezSceneViewPerspective::Orthogonal_Right;
        break;
      case ActionType::ViewTop:
        perspective = ezSceneViewPerspective::Orthogonal_Top;
        break;
      default:
        return;
    }

    if (pViewConfig->m_Perspective != perspective)
    {
      pViewConfig->m_Perspective = perspective;
      pViewConfig->ApplyPerspectiveSetting(pViewConfig->m_Camera.GetFovOrDim());
    }

    return;
  }

  // the view directions match the orthographic modes, e.g. 'front' looks at the side of objects that faces +X
  const ezAngle orbitStep = ezAngle::MakeFromDegree(15.0f);

  switch (m_Type)
  {
    case ActionType::ViewFront:
      pView->OrbitCameraTo(ezVec3(-1, 0, 0), ezVec3(0, 0, 1));
      break;
    case ActionType::ViewBack:
      pView->OrbitCameraTo(ezVec3(1, 0, 0), ezVec3(0, 0, 1));
      break;
    case ActionType::ViewRight:
      pView->OrbitCameraTo(ezVec3(0, -1, 0), ezVec3(0, 0, 1));
      break;
    case ActionType::ViewLeft:
      pView->OrbitCameraTo(ezVec3(0, 1, 0), ezVec3(0, 0, 1));
      break;
    case ActionType::ViewTop:
      pView->OrbitCameraTo(ezVec3(0, 0, -1), ezVec3(1, 0, 0));
      break;
    case ActionType::ViewBottom:
      pView->OrbitCameraTo(ezVec3(0, 0, 1), ezVec3(-1, 0, 0));
      break;

    case ActionType::FlipView:
    {
      ezVec3 vOrbitPoint, vDirection, vUp;
      if (!pView->GetCameraOrbitTarget(vOrbitPoint, vDirection, vUp))
        vDirection = pViewConfig->m_Camera.GetCenterDirForwards();

      // turning around the world up axis keeps the horizon level, but does nothing when looking straight up or down
      if (ezMath::Abs(vDirection.z) > 0.99f)
        pView->OrbitCameraBy(ezAngle(), ezAngle::MakeFromDegree(180.0f));
      else
        pView->OrbitCameraBy(ezAngle::MakeFromDegree(180.0f), ezAngle());
      break;
    }

    case ActionType::OrbitLeft:
      pView->OrbitCameraBy(orbitStep, ezAngle());
      break;
    case ActionType::OrbitRight:
      pView->OrbitCameraBy(-orbitStep, ezAngle());
      break;
    case ActionType::OrbitUp:
      pView->OrbitCameraBy(ezAngle(), orbitStep);
      break;
    case ActionType::OrbitDown:
      pView->OrbitCameraBy(ezAngle(), -orbitStep);
      break;
  }
}
