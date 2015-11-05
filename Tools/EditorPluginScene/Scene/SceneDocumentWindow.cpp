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

ezQtSceneDocumentWindow::ezQtSceneDocumentWindow(ezDocument* pDocument)
  : ezQtEngineDocumentWindow(pDocument)
{
  QWidget* pCenter = new QWidget(this);
  m_pViewLayout = new QGridLayout(pCenter);
  m_pViewLayout->setMargin(0);
  m_pViewLayout->setSpacing(4);
  pCenter->setLayout(m_pViewLayout);

  setCentralWidget(pCenter);

  SetupDefaultViewConfigs();
  LoadViewConfigs();

  m_bResendSelection = false;
  m_bInGizmoInteraction = false;
  SetTargetFramerate(35);

  GetDocument()->GetObjectManager()->m_StructureEvents.AddEventHandler(ezMakeDelegate(&ezQtSceneDocumentWindow::DocumentTreeEventHandler, this));
  GetDocument()->GetObjectManager()->m_PropertyEvents.AddEventHandler(ezMakeDelegate(&ezQtSceneDocumentWindow::PropertyEventHandler, this));

  GetSceneDocument()->m_ObjectMetaData.m_DataModifiedEvent.AddEventHandler(ezMakeDelegate(&ezQtSceneDocumentWindow::SceneObjectMetaDataEventHandler, this));

  {
    // Menu Bar
    ezMenuBarActionMapView* pMenuBar = static_cast<ezMenuBarActionMapView*>(menuBar());
    ezActionContext context;
    context.m_sMapping = "EditorPluginScene_DocumentMenuBar";
    context.m_pDocument = pDocument;
    pMenuBar->SetActionContext(context);
  }

  {
    // Tool Bar
    ezToolBarActionMapView* pToolBar = new ezToolBarActionMapView(this);
    ezActionContext context;
    context.m_sMapping = "EditorPluginScene_DocumentToolBar";
    context.m_pDocument = pDocument;
    pToolBar->SetActionContext(context);
    pToolBar->setObjectName("SceneDocumentWindow_ToolBar");
    addToolBar(pToolBar);
  }

  ezSceneDocument* pSceneDoc = static_cast<ezSceneDocument*>(GetDocument());
  pSceneDoc->m_SceneEvents.AddEventHandler(ezMakeDelegate(&ezQtSceneDocumentWindow::DocumentEventHandler, this));

  pSceneDoc->GetSelectionManager()->m_Events.AddEventHandler(ezMakeDelegate(&ezQtSceneDocumentWindow::SelectionManagerEventHandler, this));

  // TODO: (works but..) give the gizmo the proper view? remove the view from the input context altogether?
  m_TranslateGizmo.SetOwner(this, m_ViewWidgets[0]);
  m_RotateGizmo.SetOwner(this, m_ViewWidgets[0]);
  m_ScaleGizmo.SetOwner(this, m_ViewWidgets[0]);
  m_DragToPosGizmo.SetOwner(this, m_ViewWidgets[0]);

  m_RotateGizmo.SetSnappingAngle(ezAngle::Degree(ezRotateGizmoAction::GetCurrentSnappingValue()));
  m_ScaleGizmo.SetSnappingValue(ezScaleGizmoAction::GetCurrentSnappingValue());
  m_TranslateGizmo.SetSnappingValue(ezTranslateGizmoAction::GetCurrentSnappingValue());

  m_TranslateGizmo.m_GizmoEvents.AddEventHandler(ezMakeDelegate(&ezQtSceneDocumentWindow::TransformationGizmoEventHandler, this));
  m_RotateGizmo.m_GizmoEvents.AddEventHandler(ezMakeDelegate(&ezQtSceneDocumentWindow::TransformationGizmoEventHandler, this));
  m_ScaleGizmo.m_GizmoEvents.AddEventHandler(ezMakeDelegate(&ezQtSceneDocumentWindow::TransformationGizmoEventHandler, this));
  m_DragToPosGizmo.m_GizmoEvents.AddEventHandler(ezMakeDelegate(&ezQtSceneDocumentWindow::TransformationGizmoEventHandler, this));
  pSceneDoc->GetObjectManager()->m_StructureEvents.AddEventHandler(ezMakeDelegate(&ezQtSceneDocumentWindow::ObjectStructureEventHandler, this));
  pSceneDoc->GetCommandHistory()->m_Events.AddEventHandler(ezMakeDelegate(&ezQtSceneDocumentWindow::CommandHistoryEventHandler, this));
  ezRotateGizmoAction::s_Events.AddEventHandler(ezMakeDelegate(&ezQtSceneDocumentWindow::RotateGizmoEventHandler, this));
  ezScaleGizmoAction::s_Events.AddEventHandler(ezMakeDelegate(&ezQtSceneDocumentWindow::ScaleGizmoEventHandler, this));
  ezTranslateGizmoAction::s_Events.AddEventHandler(ezMakeDelegate(&ezQtSceneDocumentWindow::TranslateGizmoEventHandler, this));


  {
    ezDocumentPanel* pPropertyPanel = new ezDocumentPanel(this);
    pPropertyPanel->setObjectName("PropertyPanel");
    pPropertyPanel->setWindowTitle("Properties");
    pPropertyPanel->show();

    ezDocumentPanel* pPanelTree = new ezScenegraphPanel(this, static_cast<ezSceneDocument*>(pDocument));
    pPanelTree->show();

    ezPropertyGridWidget* pPropertyGrid = new ezPropertyGridWidget(pDocument, pPropertyPanel);
    pPropertyPanel->setWidget(pPropertyGrid);

    addDockWidget(Qt::DockWidgetArea::RightDockWidgetArea, pPropertyPanel);
    addDockWidget(Qt::DockWidgetArea::LeftDockWidgetArea, pPanelTree);
  }

  FinishWindowCreation();
}

ezQtSceneDocumentWindow::~ezQtSceneDocumentWindow()
{
  SaveViewConfigs();

  ezSceneDocument* pSceneDoc = static_cast<ezSceneDocument*>(GetDocument());
  pSceneDoc->m_SceneEvents.RemoveEventHandler(ezMakeDelegate(&ezQtSceneDocumentWindow::DocumentEventHandler, this));
  GetSceneDocument()->m_ObjectMetaData.m_DataModifiedEvent.RemoveEventHandler(ezMakeDelegate(&ezQtSceneDocumentWindow::SceneObjectMetaDataEventHandler, this));

  m_TranslateGizmo.m_GizmoEvents.RemoveEventHandler(ezMakeDelegate(&ezQtSceneDocumentWindow::TransformationGizmoEventHandler, this));
  m_RotateGizmo.m_GizmoEvents.RemoveEventHandler(ezMakeDelegate(&ezQtSceneDocumentWindow::TransformationGizmoEventHandler, this));
  m_ScaleGizmo.m_GizmoEvents.RemoveEventHandler(ezMakeDelegate(&ezQtSceneDocumentWindow::TransformationGizmoEventHandler, this));
  m_DragToPosGizmo.m_GizmoEvents.RemoveEventHandler(ezMakeDelegate(&ezQtSceneDocumentWindow::TransformationGizmoEventHandler, this));
  pSceneDoc->GetObjectManager()->m_StructureEvents.RemoveEventHandler(ezMakeDelegate(&ezQtSceneDocumentWindow::ObjectStructureEventHandler, this));
  pSceneDoc->GetCommandHistory()->m_Events.RemoveEventHandler(ezMakeDelegate(&ezQtSceneDocumentWindow::CommandHistoryEventHandler, this));
  ezRotateGizmoAction::s_Events.RemoveEventHandler(ezMakeDelegate(&ezQtSceneDocumentWindow::RotateGizmoEventHandler, this));
  ezScaleGizmoAction::s_Events.RemoveEventHandler(ezMakeDelegate(&ezQtSceneDocumentWindow::ScaleGizmoEventHandler, this));
  ezTranslateGizmoAction::s_Events.RemoveEventHandler(ezMakeDelegate(&ezQtSceneDocumentWindow::TranslateGizmoEventHandler, this));

  GetDocument()->GetSelectionManager()->m_Events.RemoveEventHandler(ezMakeDelegate(&ezQtSceneDocumentWindow::SelectionManagerEventHandler, this));

  GetDocument()->GetObjectManager()->m_PropertyEvents.RemoveEventHandler(ezMakeDelegate(&ezQtSceneDocumentWindow::PropertyEventHandler, this));
  GetDocument()->GetObjectManager()->m_StructureEvents.RemoveEventHandler(ezMakeDelegate(&ezQtSceneDocumentWindow::DocumentTreeEventHandler, this));
}

void ezQtSceneDocumentWindow::PropertyEventHandler(const ezDocumentObjectPropertyEvent& e)
{
  m_pEngineConnection->SendObjectProperties(e);
}

void ezQtSceneDocumentWindow::CommandHistoryEventHandler(const ezCommandHistory::Event& e)
{
  switch (e.m_Type)
  {
  case ezCommandHistory::Event::Type::ExecutedUndo:
  case ezCommandHistory::Event::Type::ExecutedRedo:
  case ezCommandHistory::Event::Type::AfterEndTransaction:
    {
      UpdateGizmoVisibility();
    }
    break;

  }

}

void ezQtSceneDocumentWindow::SnapSelectionToPosition(bool bSnapEachObject)
{
  const float fSnap = ezTranslateGizmoAction::GetCurrentSnappingValue();

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
    ezVec3 vSnappedPos;
    vSnappedPos.x = ezMath::Round(vPivotPos.x, fSnap);
    vSnappedPos.y = ezMath::Round(vPivotPos.y, fSnap);
    vSnappedPos.z = ezMath::Round(vPivotPos.z, fSnap);

    vPivotSnapOffset = vSnappedPos - vPivotPos;

    if (vPivotSnapOffset.IsZero())
      return;
  }

  UpdateGizmoSelectionList();

  if (m_GizmoSelection.IsEmpty())
    return;

  auto CmdHistory = GetDocument()->GetCommandHistory();

  CmdHistory->StartTransaction();

  bool bDidAny = false;

  for (ezUInt32 sel = 0; sel < m_GizmoSelection.GetCount(); ++sel)
  {
    const auto& obj = m_GizmoSelection[sel];

    ezTransform vSnappedPos = obj.m_GlobalTransform;

    // if we snap each object individually, compute the snap position for each one here
    if (bSnapEachObject)
    {
      vSnappedPos.m_vPosition.x = ezMath::Round(obj.m_GlobalTransform.m_vPosition.x, fSnap);
      vSnappedPos.m_vPosition.y = ezMath::Round(obj.m_GlobalTransform.m_vPosition.y, fSnap);
      vSnappedPos.m_vPosition.z = ezMath::Round(obj.m_GlobalTransform.m_vPosition.z, fSnap);

      if (obj.m_GlobalTransform.m_vPosition == vSnappedPos.m_vPosition)
        continue;
    }
    else
    {
      // otherwise use the offset from the pivot point for repositioning
      vSnappedPos.m_vPosition += vPivotSnapOffset;
    }

    bDidAny = true;
    GetSceneDocument()->SetGlobalTransform(obj.m_pObject, vSnappedPos);
  }

  if (bDidAny)
    CmdHistory->FinishTransaction();
  else
    CmdHistory->CancelTransaction();

  m_GizmoSelection.Clear();
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

void ezQtSceneDocumentWindow::DocumentEventHandler(const ezSceneDocument::SceneEvent& e)
{
  switch (e.m_Type)
  {
  case ezSceneDocument::SceneEvent::Type::ActiveGizmoChanged:
    UpdateGizmoVisibility();
    break;

  case ezSceneDocument::SceneEvent::Type::FocusOnSelection_Hovered:
    GetSceneDocument()->ShowOrHideSelectedObjects(ezSceneDocument::ShowOrHide::Show);
    FocusOnSelectionHoveredView();
    break;

  case ezSceneDocument::SceneEvent::Type::FocusOnSelection_All:
    GetSceneDocument()->ShowOrHideSelectedObjects(ezSceneDocument::ShowOrHide::Show);
    FocusOnSelectionAllViews();
    break;

  case ezSceneDocument::SceneEvent::Type::SnapSelectionPivotToGrid:
    SnapSelectionToPosition(false);
    break;

  case ezSceneDocument::SceneEvent::Type::SnapEachSelectedObjectToGrid:
    SnapSelectionToPosition(true);
    break;

  case ezSceneDocument::SceneEvent::Type::ExportScene:
    {
      const ezAssetDocumentInfo* pInfo = static_cast<const ezAssetDocumentInfo*>(GetSceneDocument()->GetDocumentInfo());

      ezExportSceneMsgToEngine msg;
      msg.m_sOutputFile = GetSceneDocument()->GetBinaryTargetFile();
      msg.m_uiAssetHash = pInfo->m_uiSettingsHash;

      GetEditorEngineConnection()->SendMessage(&msg);
    }
    break;
  }
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
  SendMessageToEngine(&msg);
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
  SendMessageToEngine(&msg);
}

void ezQtSceneDocumentWindow::SendObjectMsg(const ezDocumentObject* pObj, ezObjectTagMsgToEngine* pMsg)
{
  // if ezObjectTagMsgToEngine were derived from a general 'object msg' one could send other message types as well

  if (!pObj->GetTypeAccessor().GetType()->IsDerivedFrom<ezGameObject>())
    return;

  pMsg->m_ObjectGuid = pObj->GetGuid();
  GetEditorEngineConnection()->SendMessage(pMsg);
}
void ezQtSceneDocumentWindow::SendObjectMsgRecursive(const ezDocumentObject* pObj, ezObjectTagMsgToEngine* pMsg)
{
  // if ezObjectTagMsgToEngine were derived from a general 'object msg' one could send other message types as well

  if (!pObj->GetTypeAccessor().GetType()->IsDerivedFrom<ezGameObject>())
    return;

  pMsg->m_ObjectGuid = pObj->GetGuid();
  GetEditorEngineConnection()->SendMessage(pMsg);

  for (auto pChild : pObj->GetChildren())
  {
    SendObjectMsgRecursive(pChild, pMsg);
  }
}

void ezQtSceneDocumentWindow::SendObjectSelection()
{
  if (!m_bResendSelection)
    return;

  m_bResendSelection = false;

  const auto& sel = GetDocument()->GetSelectionManager()->GetSelection();

  ezObjectSelectionMsgToEngine msg;
  ezStringBuilder sTemp;
  ezString sGuid;

  for (const auto& item : sel)
  {
    sGuid = ezConversionUtils::ToString(item->GetGuid());

    sTemp.Append(";", sGuid);
  }

  msg.m_sSelection = sTemp;

  m_pEngineConnection->SendMessage(&msg);
}

void ezQtSceneDocumentWindow::DocumentTreeEventHandler(const ezDocumentObjectStructureEvent& e)
{
  m_pEngineConnection->SendDocumentTreeChange(e);
}

void ezQtSceneDocumentWindow::InternalRedraw()
{
  ezQtEngineDocumentWindow::SyncObjectsToEngine();

  ezEditorInputContext::UpdateActiveInputContext();

  SendRedrawMsg();
}

void ezQtSceneDocumentWindow::SendRedrawMsg()
{
  // do not try to redraw while the process is crashed, it is obviously futile
  if (ezEditorEngineProcessConnection::GetInstance()->IsProcessCrashed())
    return;

  SendObjectSelection();

  auto pHoveredView = GetHoveredViewWidget();

  for (auto pView : m_ViewWidgets)
  {
    pView->SetEnablePicking(pView == pHoveredView);
    pView->UpdateCameraInterpolation();
    pView->SyncToEngine();
  }

  {
    ezSyncWithProcessMsgToEngine sm;
    ezEditorEngineProcessConnection::GetInstance()->SendMessage(&sm);

    ezEditorEngineProcessConnection::GetInstance()->WaitForMessage(ezGetStaticRTTI<ezSyncWithProcessMsgToEditor>(), ezTime::Seconds(2.0));
  }
}

void ezQtSceneDocumentWindow::SetupDefaultViewConfigs()
{
  m_ViewConfigSingle.m_Perspective = ezSceneViewPerspective::Perspective;
  m_ViewConfigSingle.m_RenderMode = ezViewRenderMode::Default;
  m_ViewConfigSingle.m_Camera.LookAt(ezVec3(0), ezVec3(1, 0, 0), ezVec3(0, 0, 1));
  m_ViewConfigSingle.ApplyPerspectiveSetting();

  for (int i = 0; i < 4; ++i)
  {
    m_ViewConfigQuad[i].m_Perspective = (ezSceneViewPerspective::Enum)(ezSceneViewPerspective::Orthogonal_Front + i);
    m_ViewConfigQuad[i].m_RenderMode = (i == ezSceneViewPerspective::Perspective) ? ezViewRenderMode::Default : ezViewRenderMode::WireframeMonochrome;
    m_ViewConfigQuad[i].m_Camera.LookAt(ezVec3(0), ezVec3(1, 0, 0), ezVec3(0, 0, 1));
    m_ViewConfigQuad[i].ApplyPerspectiveSetting();
  }
}

void ezQtSceneDocumentWindow::SaveViewConfig(QSettings& Settings, const ezSceneViewConfig& cfg, const char* szName) const
{
  ezStringBuilder s("ViewConfig_", szName);

  Settings.beginGroup(QLatin1String(s.GetData()));
  {
    Settings.setValue(QLatin1String("Perspective"), (int)cfg.m_Perspective);
    Settings.setValue(QLatin1String("Mode"), (int)cfg.m_RenderMode);
  }
  Settings.endGroup();
}

void ezQtSceneDocumentWindow::LoadViewConfig(QSettings& Settings, ezSceneViewConfig& cfg, const char* szName)
{
  ezStringBuilder s("ViewConfig_", szName);

  Settings.beginGroup(QLatin1String(s.GetData()));
  {
    cfg.m_Perspective = (ezSceneViewPerspective::Enum)Settings.value(QLatin1String("Perspective"), (int)cfg.m_Perspective).toInt();
    cfg.m_RenderMode = (ezViewRenderMode::Enum)Settings.value(QLatin1String("Mode"), (int)cfg.m_RenderMode).toInt();
  }
  Settings.endGroup();
}

void ezQtSceneDocumentWindow::SaveViewConfigs() const
{
  QSettings Settings;
  Settings.beginGroup(QLatin1String("SceneDocument"));
  {
    Settings.setValue("QuadView", m_ViewWidgets.GetCount() == 4);
    SaveViewConfig(Settings, m_ViewConfigSingle, "Single");
    SaveViewConfig(Settings, m_ViewConfigQuad[0], "Quad0");
    SaveViewConfig(Settings, m_ViewConfigQuad[1], "Quad1");
    SaveViewConfig(Settings, m_ViewConfigQuad[2], "Quad2");
    SaveViewConfig(Settings, m_ViewConfigQuad[3], "Quad3");
  }
  Settings.endGroup();
}

void ezQtSceneDocumentWindow::LoadViewConfigs()
{
  bool bQuadView = true;

  QSettings Settings;
  Settings.beginGroup(QLatin1String("SceneDocument"));
  {
    bQuadView = Settings.value("QuadView", bQuadView).toBool();
    LoadViewConfig(Settings, m_ViewConfigSingle, "Single");
    LoadViewConfig(Settings, m_ViewConfigQuad[0], "Quad0");
    LoadViewConfig(Settings, m_ViewConfigQuad[1], "Quad1");
    LoadViewConfig(Settings, m_ViewConfigQuad[2], "Quad2");
    LoadViewConfig(Settings, m_ViewConfigQuad[3], "Quad3");
  }
  Settings.endGroup();

  CreateViews(bQuadView);
}

void ezQtSceneDocumentWindow::CreateViews(bool bQuad)
{
  QtScopedUpdatesDisabled _(this);
  for (auto pContainer : m_ActiveMainViews)
  {
    delete pContainer;
  }
  m_ActiveMainViews.Clear();

  if (bQuad)
  {
    for (ezUInt32 i = 0; i < 4; ++i)
    {
      ezQtSceneViewWidgetContainer* pContainer = new ezQtSceneViewWidgetContainer(this, this, &m_CameraMoveSettings, &m_ViewConfigQuad[i]);
      m_ActiveMainViews.PushBack(pContainer);
      m_pViewLayout->addWidget(pContainer, i / 2, i % 2);
    }
  }
  else
  {
    ezQtSceneViewWidgetContainer* pContainer = new ezQtSceneViewWidgetContainer(this, this, &m_CameraMoveSettings, &m_ViewConfigSingle);
    m_ActiveMainViews.PushBack(pContainer);
    m_pViewLayout->addWidget(pContainer, 0, 0);
  }
}

void ezQtSceneDocumentWindow::HandleFocusOnSelection(const ezQuerySelectionBBoxResultMsgToEditor* pMsg, ezQtSceneViewWidget* pSceneView)
{
  /// \todo Object Pivot Offset
  /// \todo Zoom in / out only on second focus ?
  const ezVec3 vPivotPoint = pMsg->m_vCenter;

  const ezCamera& cam = pSceneView->m_pViewConfig->m_Camera;

  ezVec3 vNewCameraPosition = cam.GetCenterPosition();
  ezVec3 vNewCameraDirection = cam.GetDirForwards();
  float fNewFovOrDim = cam.GetFovOrDim();

  if (pSceneView->width() == 0 || pSceneView->height() == 0)
    return;

  const float fApsectRation = (float)pSceneView->width() / (float)pSceneView->height();

  if (cam.GetCameraMode() == ezCamera::PerspectiveFixedFovX ||
      cam.GetCameraMode() == ezCamera::PerspectiveFixedFovY)
  {
    const ezAngle fovX = cam.GetFovX(fApsectRation);
    const ezAngle fovY = cam.GetFovY(fApsectRation);

    ezBoundingBox bbox;
    bbox.SetCenterAndHalfExtents(pMsg->m_vCenter, pMsg->m_vHalfExtents);
    const float fRadius = bbox.GetBoundingSphere().m_fRadius * 1.5f;

    const float dist1 = fRadius / ezMath::Sin(fovX * 0.5);
    const float dist2 = fRadius / ezMath::Sin(fovY * 0.5);
    const float distBest = ezMath::Max(dist1, dist2);

    // Previous method that will additionally rotate the camera
    //// Hard coded 'up' axis
    //// make sure the camera is always above the objects center point, looking down, never up
    //ezVec3 vCamRefPoint = cam.GetCenterPosition();
    //vCamRefPoint.z = ezMath::Max(vCamRefPoint.z, pMsg->m_vCenter.z + pMsg->m_vHalfExtents.z * 0.5f);

    //vNewCameraDirection = vPivotPoint - vCamRefPoint;
    //vNewCameraDirection.NormalizeIfNotZero(ezVec3(1, 0, 0));

    //{
    //  const ezAngle maxAngle = ezAngle::Degree(30.0f);

    //  /// \todo Hard coded 'up' direction
    //  ezVec3 vPlaneDir = vNewCameraDirection;
    //  vPlaneDir.z = 0.0f;

    //  vPlaneDir.NormalizeIfNotZero(ezVec3(1, 0, 0));

    //  if (maxAngle < vPlaneDir.GetAngleBetween(vNewCameraDirection))
    //  {
    //    const ezVec3 vAxis = vPlaneDir.Cross(vNewCameraDirection).GetNormalized();
    //    ezMat3 mRot;
    //    mRot.SetRotationMatrix(vAxis, maxAngle);

    //    vNewCameraDirection = mRot * vPlaneDir;
    //  }
    //}

    vNewCameraPosition = vPivotPoint - vNewCameraDirection * distBest;
  }
  else
  {
    vNewCameraPosition = pMsg->m_vCenter;

    const ezVec3 right = cam.GetDirRight();
    const ezVec3 up = cam.GetDirUp();

    const float fSizeFactor = 2.0f;

    const float fRequiredWidth = ezMath::Abs(right.Dot(pMsg->m_vHalfExtents) * 2.0f) * fSizeFactor;
    const float fRequiredHeight = ezMath::Abs(up.Dot(pMsg->m_vHalfExtents) * 2.0f) * fSizeFactor;

    float fDimWidth, fDimHeight;

    if (cam.GetCameraMode() == ezCamera::OrthoFixedHeight)
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

    fNewFovOrDim *= fScaleDim;
  }

  pSceneView->m_pCameraMoveContext->SetOrbitPoint(vPivotPoint);
  pSceneView->InterpolateCameraTo(vNewCameraPosition, vNewCameraDirection, fNewFovOrDim);
}

void ezQtSceneDocumentWindow::SyncObjectHiddenState()
{
  for (auto pChild : GetDocument()->GetObjectManager()->GetRootObject()->GetChildren())
  {
    SyncObjectHiddenState(pChild);
  }
}

void ezQtSceneDocumentWindow::SyncObjectHiddenState(ezDocumentObject* pObject)
{
  const bool bHidden = GetSceneDocument()->m_ObjectMetaData.BeginReadMetaData(pObject->GetGuid())->m_bHidden;
  GetSceneDocument()->m_ObjectMetaData.EndReadMetaData();

  ezObjectTagMsgToEngine msg;
  msg.m_bSetTag = bHidden;
  msg.m_sTag = "EditorHidden";

  SendObjectMsg(pObject, &msg);

  for (auto pChild : pObject->GetChildren())
  {
    SyncObjectHiddenState(pChild);
  }
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

bool ezQtSceneDocumentWindow::HandleEngineMessage(const ezEditorEngineDocumentMsg* pMsg)
{
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
        return true;

      if (msg->m_iPurpose == 0)
        HandleFocusOnSelection(msg, pSceneView);
    }

    return true;
  }

  if (ezQtEngineDocumentWindow::HandleEngineMessage(pMsg))
  {
    if (pMsg->GetDynamicRTTI()->IsDerivedFrom<ezDocumentOpenResponseMsgToEditor>())
    {
      SyncObjectHiddenState();
    }

    return true;
  }

  return false;
}

void ezQtSceneDocumentWindow::SelectionManagerEventHandler(const ezSelectionManager::Event& e)
{
  m_bResendSelection = true;

  switch (e.m_Type)
  {
  case ezSelectionManager::Event::Type::SelectionCleared:
    {
      m_GizmoSelection.Clear();
      UpdateGizmoVisibility();
    }
    break;

  case ezSelectionManager::Event::Type::SelectionSet:
  case ezSelectionManager::Event::Type::ObjectAdded:
    {
      EZ_ASSERT_DEBUG(m_GizmoSelection.IsEmpty(), "This array should have been cleared when the gizmo lost focus");

      UpdateGizmoVisibility();
    }
    break;
  }
}

void ezQtSceneDocumentWindow::SceneObjectMetaDataEventHandler(const ezObjectMetaData<ezUuid, ezSceneObjectMetaData>::EventData& e)
{
  if ((e.m_uiModifiedFlags & ezSceneObjectMetaData::HiddenFlag) != 0)
  {
    ezObjectTagMsgToEngine msg;
    msg.m_bSetTag = e.m_pValue->m_bHidden;
    msg.m_sTag = "EditorHidden";

    SendObjectMsg(GetDocument()->GetObjectManager()->GetObject(e.m_ObjectKey), &msg);
  }
}

void ezQtSceneDocumentWindow::RotateGizmoEventHandler(const ezRotateGizmoAction::Event& e)
{
  switch (e.m_Type)
  {
  case ezRotateGizmoAction::Event::Type::SnapppingAngleChanged:
    m_RotateGizmo.SetSnappingAngle(ezAngle::Degree(ezRotateGizmoAction::GetCurrentSnappingValue()));
    break;
  }
}

void ezQtSceneDocumentWindow::ScaleGizmoEventHandler(const ezScaleGizmoAction::Event& e)
{
  switch (e.m_Type)
  {
  case ezScaleGizmoAction::Event::Type::SnapppingValueChanged:
    m_ScaleGizmo.SetSnappingValue(ezScaleGizmoAction::GetCurrentSnappingValue());
    break;
  }
}

void ezQtSceneDocumentWindow::TranslateGizmoEventHandler(const ezTranslateGizmoAction::Event& e)
{
  switch (e.m_Type)
  {
  case ezTranslateGizmoAction::Event::Type::SnapppingValueChanged:
    m_TranslateGizmo.SetSnappingValue(ezTranslateGizmoAction::GetCurrentSnappingValue());
    break;

  }
}

