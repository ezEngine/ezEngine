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
#include <InputContexts/OrthoGizmoContext.h>
#include <Core/Assets/AssetFileHeader.h>
#include <GuiFoundation/PropertyGrid/ManipulatorManager.h>
#include <EditorFramework/Preferences/EditorPreferences.h>
#include <EditorPluginScene/Preferences/ScenePreferences.h>
#include <EditorFramework/Gizmos/SnapProvider.h>
#include <ToolsFoundation/Command/TreeCommands.h>
#include <EditorFramework/Preferences/Preferences.h>

ezQtSceneDocumentWindow::ezQtSceneDocumentWindow(ezAssetDocument* pDocument)
  : ezQtEngineDocumentWindow(pDocument)
{
  QWidget* pCenter = new QWidget(this);
  m_pViewLayout = new QGridLayout(pCenter);
  m_pViewLayout->setMargin(0);
  m_pViewLayout->setSpacing(4);
  pCenter->setLayout(m_pViewLayout);

  setCentralWidget(pCenter);

  LoadViewConfigs();

  m_bInGizmoInteraction = false;
  SetTargetFramerate(25);


  {
    // Menu Bar
    ezQtMenuBarActionMapView* pMenuBar = static_cast<ezQtMenuBarActionMapView*>(menuBar());
    ezActionContext context;
    context.m_sMapping = "EditorPluginScene_DocumentMenuBar";
    context.m_pDocument = pDocument;
    pMenuBar->SetActionContext(context);
  }

  {
    // Tool Bar
    ezQtToolBarActionMapView* pToolBar = new ezQtToolBarActionMapView("Toolbar", this);
    ezActionContext context;
    context.m_sMapping = "EditorPluginScene_DocumentToolBar";
    context.m_pDocument = pDocument;
    pToolBar->SetActionContext(context);
    pToolBar->setObjectName("SceneDocumentWindow_ToolBar");
    addToolBar(pToolBar);
  }

  const ezSceneDocument* pSceneDoc = static_cast<const ezSceneDocument*>(GetDocument());
  pSceneDoc->m_SceneEvents.AddEventHandler(ezMakeDelegate(&ezQtSceneDocumentWindow::DocumentEventHandler, this));

  pSceneDoc->GetSelectionManager()->m_Events.AddEventHandler(ezMakeDelegate(&ezQtSceneDocumentWindow::SelectionManagerEventHandler, this));

  // TODO: (works but..) give the gizmo the proper view? remove the view from the input context altogether?
  m_TranslateGizmo.SetOwner(this, nullptr);
  m_RotateGizmo.SetOwner(this, nullptr);
  m_ScaleGizmo.SetOwner(this, nullptr);
  m_DragToPosGizmo.SetOwner(this, nullptr);

  m_TranslateGizmo.m_GizmoEvents.AddEventHandler(ezMakeDelegate(&ezQtSceneDocumentWindow::TransformationGizmoEventHandler, this));
  m_RotateGizmo.m_GizmoEvents.AddEventHandler(ezMakeDelegate(&ezQtSceneDocumentWindow::TransformationGizmoEventHandler, this));
  m_ScaleGizmo.m_GizmoEvents.AddEventHandler(ezMakeDelegate(&ezQtSceneDocumentWindow::TransformationGizmoEventHandler, this));
  m_DragToPosGizmo.m_GizmoEvents.AddEventHandler(ezMakeDelegate(&ezQtSceneDocumentWindow::TransformationGizmoEventHandler, this));
  pSceneDoc->GetObjectManager()->m_StructureEvents.AddEventHandler(ezMakeDelegate(&ezQtSceneDocumentWindow::ObjectStructureEventHandler, this));
  pSceneDoc->GetCommandHistory()->m_Events.AddEventHandler(ezMakeDelegate(&ezQtSceneDocumentWindow::CommandHistoryEventHandler, this));

  ezSnapProvider::s_Events.AddEventHandler(ezMakeDelegate(&ezQtSceneDocumentWindow::SnapProviderEventHandler, this));
  ezManipulatorManager::GetSingleton()->m_Events.AddEventHandler(ezMakeDelegate(&ezQtSceneDocumentWindow::ManipulatorManagerEventHandler, this));
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
  SaveViewConfigs();

  const ezSceneDocument* pSceneDoc = static_cast<const ezSceneDocument*>(GetDocument());
  pSceneDoc->m_SceneEvents.RemoveEventHandler(ezMakeDelegate(&ezQtSceneDocumentWindow::DocumentEventHandler, this));

  m_TranslateGizmo.m_GizmoEvents.RemoveEventHandler(ezMakeDelegate(&ezQtSceneDocumentWindow::TransformationGizmoEventHandler, this));
  m_RotateGizmo.m_GizmoEvents.RemoveEventHandler(ezMakeDelegate(&ezQtSceneDocumentWindow::TransformationGizmoEventHandler, this));
  m_ScaleGizmo.m_GizmoEvents.RemoveEventHandler(ezMakeDelegate(&ezQtSceneDocumentWindow::TransformationGizmoEventHandler, this));
  m_DragToPosGizmo.m_GizmoEvents.RemoveEventHandler(ezMakeDelegate(&ezQtSceneDocumentWindow::TransformationGizmoEventHandler, this));
  pSceneDoc->GetObjectManager()->m_StructureEvents.RemoveEventHandler(ezMakeDelegate(&ezQtSceneDocumentWindow::ObjectStructureEventHandler, this));
  pSceneDoc->GetCommandHistory()->m_Events.RemoveEventHandler(ezMakeDelegate(&ezQtSceneDocumentWindow::CommandHistoryEventHandler, this));
  ezPreferences::QueryPreferences<ezScenePreferencesUser>(GetDocument())->m_ChangedEvent.RemoveEventHandler(ezMakeDelegate(&ezQtSceneDocumentWindow::OnPreferenceChange, this));

  ezSnapProvider::s_Events.RemoveEventHandler(ezMakeDelegate(&ezQtSceneDocumentWindow::SnapProviderEventHandler, this));
  ezManipulatorManager::GetSingleton()->m_Events.RemoveEventHandler(ezMakeDelegate(&ezQtSceneDocumentWindow::ManipulatorManagerEventHandler, this));

  GetDocument()->GetSelectionManager()->m_Events.RemoveEventHandler(ezMakeDelegate(&ezQtSceneDocumentWindow::SelectionManagerEventHandler, this));
}

ezSceneDocument* ezQtSceneDocumentWindow::GetSceneDocument() const
{
  return static_cast<ezSceneDocument*>(GetDocument());
}

void ezQtSceneDocumentWindow::CommandHistoryEventHandler(const ezCommandHistoryEvent& e)
{
  switch (e.m_Type)
  {
  case ezCommandHistoryEvent::Type::UndoEnded:
  case ezCommandHistoryEvent::Type::RedoEnded:
  case ezCommandHistoryEvent::Type::TransactionEnded:
  case ezCommandHistoryEvent::Type::TransactionCanceled:
    {
      UpdateGizmoVisibility();
    }
    break;
  }
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

  UpdateGizmoSelectionList();

  if (m_GizmoSelection.IsEmpty())
    return;

  auto CmdHistory = GetDocument()->GetCommandHistory();

  CmdHistory->StartTransaction("Snap to Position");

  bool bDidAny = false;

  for (ezUInt32 sel = 0; sel < m_GizmoSelection.GetCount(); ++sel)
  {
    const auto& obj = m_GizmoSelection[sel];

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

  m_GizmoSelection.Clear();
}

void ezQtSceneDocumentWindow::UpdateManipulatorVisibility()
{
  ezManipulatorManager::GetSingleton()->HideActiveManipulator(GetDocument(), GetSceneDocument()->GetActiveGizmo() != ActiveGizmo::None);
}

void ezQtSceneDocumentWindow::ObjectStructureEventHandler(const ezDocumentObjectStructureEvent& e)
{
  if (m_bInGizmoInteraction)
    return;

  if (!m_TranslateGizmo.IsVisible() && !m_RotateGizmo.IsVisible() && !m_ScaleGizmo.IsVisible() && !m_DragToPosGizmo.IsVisible())
    return;

  switch (e.m_EventType)
  {
  case ezDocumentObjectStructureEvent::Type::AfterObjectRemoved:
    {
      UpdateGizmoVisibility();
    }
    break;
  }
}

void ezQtSceneDocumentWindow::DocumentEventHandler(const ezSceneDocumentEvent& e)
{
  switch (e.m_Type)
  {
  case ezSceneDocumentEvent::Type::ActiveGizmoChanged:
    UpdateGizmoVisibility();
    //if (!m_bIgnoreGizmoChangedEvent)
    {
      UpdateManipulatorVisibility();
    }
    break;

  case ezSceneDocumentEvent::Type::FocusOnSelection_Hovered:
    GetSceneDocument()->ShowOrHideSelectedObjects(ezSceneDocument::ShowOrHide::Show);
    FocusOnSelectionHoveredView();
    break;

  case ezSceneDocumentEvent::Type::FocusOnSelection_All:
    GetSceneDocument()->ShowOrHideSelectedObjects(ezSceneDocument::ShowOrHide::Show);
    FocusOnSelectionAllViews();
    break;

  case ezSceneDocumentEvent::Type::SnapSelectionPivotToGrid:
    SnapSelectionToPosition(false);
    break;

  case ezSceneDocumentEvent::Type::SnapEachSelectedObjectToGrid:
    SnapSelectionToPosition(true);
    break;
  }
}

void ezQtSceneDocumentWindow::OnPreferenceChange(ezPreferences* pref)
{
  ezScenePreferencesUser* pPref = ezDynamicCast<ezScenePreferencesUser*>(pref);

  m_TranslateGizmo.SetCameraSpeed(ezCameraMoveContext::ConvertCameraSpeed(pPref->GetCameraSpeed()));
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

    if (pPreferences->GetShowGrid() && m_TranslateGizmo.IsVisible())
    {
      msg.m_vGridCenter = m_TranslateGizmo.GetStartPosition();

      if (m_TranslateGizmo.GetTranslateMode() == ezTranslateGizmo::TranslateMode::Axis)
        msg.m_vGridCenter = m_TranslateGizmo.GetTransformation().GetTranslationVector();

      if (pSceneDoc->GetGizmoWorldSpace())
      {
        ezSnapProvider::SnapTranslation(msg.m_vGridCenter);

        switch (m_TranslateGizmo.GetLastPlaneInteraction())
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

      switch (m_TranslateGizmo.GetLastPlaneInteraction())
      {
      case ezTranslateGizmo::PlaneInteraction::PlaneX:
        msg.m_vGridTangent1 = m_TranslateGizmo.GetTransformation().TransformDirection(ezVec3(0, 1, 0));
        msg.m_vGridTangent2 = m_TranslateGizmo.GetTransformation().TransformDirection(ezVec3(0, 0, 1));
        break;
      case ezTranslateGizmo::PlaneInteraction::PlaneY:
        msg.m_vGridTangent1 = m_TranslateGizmo.GetTransformation().TransformDirection(ezVec3(1, 0, 0));
        msg.m_vGridTangent2 = m_TranslateGizmo.GetTransformation().TransformDirection(ezVec3(0, 0, 1));
        break;
      case ezTranslateGizmo::PlaneInteraction::PlaneZ:
        msg.m_vGridTangent1 = m_TranslateGizmo.GetTransformation().TransformDirection(ezVec3(1, 0, 0));
        msg.m_vGridTangent2 = m_TranslateGizmo.GetTransformation().TransformDirection(ezVec3(0, 1, 0));
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

void ezQtSceneDocumentWindow::SaveViewConfig(const ezSceneViewConfig& cfg, ezSceneViewPreferences& pref) const
{
  ezScenePreferencesUser* pPreferences = ezPreferences::QueryPreferences<ezScenePreferencesUser>(GetDocument());

  pref.m_vCamPos = cfg.m_Camera.GetPosition();
  pref.m_vCamDir = cfg.m_Camera.GetDirForwards();
  pref.m_vCamUp = cfg.m_Camera.GetDirUp();
  pref.m_uiPerspectiveMode = cfg.m_Perspective;
  pref.m_uiRenderMode = cfg.m_RenderMode;
  pref.m_fFov = cfg.m_Camera.GetFovOrDim();
}

void ezQtSceneDocumentWindow::LoadViewConfig(ezSceneViewConfig& cfg, ezSceneViewPreferences& pref)
{
  cfg.m_Perspective = (ezSceneViewPerspective::Enum)pref.m_uiPerspectiveMode;
  cfg.m_RenderMode = (ezViewRenderMode::Enum)pref.m_uiRenderMode;
  cfg.m_Camera.LookAt(ezVec3(0), ezVec3(1, 0, 0), ezVec3(0, 0, 1));

  if (cfg.m_Perspective == ezSceneViewPerspective::Perspective)
  {
    ezEditorPreferencesUser* pPref = ezPreferences::QueryPreferences<ezEditorPreferencesUser>();
    cfg.ApplyPerspectiveSetting(pPref->m_fPerspectiveFieldOfView);
  }
  else
  {
    cfg.ApplyPerspectiveSetting(pref.m_fFov);
  }

  pref.m_vCamDir.NormalizeIfNotZero(ezVec3(1, 0, 0));
  pref.m_vCamUp.MakeOrthogonalTo(pref.m_vCamDir);
  pref.m_vCamUp.NormalizeIfNotZero(pref.m_vCamDir.GetOrthogonalVector().GetNormalized());

  cfg.m_Camera.LookAt(pref.m_vCamPos, pref.m_vCamPos + pref.m_vCamDir, pref.m_vCamUp);

}

void ezQtSceneDocumentWindow::SaveViewConfigs() const
{
  ezScenePreferencesUser* pPreferences = ezPreferences::QueryPreferences<ezScenePreferencesUser>(GetDocument());
  pPreferences->m_bQuadView = (m_ViewWidgets.GetCount() == 4);

  SaveViewConfig(m_ViewConfigSingle, pPreferences->m_ViewSingle);
  SaveViewConfig(m_ViewConfigQuad[0], pPreferences->m_ViewQuad0);
  SaveViewConfig(m_ViewConfigQuad[1], pPreferences->m_ViewQuad1);
  SaveViewConfig(m_ViewConfigQuad[2], pPreferences->m_ViewQuad2);
  SaveViewConfig(m_ViewConfigQuad[3], pPreferences->m_ViewQuad3);
}

void ezQtSceneDocumentWindow::LoadViewConfigs()
{
  ezScenePreferencesUser* pPreferences = ezPreferences::QueryPreferences<ezScenePreferencesUser>(GetDocument());

  LoadViewConfig(m_ViewConfigSingle, pPreferences->m_ViewSingle);
  LoadViewConfig(m_ViewConfigQuad[0], pPreferences->m_ViewQuad0);
  LoadViewConfig(m_ViewConfigQuad[1], pPreferences->m_ViewQuad1);
  LoadViewConfig(m_ViewConfigQuad[2], pPreferences->m_ViewQuad2);
  LoadViewConfig(m_ViewConfigQuad[3], pPreferences->m_ViewQuad3);

  CreateViews(pPreferences->m_bQuadView);
}

void ezQtSceneDocumentWindow::CreateViews(bool bQuad)
{
  ezQtScopedUpdatesDisabled _(this);
  for (auto pContainer : m_ActiveMainViews)
  {
    delete pContainer;
  }
  m_ActiveMainViews.Clear();

  if (bQuad)
  {
    for (ezUInt32 i = 0; i < 4; ++i)
    {
      ezQtSceneViewWidget* pViewWidget = new ezQtSceneViewWidget(nullptr, this, &m_ViewConfigQuad[i]);
      ezQtViewWidgetContainer* pContainer = new ezQtViewWidgetContainer(this, pViewWidget, "EditorPluginScene_ViewToolBar");
      m_ActiveMainViews.PushBack(pContainer);
      m_pViewLayout->addWidget(pContainer, i / 2, i % 2);

      pViewWidget->m_pOrthoGizmoContext->m_GizmoEvents.AddEventHandler(ezMakeDelegate(&ezQtSceneDocumentWindow::TransformationGizmoEventHandler, this));
    }
  }
  else
  {
    ezQtSceneViewWidget* pViewWidget = new ezQtSceneViewWidget(nullptr, this, &m_ViewConfigSingle);
    ezQtViewWidgetContainer* pContainer = new ezQtViewWidgetContainer(this, pViewWidget, "EditorPluginScene_ViewToolBar");
    m_ActiveMainViews.PushBack(pContainer);
    m_pViewLayout->addWidget(pContainer, 0, 0);

    pViewWidget->m_pOrthoGizmoContext->m_GizmoEvents.AddEventHandler(ezMakeDelegate(&ezQtSceneDocumentWindow::TransformationGizmoEventHandler, this));
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

void ezQtSceneDocumentWindow::ToggleViews(QWidget* pView)
{
  ezQtSceneViewWidget* pViewport = qobject_cast<ezQtSceneViewWidget*>(pView);
  EZ_ASSERT_DEV(pViewport != nullptr, "ezQtSceneDocumentWindow::ToggleViews must be called with a ezQtSceneViewWidget as parameter!");
  bool bIsQuad = m_ActiveMainViews.GetCount() == 4;
  if (bIsQuad)
  {
    m_ViewConfigSingle = *pViewport->m_pViewConfig;
    m_ViewConfigSingle.m_pLinkedViewConfig = pViewport->m_pViewConfig;
    CreateViews(false);
  }
  else
  {
    if (pViewport->m_pViewConfig->m_pLinkedViewConfig != nullptr)
      *pViewport->m_pViewConfig->m_pLinkedViewConfig = *pViewport->m_pViewConfig;

    CreateViews(true);
  }
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

void ezQtSceneDocumentWindow::SelectionManagerEventHandler(const ezSelectionManagerEvent& e)
{
  switch (e.m_Type)
  {
  case ezSelectionManagerEvent::Type::SelectionCleared:
    {
      m_GizmoSelection.Clear();
      UpdateGizmoVisibility();
    }
    break;

  case ezSelectionManagerEvent::Type::SelectionSet:
  case ezSelectionManagerEvent::Type::ObjectAdded:
    {
      EZ_ASSERT_DEBUG(m_GizmoSelection.IsEmpty(), "This array should have been cleared when the gizmo lost focus");

      UpdateGizmoVisibility();
    }
    break;
  }
}

void ezQtSceneDocumentWindow::ManipulatorManagerEventHandler(const ezManipulatorManagerEvent& e)
{
  // make sure the gizmo is deactivated when a manipulator becomes active
  if (e.m_pDocument == GetDocument() && e.m_pManipulator != nullptr && e.m_pSelection != nullptr && !e.m_pSelection->IsEmpty() && !e.m_bHideManipulators)
  {
    GetSceneDocument()->SetActiveGizmo(ActiveGizmo::None);
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

