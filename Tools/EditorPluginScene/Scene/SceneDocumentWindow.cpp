#include <PCH.h>
#include <EditorPluginScene/Scene/SceneDocumentWindow.moc.h>
#include <EditorPluginScene/Scene/SceneViewWidget.moc.h>
#include <EditorPluginScene/Scene/SceneDocument.h>
#include <GuiFoundation/ActionViews/MenuBarActionMapView.moc.h>
#include <GuiFoundation/ActionViews/ToolBarActionMapView.moc.h>
#include <EditorPluginScene/Panels/ScenegraphPanel/ScenegraphPanel.moc.h>
#include <GuiFoundation/PropertyGrid/PropertyGridWidget.moc.h>
#include <QGridLayout>
#include <QSettings>
#include <EditorFramework/InputContexts/OrthoGizmoContext.h>
#include <Core/Assets/AssetFileHeader.h>
#include <GuiFoundation/PropertyGrid/ManipulatorManager.h>
#include <EditorFramework/Preferences/EditorPreferences.h>
#include <EditorFramework/Preferences/ScenePreferences.h>
#include <EditorFramework/Gizmos/SnapProvider.h>
#include <ToolsFoundation/Command/TreeCommands.h>
#include <EditorFramework/Preferences/Preferences.h>
#include <EditorFramework/DocumentWindow/QuadViewWidget.moc.h>


ezQtSceneDocumentWindow::ezQtSceneDocumentWindow(ezSceneDocument* pDocument)
  : ezQtGameObjectDocumentWindow(pDocument)
{
  auto ViewFactory = [](ezQtEngineDocumentWindow* pWindow, ezEngineViewConfig* pConfig) -> ezQtEngineViewWidget*
  {
    ezQtSceneViewWidget* pWidget = new ezQtSceneViewWidget(nullptr, static_cast<ezQtSceneDocumentWindow*>(pWindow), pConfig);
    pWindow->AddViewWidget(pWidget);
    return pWidget;
  };
  m_pQuadViewWidget = new ezQtQuadViewWidget(pDocument, this, ViewFactory, "EditorPluginScene_ViewToolBar");
  m_GizmoHandler = EZ_DEFAULT_NEW(ezGameObjectGizmoHandler, pDocument, this, this);

  setCentralWidget(m_pQuadViewWidget);

  SetTargetFramerate(25);

  {
    // Menu Bar
    ezQtMenuBarActionMapView* pMenuBar = static_cast<ezQtMenuBarActionMapView*>(menuBar());
    ezActionContext context;
    context.m_sMapping = "EditorPluginScene_DocumentMenuBar";
    context.m_pDocument = pDocument;
    context.m_pWindow = this;
    pMenuBar->SetActionContext(context);
  }

  {
    // Tool Bar
    ezQtToolBarActionMapView* pToolBar = new ezQtToolBarActionMapView("Toolbar", this);
    ezActionContext context;
    context.m_sMapping = "EditorPluginScene_DocumentToolBar";
    context.m_pDocument = pDocument;
    context.m_pWindow = this;
    pToolBar->SetActionContext(context);
    pToolBar->setObjectName("SceneDocumentWindow_ToolBar");
    addToolBar(pToolBar);
  }

  const ezSceneDocument* pSceneDoc = static_cast<const ezSceneDocument*>(GetDocument());
  pSceneDoc->m_GameObjectEvents.AddEventHandler(ezMakeDelegate(&ezQtSceneDocumentWindow::GameObjectEventHandler, this));
  ezSnapProvider::s_Events.AddEventHandler(ezMakeDelegate(&ezQtSceneDocumentWindow::SnapProviderEventHandler, this));
  ezPreferences::QueryPreferences<ezScenePreferencesUser>(GetDocument())->m_ChangedEvent.AddEventHandler(ezMakeDelegate(&ezQtSceneDocumentWindow::OnPreferenceChange, this));

  {
    ezQtDocumentPanel* pPropertyPanel = new ezQtDocumentPanel(this);
    pPropertyPanel->setObjectName("PropertyPanel");
    pPropertyPanel->setWindowTitle("Properties");
    pPropertyPanel->show();

    ezQtDocumentPanel* pPanelTree = new ezQtScenegraphPanel(this, static_cast<ezSceneDocument*>(pDocument));
    pPanelTree->show();

    ezQtPropertyGridWidget* pPropertyGrid = new ezQtPropertyGridWidget(pPropertyPanel, pDocument);
    pPropertyPanel->setWidget(pPropertyGrid);

    addDockWidget(Qt::DockWidgetArea::RightDockWidgetArea, pPropertyPanel);
    addDockWidget(Qt::DockWidgetArea::LeftDockWidgetArea, pPanelTree);
  }

  FinishWindowCreation();
}

ezQtSceneDocumentWindow::~ezQtSceneDocumentWindow()
{

  const ezSceneDocument* pSceneDoc = static_cast<const ezSceneDocument*>(GetDocument());
  pSceneDoc->m_GameObjectEvents.RemoveEventHandler(ezMakeDelegate(&ezQtSceneDocumentWindow::GameObjectEventHandler, this));

  ezPreferences::QueryPreferences<ezScenePreferencesUser>(GetDocument())->m_ChangedEvent.RemoveEventHandler(ezMakeDelegate(&ezQtSceneDocumentWindow::OnPreferenceChange, this));

  ezSnapProvider::s_Events.RemoveEventHandler(ezMakeDelegate(&ezQtSceneDocumentWindow::SnapProviderEventHandler, this));

}

ezSceneDocument* ezQtSceneDocumentWindow::GetSceneDocument() const
{
  return static_cast<ezSceneDocument*>(GetDocument());
}

void ezQtSceneDocumentWindow::ToggleViews(QWidget* pView)
{
  m_pQuadViewWidget->ToggleViews(pView);
}


ezObjectAccessorBase* ezQtSceneDocumentWindow::GetObjectAccessor()
{
  return GetDocument()->GetObjectAccessor();
}

bool ezQtSceneDocumentWindow::CanDuplicateSelection() const
{
  return true;
}

void ezQtSceneDocumentWindow::DuplicateSelection()
{
  GetSceneDocument()->DuplicateSelection();
}

void ezQtSceneDocumentWindow::SnapSelectionToPosition(bool bSnapEachObject)
{
  const float fSnap = ezSnapProvider::GetTranslationSnapValue();

  if (fSnap == 0.0f)
    return;

  const ezDeque<const ezDocumentObject*>& selection = GetSceneDocument()->GetSelectionManager()->GetSelection();
  if (selection.IsEmpty())
    return;

  const auto& pivotObj = selection.PeekBack();

  ezVec3 vPivotSnapOffset;

  if (!bSnapEachObject)
  {
    // if we snap by the pivot object only, the last selected object must be a valid game object
    if (!pivotObj->GetTypeAccessor().GetType()->IsDerivedFrom<ezGameObject>())
      return;

    const ezVec3 vPivotPos = GetSceneDocument()->GetGlobalTransform(pivotObj).m_vPosition;
    ezVec3 vSnappedPos = vPivotPos;
    ezSnapProvider::SnapTranslation(vSnappedPos);

    vPivotSnapOffset = vSnappedPos - vPivotPos;

    if (vPivotSnapOffset.IsZero())
      return;
  }

  ezDeque<ezGameObjectGizmoHandler::SelectedGO> gizmoSelection = m_GizmoHandler.Borrow()->GetSelectedGizmoObjects();
  if (gizmoSelection.IsEmpty())
    return;

  auto CmdHistory = GetDocument()->GetCommandHistory();

  CmdHistory->StartTransaction("Snap to Position");

  bool bDidAny = false;

  for (ezUInt32 sel = 0; sel < gizmoSelection.GetCount(); ++sel)
  {
    const auto& obj = gizmoSelection[sel];

    ezTransform vSnappedPos = obj.m_GlobalTransform;

    // if we snap each object individually, compute the snap position for each one here
    if (bSnapEachObject)
    {
      vSnappedPos.m_vPosition = obj.m_GlobalTransform.m_vPosition;
      ezSnapProvider::SnapTranslation(vSnappedPos.m_vPosition);

      if (obj.m_GlobalTransform.m_vPosition == vSnappedPos.m_vPosition)
        continue;
    }
    else
    {
      // otherwise use the offset from the pivot point for repositioning
      vSnappedPos.m_vPosition += vPivotSnapOffset;
    }

    bDidAny = true;
    GetSceneDocument()->SetGlobalTransform(obj.m_pObject, vSnappedPos, TransformationChanges::Translation);
  }

  if (bDidAny)
    CmdHistory->FinishTransaction();
  else
    CmdHistory->CancelTransaction();

  gizmoSelection.Clear();
}

void ezQtSceneDocumentWindow::GameObjectEventHandler(const ezGameObjectEvent& e)
{
  switch (e.m_Type)
  {
  case ezGameObjectEvent::Type::TriggerFocusOnSelection_Hovered:
    GetSceneDocument()->ShowOrHideSelectedObjects(ezSceneDocument::ShowOrHide::Show);
    FocusOnSelectionHoveredView();
    break;

  case ezGameObjectEvent::Type::TriggerFocusOnSelection_All:
    GetSceneDocument()->ShowOrHideSelectedObjects(ezSceneDocument::ShowOrHide::Show);
    FocusOnSelectionAllViews();
    break;

  case ezGameObjectEvent::Type::TriggerSnapSelectionPivotToGrid:
    SnapSelectionToPosition(false);
    break;

  case ezGameObjectEvent::Type::TriggerSnapEachSelectedObjectToGrid:
    SnapSelectionToPosition(true);
    break;
  }
}

void ezQtSceneDocumentWindow::OnPreferenceChange(ezPreferences* pref)
{
  ezScenePreferencesUser* pPref = ezDynamicCast<ezScenePreferencesUser*>(pref);
  m_GizmoHandler.Borrow()->GetTranslateGizmo().SetCameraSpeed(ezCameraMoveContext::ConvertCameraSpeed(pPref->GetCameraSpeed()));
}

void ezQtSceneDocumentWindow::FocusOnSelectionAllViews()
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

void ezQtSceneDocumentWindow::FocusOnSelectionHoveredView()
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

void ezQtSceneDocumentWindow::InternalRedraw()
{
  // If play the game is on, only render (in editor) if the window is active
  ezSceneDocument* doc = GetSceneDocument();
  if (doc->GetGameMode() == GameMode::Play && !window()->isActiveWindow())
    return;

  ezEditorInputContext::UpdateActiveInputContext();
  SendRedrawMsg();
  ezQtEngineDocumentWindow::InternalRedraw();
}

void ezQtSceneDocumentWindow::SendRedrawMsg()
{
  // do not try to redraw while the process is crashed, it is obviously futile
  if (ezEditorEngineProcessConnection::GetSingleton()->IsProcessCrashed())
    return;

  auto pSceneDoc = GetSceneDocument();

  ezScenePreferencesUser* pPreferences = ezPreferences::QueryPreferences<ezScenePreferencesUser>(GetDocument());

  {
    ezSceneSettingsMsgToEngine msg;
    msg.m_bSimulateWorld = pSceneDoc->GetGameMode() != GameMode::Off;
    msg.m_fSimulationSpeed = pSceneDoc->GetSimulationSpeed();
    msg.m_fGizmoScale = ezPreferences::QueryPreferences<ezEditorPreferencesUser>()->m_fGizmoScale;
    msg.m_bRenderOverlay = pSceneDoc->GetRenderSelectionOverlay();
    msg.m_bRenderShapeIcons = pSceneDoc->GetRenderShapeIcons();
    msg.m_bRenderSelectionBoxes = pSceneDoc->GetRenderVisualizers();
    msg.m_bAddAmbientLight = pSceneDoc->GetAddAmbientLight();
    msg.m_fGridDensity = ezSnapProvider::GetTranslationSnapValue() * (pSceneDoc->GetGizmoWorldSpace() ? 1.0f : -1.0f); // negative density = local space
    msg.m_vGridTangent1.SetZero(); // indicates that the grid is disabled
    msg.m_vGridTangent2.SetZero(); // indicates that the grid is disabled

    ezTranslateGizmo& translateGizmo = m_GizmoHandler.Borrow()->GetTranslateGizmo();
    if (pPreferences->GetShowGrid() && translateGizmo.IsVisible())
    {
      msg.m_vGridCenter = translateGizmo.GetStartPosition();

      if (translateGizmo.GetTranslateMode() == ezTranslateGizmo::TranslateMode::Axis)
        msg.m_vGridCenter = translateGizmo.GetTransformation().m_vPosition;

      if (pSceneDoc->GetGizmoWorldSpace())
      {
        ezSnapProvider::SnapTranslation(msg.m_vGridCenter);

        switch (translateGizmo.GetLastPlaneInteraction())
        {
        case ezTranslateGizmo::PlaneInteraction::PlaneX:
          msg.m_vGridCenter.y = ezMath::Round(msg.m_vGridCenter.y, ezSnapProvider::GetTranslationSnapValue() * 10);
          msg.m_vGridCenter.z = ezMath::Round(msg.m_vGridCenter.z, ezSnapProvider::GetTranslationSnapValue() * 10);
          break;
        case ezTranslateGizmo::PlaneInteraction::PlaneY:
          msg.m_vGridCenter.x = ezMath::Round(msg.m_vGridCenter.x, ezSnapProvider::GetTranslationSnapValue() * 10);
          msg.m_vGridCenter.z = ezMath::Round(msg.m_vGridCenter.z, ezSnapProvider::GetTranslationSnapValue() * 10);
          break;
        case ezTranslateGizmo::PlaneInteraction::PlaneZ:
          msg.m_vGridCenter.x = ezMath::Round(msg.m_vGridCenter.x, ezSnapProvider::GetTranslationSnapValue() * 10);
          msg.m_vGridCenter.y = ezMath::Round(msg.m_vGridCenter.y, ezSnapProvider::GetTranslationSnapValue() * 10);
          break;
        }
      }

      switch (translateGizmo.GetLastPlaneInteraction())
      {
      case ezTranslateGizmo::PlaneInteraction::PlaneX:
        msg.m_vGridTangent1 = translateGizmo.GetTransformation().m_qRotation * ezVec3(0, 1, 0);
        msg.m_vGridTangent2 = translateGizmo.GetTransformation().m_qRotation * ezVec3(0, 0, 1);
        break;
      case ezTranslateGizmo::PlaneInteraction::PlaneY:
        msg.m_vGridTangent1 = translateGizmo.GetTransformation().m_qRotation * ezVec3(1, 0, 0);
        msg.m_vGridTangent2 = translateGizmo.GetTransformation().m_qRotation * ezVec3(0, 0, 1);
        break;
      case ezTranslateGizmo::PlaneInteraction::PlaneZ:
        msg.m_vGridTangent1 = translateGizmo.GetTransformation().m_qRotation * ezVec3(1, 0, 0);
        msg.m_vGridTangent2 = translateGizmo.GetTransformation().m_qRotation * ezVec3(0, 1, 0);
        break;
      }
    }

    GetEditorEngineConnection()->SendMessage(&msg);
  }

  pSceneDoc->SendObjectSelection();

  auto pHoveredView = GetHoveredViewWidget();

  for (auto pView : m_ViewWidgets)
  {
    pView->SetEnablePicking(pView == pHoveredView);
    pView->UpdateCameraInterpolation();
    pView->SyncToEngine();
  }
}

void ezQtSceneDocumentWindow::HandleFocusOnSelection(const ezQuerySelectionBBoxResultMsgToEditor* pMsg, ezQtSceneViewWidget* pSceneView)
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

  if (cam.GetCameraMode() == ezCameraMode::PerspectiveFixedFovX ||
      cam.GetCameraMode() == ezCameraMode::PerspectiveFixedFovY)
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

void ezQtSceneDocumentWindow::ProcessMessageEventHandler(const ezEditorEngineDocumentMsg* pMsg)
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
          HandleFocusOnSelection(msg, static_cast<ezQtSceneViewWidget*>(pView));
      }
    }
    else
    {
      ezQtSceneViewWidget* pSceneView = static_cast<ezQtSceneViewWidget*>(GetViewWidgetByID(msg->m_uiViewID));

      if (!pSceneView)
        return;

      if (msg->m_iPurpose == 0)
        HandleFocusOnSelection(msg, pSceneView);
    }

    return;
  }

}

void ezQtSceneDocumentWindow::SnapProviderEventHandler(const ezSnapProviderEvent& e)
{
  switch (e.m_Type)
  {
  case ezSnapProviderEvent::Type::RotationSnapChanged:
    ShowStatusBarMsg(ezFmt(ezStringUtf8(L"Snapping Angle: {0}°").GetData(), ezSnapProvider::GetRotationSnapValue().GetDegree()));
    break;

  case ezSnapProviderEvent::Type::ScaleSnapChanged:
    ShowStatusBarMsg(ezFmt("Snapping Value: {0}", ezSnapProvider::GetScaleSnapValue()));
    break;

  case ezSnapProviderEvent::Type::TranslationSnapChanged:
    ShowStatusBarMsg(ezFmt("Snapping Value: {0}", ezSnapProvider::GetTranslationSnapValue()));
    break;
  }
}


