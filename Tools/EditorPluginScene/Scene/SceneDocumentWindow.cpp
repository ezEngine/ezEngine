#include <PCH.h>
#include <EditorPluginScene/Scene/SceneDocumentWindow.moc.h>
#include <EditorPluginScene/Scene/SceneViewWidget.moc.h>
#include <EditorPluginScene/Scene/SceneDocument.h>
#include <GuiFoundation/ActionViews/MenuBarActionMapView.moc.h>
#include <GuiFoundation/ActionViews/ToolBarActionMapView.moc.h>
#include <EditorPluginScene/Panels/ScenegraphPanel/ScenegraphPanel.moc.h>
#include <EditorPluginScene/Panels/ObjectCreatorPanel/ObjectCreatorList.moc.h>
#include <GuiFoundation/PropertyGrid/PropertyGridWidget.moc.h>
#include <QGridLayout>

ezSceneDocumentWindow::ezSceneDocumentWindow(ezDocumentBase* pDocument)
  : ezDocumentWindow3D(pDocument)
{
  QWidget* pCenter = new QWidget(this);
  m_pViewLayout = new QGridLayout(pCenter);
  m_pViewLayout->setMargin(0);
  m_pViewLayout->setSpacing(4);
  pCenter->setLayout(m_pViewLayout);

  setCentralWidget(pCenter);

  SetupDefaultViewConfigs();
  CreateViews(true);

  m_bResendSelection = false;
  m_bInGizmoInteraction = false;
  SetTargetFramerate(35);

  GetDocument()->GetObjectManager()->m_StructureEvents.AddEventHandler(ezMakeDelegate(&ezSceneDocumentWindow::DocumentTreeEventHandler, this));
  GetDocument()->GetObjectManager()->m_PropertyEvents.AddEventHandler(ezMakeDelegate(&ezSceneDocumentWindow::PropertyEventHandler, this));

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
  pSceneDoc->m_SceneEvents.AddEventHandler(ezMakeDelegate(&ezSceneDocumentWindow::DocumentEventHandler, this));

  pSceneDoc->GetSelectionManager()->m_Events.AddEventHandler(ezMakeDelegate(&ezSceneDocumentWindow::SelectionManagerEventHandler, this));

  // TODO: (works but..) give the gizmo the proper view? remove the view from the input context altogether?
  m_TranslateGizmo.SetOwner(this, m_ViewWidgets[0]);
  m_RotateGizmo.SetOwner(this, m_ViewWidgets[0]);
  m_ScaleGizmo.SetOwner(this, m_ViewWidgets[0]);
  m_DragToPosGizmo.SetOwner(this, m_ViewWidgets[0]);
  
  m_RotateGizmo.SetSnappingAngle(ezAngle::Degree(ezRotateGizmoAction::GetCurrentSnappingValue()));
  m_ScaleGizmo.SetSnappingValue(ezScaleGizmoAction::GetCurrentSnappingValue());
  m_TranslateGizmo.SetSnappingValue(ezTranslateGizmoAction::GetCurrentSnappingValue());

  m_TranslateGizmo.m_BaseEvents.AddEventHandler(ezMakeDelegate(&ezSceneDocumentWindow::TransformationGizmoEventHandler, this));
  m_RotateGizmo.m_BaseEvents.AddEventHandler(ezMakeDelegate(&ezSceneDocumentWindow::TransformationGizmoEventHandler, this));
  m_ScaleGizmo.m_BaseEvents.AddEventHandler(ezMakeDelegate(&ezSceneDocumentWindow::TransformationGizmoEventHandler, this));
  m_DragToPosGizmo.m_BaseEvents.AddEventHandler(ezMakeDelegate(&ezSceneDocumentWindow::TransformationGizmoEventHandler, this));
  pSceneDoc->GetObjectManager()->m_StructureEvents.AddEventHandler(ezMakeDelegate(&ezSceneDocumentWindow::ObjectStructureEventHandler, this));
  pSceneDoc->GetCommandHistory()->m_Events.AddEventHandler(ezMakeDelegate(&ezSceneDocumentWindow::CommandHistoryEventHandler, this));
  ezRotateGizmoAction::s_Events.AddEventHandler(ezMakeDelegate(&ezSceneDocumentWindow::RotateGizmoEventHandler, this));
  ezScaleGizmoAction::s_Events.AddEventHandler(ezMakeDelegate(&ezSceneDocumentWindow::ScaleGizmoEventHandler, this));
  ezTranslateGizmoAction::s_Events.AddEventHandler(ezMakeDelegate(&ezSceneDocumentWindow::TranslateGizmoEventHandler, this));


  {
    ezDocumentPanel* pPropertyPanel = new ezDocumentPanel(this);
    pPropertyPanel->setObjectName("PropertyPanel");
    pPropertyPanel->setWindowTitle("Properties");
    pPropertyPanel->show();

    ezDocumentPanel* pPanelTree = new ezScenegraphPanel(this, static_cast<ezSceneDocument*>(pDocument));
    pPanelTree->show();

    ezDocumentPanel* pPanelCreator = new ezDocumentPanel(this);
    pPanelCreator->setObjectName("CreatorPanel");
    pPanelCreator->setWindowTitle("Object Creator");
    pPanelCreator->show();

    ezPropertyGridWidget* pPropertyGrid = new ezPropertyGridWidget(pDocument, pPropertyPanel);
    pPropertyPanel->setWidget(pPropertyGrid);

    ezObjectCreatorList* pCreatorWidget = new ezObjectCreatorList(pDocument->GetObjectManager(), pPanelCreator);
    pPanelCreator->setWidget(pCreatorWidget);

    addDockWidget(Qt::DockWidgetArea::RightDockWidgetArea, pPropertyPanel);
    addDockWidget(Qt::DockWidgetArea::LeftDockWidgetArea, pPanelTree);
    addDockWidget(Qt::DockWidgetArea::LeftDockWidgetArea, pPanelCreator);
  }
}

ezSceneDocumentWindow::~ezSceneDocumentWindow()
{
  ezSceneDocument* pSceneDoc = static_cast<ezSceneDocument*>(GetDocument());
  pSceneDoc->m_SceneEvents.RemoveEventHandler(ezMakeDelegate(&ezSceneDocumentWindow::DocumentEventHandler, this));

  m_TranslateGizmo.m_BaseEvents.RemoveEventHandler(ezMakeDelegate(&ezSceneDocumentWindow::TransformationGizmoEventHandler, this));
  m_RotateGizmo.m_BaseEvents.RemoveEventHandler(ezMakeDelegate(&ezSceneDocumentWindow::TransformationGizmoEventHandler, this));
  m_ScaleGizmo.m_BaseEvents.RemoveEventHandler(ezMakeDelegate(&ezSceneDocumentWindow::TransformationGizmoEventHandler, this));
  m_DragToPosGizmo.m_BaseEvents.RemoveEventHandler(ezMakeDelegate(&ezSceneDocumentWindow::TransformationGizmoEventHandler, this));
  pSceneDoc->GetObjectManager()->m_StructureEvents.RemoveEventHandler(ezMakeDelegate(&ezSceneDocumentWindow::ObjectStructureEventHandler, this));
  pSceneDoc->GetCommandHistory()->m_Events.RemoveEventHandler(ezMakeDelegate(&ezSceneDocumentWindow::CommandHistoryEventHandler, this));
  ezRotateGizmoAction::s_Events.RemoveEventHandler(ezMakeDelegate(&ezSceneDocumentWindow::RotateGizmoEventHandler, this));
  ezScaleGizmoAction::s_Events.RemoveEventHandler(ezMakeDelegate(&ezSceneDocumentWindow::ScaleGizmoEventHandler, this));
  ezTranslateGizmoAction::s_Events.RemoveEventHandler(ezMakeDelegate(&ezSceneDocumentWindow::TranslateGizmoEventHandler, this));

  GetDocument()->GetSelectionManager()->m_Events.RemoveEventHandler(ezMakeDelegate(&ezSceneDocumentWindow::SelectionManagerEventHandler, this));

  GetDocument()->GetObjectManager()->m_PropertyEvents.RemoveEventHandler(ezMakeDelegate(&ezSceneDocumentWindow::PropertyEventHandler, this));
  GetDocument()->GetObjectManager()->m_StructureEvents.RemoveEventHandler(ezMakeDelegate(&ezSceneDocumentWindow::DocumentTreeEventHandler, this));
}

void ezSceneDocumentWindow::PropertyEventHandler(const ezDocumentObjectPropertyEvent& e)
{
  m_pEngineConnection->SendObjectProperties(e);
}

void ezSceneDocumentWindow::CommandHistoryEventHandler(const ezCommandHistory::Event& e)
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

void ezSceneDocumentWindow::SnapSelectionToPosition(bool bSnapEachObject)
{
  const float fSnap = ezTranslateGizmoAction::GetCurrentSnappingValue();

  if (fSnap == 0.0f)
    return;

  const ezDeque<const ezDocumentObjectBase*>& selection = GetSceneDocument()->GetSelectionManager()->GetSelection();
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

void ezSceneDocumentWindow::ObjectStructureEventHandler(const ezDocumentObjectStructureEvent& e)
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

void ezSceneDocumentWindow::DocumentEventHandler(const ezSceneDocument::SceneEvent& e)
{
  switch (e.m_Type)
  {
  case ezSceneDocument::SceneEvent::Type::ActiveGizmoChanged:
    {
      UpdateGizmoVisibility();
    }
    break;

  case ezSceneDocument::SceneEvent::Type::FocusOnSelection:
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
      SendMessageToEngine(&msg, true);
    }
    break;

  case ezSceneDocument::SceneEvent::Type::SnapSelectionPivotToGrid:
    SnapSelectionToPosition(false);
    break;

  case ezSceneDocument::SceneEvent::Type::SnapEachSelectedObjectToGrid:
    SnapSelectionToPosition(true);
    break;
  
  case ezSceneDocument::SceneEvent::Type::HideSelectedObjects:
    HideSelectedObjects(true);
    break;

  case ezSceneDocument::SceneEvent::Type::HideUnselectedObjects:
    HideUnselectedObjects();
    break;

  case ezSceneDocument::SceneEvent::Type::ShowHiddenObjects:
    ShowHiddenObjects();
    break;
  }
}

void ezSceneDocumentWindow::SendObjectMsgRecursive(const ezDocumentObjectBase* pObj, ezObjectTagMsgToEngine* pMsg)
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

void ezSceneDocumentWindow::SendObjectSelection()
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

void ezSceneDocumentWindow::HideSelectedObjects(bool bHide)
{
  auto sel = GetDocument()->GetSelectionManager()->GetTopLevelSelection(ezGetStaticRTTI<ezGameObject>());

  ezObjectTagMsgToEngine msg;
  msg.m_bSetTag = bHide;
  msg.m_sTag = "EditorHidden";

  for (auto item : sel)
  {
    SendObjectMsgRecursive(item, &msg);
  }
}

void ezSceneDocumentWindow::HideUnselectedObjects()
{
  ezObjectTagMsgToEngine msg;
  msg.m_bSetTag = true;
  msg.m_sTag = "EditorHidden";

  // hide ALL
  for (auto pChild : GetDocument()->GetObjectManager()->GetRootObject()->GetChildren())
  {
    SendObjectMsgRecursive(pChild, &msg);
  }

  // unhide selected
  HideSelectedObjects(false);
}

void ezSceneDocumentWindow::ShowHiddenObjects()
{
  ezObjectTagMsgToEngine msg;
  msg.m_bSetTag = false;
  msg.m_sTag = "EditorHidden";

  // unhide ALL
  for (auto pChild : GetDocument()->GetObjectManager()->GetRootObject()->GetChildren())
  {
    SendObjectMsgRecursive(pChild, &msg);
  }
}

void ezSceneDocumentWindow::DocumentTreeEventHandler(const ezDocumentObjectStructureEvent& e)
{
  m_pEngineConnection->SendDocumentTreeChange(e);
}

void ezSceneDocumentWindow::InternalRedraw()
{
  ezDocumentWindow3D::SyncObjectsToEngine();

  ezEditorInputContext::UpdateActiveInputContext();

  SendRedrawMsg();
}

void ezSceneDocumentWindow::SendRedrawMsg()
{
  // do not try to redraw while the process is crashed, it is obviously futile
  if (ezEditorEngineProcessConnection::GetInstance()->IsProcessCrashed())
    return;

  SendObjectSelection();

  auto pHoveredView = GetHoveredViewWidget();

  for (auto pView : m_ViewWidgets)
  {
    pView->SyncToEngine();

    ezViewRedrawMsgToEngine msg;
    msg.m_uiViewID = pView->GetViewID();
    msg.m_uiHWND = (ezUInt64)(pView->winId());
    msg.m_uiWindowWidth = pView->width();
    msg.m_uiWindowHeight = pView->height();
    msg.m_bUpdatePickingData = pView == pHoveredView;

    m_pEngineConnection->SendMessage(&msg, true);
  }

  {
    ezSyncWithProcessMsgToEngine sm;
    ezEditorEngineProcessConnection::GetInstance()->SendMessage(&sm, true);

    ezEditorEngineProcessConnection::GetInstance()->WaitForMessage(ezGetStaticRTTI<ezSyncWithProcessMsgToEditor>(), ezTime::Seconds(2.0));
  }
}

void ezSceneDocumentWindow::SetupDefaultViewConfigs()
{
  m_ViewConfigSingle.m_Perspective = ezSceneViewPerspective::Perspective;
  m_ViewConfigSingle.m_RenderMode = ezViewRenderMode::Default;
  m_ViewConfigSingle.m_Camera.LookAt(ezVec3(0), ezVec3(1, 0, 0), ezVec3(0, 0, 1));
  m_ViewConfigSingle.ApplyPerspectiveSetting();
  
  for (int i = 0; i < 4; ++i)
  {
    m_ViewConfigQuad[i].m_Perspective = (ezSceneViewPerspective::Enum)(ezSceneViewPerspective::Orhogonal_Front + i);
    m_ViewConfigQuad[i].m_RenderMode = (i == ezSceneViewPerspective::Perspective) ? ezViewRenderMode::Default : ezViewRenderMode::WireframeMonochrome;
    m_ViewConfigQuad[i].m_Camera.LookAt(ezVec3(0), ezVec3(1, 0, 0), ezVec3(0, 0, 1));
    m_ViewConfigQuad[i].ApplyPerspectiveSetting();
  }
}

void ezSceneDocumentWindow::CreateViews(bool bQuad)
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
      ezSceneViewWidgetContainer* pContainer = new ezSceneViewWidgetContainer(this, this, &m_CameraMoveSettings, &m_ViewConfigQuad[i]);
      m_ActiveMainViews.PushBack(pContainer);
      m_pViewLayout->addWidget(pContainer, i / 2, i % 2);
    }
  }
  else
  {
    ezSceneViewWidgetContainer* pContainer = new ezSceneViewWidgetContainer(this, this, &m_CameraMoveSettings, &m_ViewConfigSingle);
    m_ActiveMainViews.PushBack(pContainer);
    m_pViewLayout->addWidget(pContainer, 0, 0);
  }
}

void ezSceneDocumentWindow::ToggleViews(QWidget* pView)
{
  ezSceneViewWidget* pViewport = qobject_cast<ezSceneViewWidget*>(pView);
  EZ_ASSERT_DEV(pViewport != nullptr, "ezSceneDocumentWindow::ToggleViews must be called with a ezSceneViewWidget as parameter!");
  bool bIsQuad = m_ActiveMainViews.GetCount() == 4;
  if (bIsQuad)
  {
    m_ViewConfigSingle = *pViewport->m_pViewConfig;
    CreateViews(false);
  }
  else
  {
    CreateViews(true);
  }
}

bool ezSceneDocumentWindow::HandleEngineMessage(const ezEditorEngineDocumentMsg* pMsg)
{
  if (pMsg->GetDynamicRTTI()->IsDerivedFrom<ezQuerySelectionBBoxResultMsgToEditor>())
  {
    const ezQuerySelectionBBoxResultMsgToEditor* msg = static_cast<const ezQuerySelectionBBoxResultMsgToEditor*>(pMsg);

    if (msg->m_uiViewID >= m_ViewWidgets.GetCount() || m_ViewWidgets[msg->m_uiViewID] == nullptr)
      return true;
    
    ezSceneViewWidget* pSceneView = static_cast<ezSceneViewWidget*>(m_ViewWidgets[msg->m_uiViewID]);

    /// \todo Object Pivot Offset
    /// \todo Zoom in / out only on second focus ?
    const ezVec3 vPivotPoint = msg->m_vCenter;

    ezVec3 vNewCameraPosition = pSceneView->m_pViewConfig->m_Camera.GetCenterPosition();
    ezVec3 vNewCameraDirection = pSceneView->m_pViewConfig->m_Camera.GetDirForwards();

    if (pSceneView->width() == 0 || pSceneView->height() == 0)
      return true;

    if (pSceneView->m_pViewConfig->m_Camera.GetCameraMode() == ezCamera::PerspectiveFixedFovX ||
      pSceneView->m_pViewConfig->m_Camera.GetCameraMode() == ezCamera::PerspectiveFixedFovY)
    {
      const ezAngle fovX = pSceneView->m_pViewConfig->m_Camera.GetFovX((float)pSceneView->width() / (float)pSceneView->height());
      const ezAngle fovY = pSceneView->m_pViewConfig->m_Camera.GetFovY((float)pSceneView->width() / (float)pSceneView->height());

      ezBoundingBox bbox;
      bbox.SetCenterAndHalfExtents(msg->m_vCenter, msg->m_vHalfExtents);
      const float fRadius = bbox.GetBoundingSphere().m_fRadius * 1.5f;

      const float dist1 = fRadius / ezMath::Sin(fovX * 0.5);
      const float dist2 = fRadius / ezMath::Sin(fovY * 0.5);
      const float distBest = ezMath::Max(dist1, dist2);

      vNewCameraDirection = vPivotPoint - pSceneView->m_pViewConfig->m_Camera.GetCenterPosition();
      vNewCameraDirection.NormalizeIfNotZero(ezVec3(1, 0, 0));

      {
        const ezAngle maxAngle = ezAngle::Degree(30.0f);

        /// \todo Hard coded 'up' direction
        ezVec3 vPlaneDir = vNewCameraDirection;
        vPlaneDir.z = 0.0f;

        vPlaneDir.NormalizeIfNotZero(ezVec3(1, 0, 0));

        if (maxAngle < vPlaneDir.GetAngleBetween(vNewCameraDirection))
        {
          const ezVec3 vAxis = vPlaneDir.Cross(vNewCameraDirection).GetNormalized();
          ezMat3 mRot;
          mRot.SetRotationMatrix(vAxis, maxAngle);

          vNewCameraDirection = mRot * vPlaneDir;
        }
      }

      vNewCameraPosition = vPivotPoint - vNewCameraDirection * distBest;
    }
    else
    {
      vNewCameraPosition = msg->m_vCenter;

      /// \todo Zoom In / Out
    }

    pSceneView->m_pCameraMoveContext->SetOrbitPoint(vPivotPoint);
    pSceneView->m_pCameraPositionContext->MoveToTarget(vNewCameraPosition, vNewCameraDirection);

    return true;
  }

  if (ezDocumentWindow3D::HandleEngineMessage(pMsg))
    return true;



  return false;
}

void ezSceneDocumentWindow::SelectionManagerEventHandler(const ezSelectionManager::Event& e)
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

void ezSceneDocumentWindow::RotateGizmoEventHandler(const ezRotateGizmoAction::Event& e)
{
  switch (e.m_Type)
  {
  case ezRotateGizmoAction::Event::Type::SnapppingAngleChanged:
    m_RotateGizmo.SetSnappingAngle(ezAngle::Degree(ezRotateGizmoAction::GetCurrentSnappingValue()));
    break;
  }
}

void ezSceneDocumentWindow::ScaleGizmoEventHandler(const ezScaleGizmoAction::Event& e)
{
  switch (e.m_Type)
  {
  case ezScaleGizmoAction::Event::Type::SnapppingValueChanged:
    m_ScaleGizmo.SetSnappingValue(ezScaleGizmoAction::GetCurrentSnappingValue());
    break;
  }
}

void ezSceneDocumentWindow::TranslateGizmoEventHandler(const ezTranslateGizmoAction::Event& e)
{
  switch (e.m_Type)
  {
  case ezTranslateGizmoAction::Event::Type::SnapppingValueChanged:
    m_TranslateGizmo.SetSnappingValue(ezTranslateGizmoAction::GetCurrentSnappingValue());
    break;

  }
}

