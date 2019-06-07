#include <EditorFrameworkPCH.h>

#include <EditorFramework/Document/GameObjectDocument.h>
#include <EditorFramework/DocumentWindow/GameObjectDocumentWindow.moc.h>
#include <EditorFramework/DocumentWindow/GameObjectViewWidget.moc.h>
#include <EditorFramework/EditTools/EditTool.h>
#include <EditorFramework/InputContexts/CameraMoveContext.h>
#include <Gizmos/SnapProvider.h>
#include <Gizmos/TranslateGizmo.h>
#include <GuiFoundation/PropertyGrid/ManipulatorManager.h>
#include <Preferences/EditorPreferences.h>
#include <Preferences/ScenePreferences.h>
#include <Manipulators/ManipulatorAdapterRegistry.h>

ezQtGameObjectDocumentWindow::ezQtGameObjectDocumentWindow(ezGameObjectDocument* pDocument)
    : ezQtEngineDocumentWindow(pDocument)
{
  pDocument->m_GameObjectEvents.AddEventHandler(ezMakeDelegate(&ezQtGameObjectDocumentWindow::GameObjectEventHandler, this));
  ezSnapProvider::s_Events.AddEventHandler(ezMakeDelegate(&ezQtGameObjectDocumentWindow::SnapProviderEventHandler, this));
}

ezQtGameObjectDocumentWindow::~ezQtGameObjectDocumentWindow()
{
  GetGameObjectDocument()->m_GameObjectEvents.RemoveEventHandler(
      ezMakeDelegate(&ezQtGameObjectDocumentWindow::GameObjectEventHandler, this));
  ezSnapProvider::s_Events.RemoveEventHandler(ezMakeDelegate(&ezQtGameObjectDocumentWindow::SnapProviderEventHandler, this));
}

ezGameObjectDocument* ezQtGameObjectDocumentWindow::GetGameObjectDocument() const
{
  return static_cast<ezGameObjectDocument*>(GetDocument());
}

ezGlobalSettingsMsgToEngine ezQtGameObjectDocumentWindow::GetGlobalSettings() const
{
  ezGlobalSettingsMsgToEngine msg;
  msg.m_fGizmoScale = ezPreferences::QueryPreferences<ezEditorPreferencesUser>()->m_fGizmoScale;
  return msg;
}

ezWorldSettingsMsgToEngine ezQtGameObjectDocumentWindow::GetWorldSettings() const
{
  ezWorldSettingsMsgToEngine msg;
  auto pGameObjectDoc = GetGameObjectDocument();
  msg.m_bRenderOverlay = pGameObjectDoc->GetRenderSelectionOverlay();
  msg.m_bRenderShapeIcons = pGameObjectDoc->GetRenderShapeIcons();
  msg.m_bRenderSelectionBoxes = pGameObjectDoc->GetRenderVisualizers();
  msg.m_bAddAmbientLight = pGameObjectDoc->GetAddAmbientLight();
  return msg;
}

ezGridSettingsMsgToEngine ezQtGameObjectDocumentWindow::GetGridSettings() const
{
  ezGridSettingsMsgToEngine msg;

  if (auto pTool = GetGameObjectDocument()->GetActiveEditTool())
  {
    pTool->GetGridSettings(msg);
  }
  else
  {
    ezManipulatorAdapterRegistry::GetSingleton()->QueryGridSettings(GetDocument(), msg);
  }

  return msg;
}

void ezQtGameObjectDocumentWindow::ProcessMessageEventHandler(const ezEditorEngineDocumentMsg* pMsg)
{
  ezQtEngineDocumentWindow::ProcessMessageEventHandler(pMsg);
  if (pMsg->GetDynamicRTTI()->IsDerivedFrom<ezQuerySelectionBBoxResultMsgToEditor>())
  {
    const ezQuerySelectionBBoxResultMsgToEditor* msg = static_cast<const ezQuerySelectionBBoxResultMsgToEditor*>(pMsg);

    if (msg->m_uiViewID == 0xFFFFFFFF)
    {
      for (auto pView : m_ViewWidgets)
      {
        if (!pView)
          continue;

        if (msg->m_iPurpose == 0)
          HandleFocusOnSelection(msg, static_cast<ezQtGameObjectViewWidget*>(pView));
      }
    }
    else
    {
      ezQtGameObjectViewWidget* pSceneView = static_cast<ezQtGameObjectViewWidget*>(GetViewWidgetByID(msg->m_uiViewID));

      if (!pSceneView)
        return;

      if (msg->m_iPurpose == 0)
        HandleFocusOnSelection(msg, pSceneView);
    }

    return;
  }
}

void ezQtGameObjectDocumentWindow::GameObjectEventHandler(const ezGameObjectEvent& e)
{
  switch (e.m_Type)
  {
    case ezGameObjectEvent::Type::TriggerFocusOnSelection_Hovered:
      FocusOnSelectionHoveredView();
      break;

    case ezGameObjectEvent::Type::TriggerFocusOnSelection_All:
      FocusOnSelectionAllViews();
      break;
  }
}

void ezQtGameObjectDocumentWindow::FocusOnSelectionAllViews()
{
  const auto& sel = GetDocument()->GetSelectionManager()->GetSelection();

  if (sel.IsEmpty())
    return;
  if (!sel.PeekBack()->GetTypeAccessor().GetType()->IsDerivedFrom<ezGameObject>())
    return;

  ezQuerySelectionBBoxMsgToEngine msg;
  msg.m_uiViewID = 0xFFFFFFFF;
  msg.m_iPurpose = 0;
  GetDocument()->SendMessageToEngine(&msg);
}

void ezQtGameObjectDocumentWindow::FocusOnSelectionHoveredView()
{
  const auto& sel = GetDocument()->GetSelectionManager()->GetSelection();

  if (sel.IsEmpty())
    return;
  if (!sel.PeekBack()->GetTypeAccessor().GetType()->IsDerivedFrom<ezGameObject>())
    return;

  auto pView = GetHoveredViewWidget();

  if (pView == nullptr)
    return;

  ezQuerySelectionBBoxMsgToEngine msg;
  msg.m_uiViewID = pView->GetViewID();
  msg.m_iPurpose = 0;
  GetDocument()->SendMessageToEngine(&msg);
}
void ezQtGameObjectDocumentWindow::HandleFocusOnSelection(const ezQuerySelectionBBoxResultMsgToEditor* pMsg,
                                                          ezQtGameObjectViewWidget* pSceneView)
{
  const ezVec3 vPivotPoint = pMsg->m_vCenter;

  const ezCamera& cam = pSceneView->m_pViewConfig->m_Camera;

  ezVec3 vNewCameraPosition = cam.GetCenterPosition();
  ezVec3 vNewCameraDirection = cam.GetDirForwards();
  float fNewFovOrDim = cam.GetFovOrDim();

  if (pSceneView->width() == 0 || pSceneView->height() == 0)
    return;

  const float fApsectRation = (float)pSceneView->width() / (float)pSceneView->height();

  ezBoundingBox bbox;

  // clamp the bbox of the selection to ranges that won't break down due to float precision
  {
    bbox.SetCenterAndHalfExtents(pMsg->m_vCenter, pMsg->m_vHalfExtents);
    bbox.m_vMin = bbox.m_vMin.CompMax(ezVec3(-1000.0f));
    bbox.m_vMax = bbox.m_vMax.CompMin(ezVec3(+1000.0f));
  }

  const ezVec3 vCurrentOrbitPoint = pSceneView->m_pCameraMoveContext->GetOrbitPoint();
  const bool bZoomIn = vPivotPoint.IsEqual(vCurrentOrbitPoint, 0.1f);

  if (cam.GetCameraMode() == ezCameraMode::PerspectiveFixedFovX || cam.GetCameraMode() == ezCameraMode::PerspectiveFixedFovY)
  {
    const float maxExt = pMsg->m_vHalfExtents.GetLength();
    const float fMinDistance = cam.GetNearPlane() * 1.1f + maxExt;

    {
      ezPlane p;
      p.SetFromNormalAndPoint(vNewCameraDirection, vNewCameraPosition);

      // at some distance the floating point precision gets so crappy that the camera movement breaks
      // therefore we clamp it to a 'reasonable' distance here
      const float distBest = ezMath::Min(ezMath::Abs(p.GetDistanceTo(vPivotPoint)), 500.0f);

      vNewCameraPosition = vPivotPoint - vNewCameraDirection * ezMath::Max(fMinDistance, distBest);
    }

    // only zoom in on the object, if the target position is already identical (action executed twice)
    if (!pMsg->m_vHalfExtents.IsZero(ezMath::BasicType<float>::DefaultEpsilon()) && bZoomIn)
    {
      const ezAngle fovX = cam.GetFovX(fApsectRation);
      const ezAngle fovY = cam.GetFovY(fApsectRation);

      const float fRadius = bbox.GetBoundingSphere().m_fRadius * 1.5f;

      const float dist1 = fRadius / ezMath::Sin(fovX * 0.75);
      const float dist2 = fRadius / ezMath::Sin(fovY * 0.75);
      const float distBest = ezMath::Max(dist1, dist2);

      vNewCameraPosition = vPivotPoint - vNewCameraDirection * ezMath::Max(fMinDistance, distBest);
    }
  }
  else
  {
    vNewCameraPosition = pMsg->m_vCenter;

    // only zoom in on the object, if the target position is already identical (action executed twice)
    if (bZoomIn)
    {

      const ezVec3 right = cam.GetDirRight();
      const ezVec3 up = cam.GetDirUp();

      const float fSizeFactor = 2.0f;

      const float fRequiredWidth = ezMath::Abs(right.Dot(bbox.GetHalfExtents()) * 2.0f) * fSizeFactor;
      const float fRequiredHeight = ezMath::Abs(up.Dot(bbox.GetHalfExtents()) * 2.0f) * fSizeFactor;

      float fDimWidth, fDimHeight;

      if (cam.GetCameraMode() == ezCameraMode::OrthoFixedHeight)
      {
        fDimHeight = cam.GetFovOrDim();
        fDimWidth = fDimHeight * fApsectRation;
      }
      else
      {
        fDimWidth = cam.GetFovOrDim();
        fDimHeight = fDimWidth / fApsectRation;
      }

      const float fScaleWidth = fRequiredWidth / fDimWidth;
      const float fScaleHeight = fRequiredHeight / fDimHeight;

      const float fScaleDim = ezMath::Max(fScaleWidth, fScaleHeight);

      if (fScaleDim > 0.0f)
      {
        fNewFovOrDim *= fScaleDim;
      }
    }
  }

  pSceneView->m_pCameraMoveContext->SetOrbitPoint(vPivotPoint);
  pSceneView->InterpolateCameraTo(vNewCameraPosition, vNewCameraDirection, fNewFovOrDim);
}

void ezQtGameObjectDocumentWindow::SnapProviderEventHandler(const ezSnapProviderEvent& e)
{
  switch (e.m_Type)
  {
    case ezSnapProviderEvent::Type::RotationSnapChanged:
      ShowTemporaryStatusBarMsg(ezFmt(ezStringUtf8(L"Snapping Angle: {0}°").GetData(), ezSnapProvider::GetRotationSnapValue().GetDegree()));
      break;

    case ezSnapProviderEvent::Type::ScaleSnapChanged:
      ShowTemporaryStatusBarMsg(ezFmt("Snapping Value: {0}", ezSnapProvider::GetScaleSnapValue()));
      break;

    case ezSnapProviderEvent::Type::TranslationSnapChanged:
      ShowTemporaryStatusBarMsg(ezFmt("Snapping Value: {0}", ezSnapProvider::GetTranslationSnapValue()));
      break;
  }
}
