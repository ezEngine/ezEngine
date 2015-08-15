#include <PCH.h>
#include <EditorFramework/EditorApp/EditorApp.moc.h>
#include <EditorPluginScene/Scene/SceneDocumentWindow.moc.h>
#include <EditorPluginScene/Scene/SceneDocument.h>
#include <Core/ResourceManager/ResourceManager.h>
#include <Core/World/GameObject.h>
#include <RendererCore/Meshes/MeshBufferResource.h>
#include <CoreUtils/Geometry/GeomUtils.h>
#include <CoreUtils/Geometry/OBJLoader.h>
#include <Foundation/IO/OSFile.h>
#include <Foundation/Time/Time.h>
#include <Foundation/Reflection/ReflectionUtils.h>
#include <GuiFoundation/ActionViews/MenuBarActionMapView.moc.h>
#include <GuiFoundation/ActionViews/ToolBarActionMapView.moc.h>
#include <ToolsFoundation/Command/TreeCommands.h>
#include <EditorPluginScene/Panels/ScenegraphPanel/ScenegraphPanel.moc.h>
#include <EditorPluginScene/Panels/ObjectCreatorPanel/ObjectCreatorList.moc.h>
#include <EditorFramework/GUI/RawPropertyGridWidget.h>
#include <EditorFramework/EngineProcess/EngineProcessMessages.h>
#include <QTimer>
#include <QPushButton>
#include <qlayout.h>
#include <QKeyEvent>
#include <QMimeData>

ezSceneDocumentWindow::ezSceneDocumentWindow(ezDocumentBase* pDocument)
  : ezDocumentWindow3D(pDocument)
{
  m_pCenterWidget = new ezScene3DWidget(this, this);

  m_pCenterWidget->setAutoFillBackground(false);
  setCentralWidget(m_pCenterWidget);

  m_bInGizmoInteraction = false;
  SetTargetFramerate(35);

  GetDocument()->GetObjectManager()->m_StructureEvents.AddEventHandler(ezMakeDelegate(&ezSceneDocumentWindow::DocumentTreeEventHandler, this));
  GetDocument()->GetObjectManager()->m_PropertyEvents.AddEventHandler(ezMakeDelegate(&ezSceneDocumentWindow::PropertyEventHandler, this));

  m_Camera.SetCameraMode(ezCamera::CameraMode::PerspectiveFixedFovY, 80.0f, 0.1f, 1000.0f);
  m_Camera.LookAt(ezVec3(0.5f, 2.0f, 1.5f), ezVec3(0.0f, 0.0f, 0.5f), ezVec3(0.0f, 0.0f, 1.0f));

  m_pSelectionContext = EZ_DEFAULT_NEW(ezSelectionContext, pDocument, this, &m_Camera);
  m_pCameraMoveContext = EZ_DEFAULT_NEW(ezCameraMoveContext, m_pCenterWidget, pDocument, this);
  m_pCameraPositionContext = EZ_DEFAULT_NEW(ezCameraPositionContext, m_pCenterWidget, pDocument, this);

  m_pCameraMoveContext->LoadState();
  m_pCameraMoveContext->SetCamera(&m_Camera);

  m_pCameraPositionContext->SetCamera(&m_Camera);

  // add the input contexts in the order in which they are supposed to be processed
  m_pCenterWidget->m_InputContexts.PushBack(m_pSelectionContext);
  m_pCenterWidget->m_InputContexts.PushBack(m_pCameraMoveContext);


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

  m_TranslateGizmo.SetDocumentWindow3D(this);
  m_RotateGizmo.SetDocumentWindow3D(this);
  m_ScaleGizmo.SetDocumentWindow3D(this);
  m_DragToPosGizmo.SetDocumentWindow3D(this);

  m_TranslateGizmo.SetDocumentGuid(pDocument->GetGuid());
  m_RotateGizmo.SetDocumentGuid(pDocument->GetGuid());
  m_ScaleGizmo.SetDocumentGuid(pDocument->GetGuid());
  m_DragToPosGizmo.SetDocumentGuid(pDocument->GetGuid());

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

    ezRawPropertyGridWidget* pPropertyGrid = new ezRawPropertyGridWidget(pDocument, pPropertyPanel);
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

  EZ_DEFAULT_DELETE(m_pSelectionContext);
  EZ_DEFAULT_DELETE(m_pCameraMoveContext);
  EZ_DEFAULT_DELETE(m_pCameraPositionContext);
}

void ezSceneDocumentWindow::PropertyEventHandler(const ezDocumentObjectPropertyEvent& e)
{
  if (e.m_bEditorProperty)
    return;

  m_pEngineView->SendObjectProperties(e);
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

      const auto& LatestSelection = GetDocument()->GetSelectionManager()->GetSelection().PeekBack();
      const ezTransform tGlobal = GetSceneDocument()->GetGlobalTransform(LatestSelection);
      const ezVec3 vPivotPoint = tGlobal.m_vPosition + tGlobal.m_Rotation * LatestSelection->GetEditorTypeAccessor().GetValue("Pivot").ConvertTo<ezVec3>();

      ezVec3 vDiff = vPivotPoint - m_Camera.GetCenterPosition();
      if (vDiff.NormalizeIfNotZero().Failed())
        return;

      /// \todo The distance value of 5 is a hack, we need the bounding box of the selection for this
      const ezVec3 vTargetPos = vPivotPoint - vDiff * 5.0f;
      m_pCameraMoveContext->SetOrbitPoint(vPivotPoint);
      m_pCameraPositionContext->MoveToTarget(vTargetPos, vDiff);
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
  m_pEngineView->SendDocumentTreeChange(e);
}

void ezSceneDocumentWindow::InternalRedraw()
{
  ezDocumentWindow3D::SyncObjects();

  ezEditorInputContext::UpdateActiveInputContext();

  SendRedrawMsg();
}

void ezSceneDocumentWindow::SendRedrawMsg()
{
  // do not try to redraw while the process is crashed, it is obviously futile
  if (ezEditorEngineProcessConnection::GetInstance()->IsProcessCrashed())
    return;

  ezViewCameraMsgToEngine cam;
  cam.m_fNearPlane = m_Camera.GetNearPlane();
  cam.m_fFarPlane = m_Camera.GetFarPlane();
  cam.m_iCameraMode = (ezInt8)m_Camera.GetCameraMode();
  cam.m_fFovOrDim = m_Camera.GetFovOrDim();
  cam.m_vDirForwards = m_Camera.GetCenterDirForwards();
  cam.m_vDirUp = m_Camera.GetCenterDirUp();
  cam.m_vDirRight = m_Camera.GetCenterDirRight();
  cam.m_vPosition = m_Camera.GetCenterPosition();
  m_Camera.GetViewMatrix(cam.m_ViewMatrix);
  m_Camera.GetProjectionMatrix((float)m_pCenterWidget->width() / (float)m_pCenterWidget->height(), cam.m_ProjMatrix);

  m_pSelectionContext->SetWindowConfig(ezVec2I32(m_pCenterWidget->width(), m_pCenterWidget->height()));

  m_pEngineView->SendMessage(&cam);

  ezViewRedrawMsgToEngine msg;
  msg.m_uiHWND = (ezUInt64)(m_pCenterWidget->winId());
  msg.m_uiWindowWidth = m_pCenterWidget->width();
  msg.m_uiWindowHeight = m_pCenterWidget->height();

  m_pEngineView->SendMessage(&msg, true);

  ezEditorEngineProcessConnection::GetInstance()->WaitForMessage(ezGetStaticRTTI<ezViewRedrawFinishedMsgToEditor>(), ezTime::Seconds(1.0));
}

bool ezSceneDocumentWindow::HandleEngineMessage(const ezEditorEngineDocumentMsg* pMsg)
{
  if (ezDocumentWindow3D::HandleEngineMessage(pMsg))
    return true;



  return false;
}

void ezSceneDocumentWindow::SelectionManagerEventHandler(const ezSelectionManager::Event& e)
{
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


ezScene3DWidget::ezScene3DWidget(QWidget* pParent, ezDocumentWindow3D* pDocument) : ez3DViewWidget(pParent, pDocument)
{
  setAcceptDrops(true);
}

void ezScene3DWidget::dragEnterEvent(QDragEnterEvent* e)
{
  if (e->mimeData()->hasFormat("application/ezEditor.AssetGuid"))
  {
    m_LastDragMoveEvent = ezTime::Now();

    m_DraggedObjects.Clear();
    e->acceptProposedAction();

    m_pDocumentWindow->GetDocument()->GetCommandHistory()->StartTransaction();

    ezObjectPickingResult res = m_pDocumentWindow->PickObject(e->pos().x(), e->pos().y());

    if (res.m_vPickedPosition.IsNaN())
      res.m_vPickedPosition.SetZero();

    QByteArray ba = e->mimeData()->data("application/ezEditor.AssetGuid");
    QDataStream stream(&ba, QIODevice::ReadOnly);

    int iGuids = 0;
    stream >> iGuids;

    ezStringBuilder sTemp;

    for (ezInt32 i = 0; i < iGuids; ++i)
    {
      QString sGuid;
      stream >> sGuid;

      sTemp = sGuid.toUtf8().data();
      ezUuid AssetGuid = ezConversionUtils::ConvertStringToUuid(sTemp);

      if (ezAssetCurator::GetInstance()->GetAssetInfo(AssetGuid)->m_Info.m_sAssetTypeName != "Mesh")
        continue;

      m_DraggedObjects.PushBack(CreateDropObject(res.m_vPickedPosition, "ezMeshComponent", "MeshFile", sTemp));
    }

    if (m_DraggedObjects.IsEmpty())
      m_pDocumentWindow->GetDocument()->GetCommandHistory()->CancelTransaction();
    else
    {
      m_pDocumentWindow->GetDocument()->GetCommandHistory()->FinishTransaction();
      m_pDocumentWindow->GetDocument()->GetCommandHistory()->BeginTemporaryCommands();
    }
  }
}

void ezScene3DWidget::dragLeaveEvent(QDragLeaveEvent * e)
{
  if (!m_DraggedObjects.IsEmpty())
  {
    m_pDocumentWindow->GetDocument()->GetCommandHistory()->CancelTemporaryCommands();
    m_pDocumentWindow->GetDocument()->GetCommandHistory()->Undo();
    m_DraggedObjects.Clear();
  }
}

void ezScene3DWidget::dragMoveEvent(QDragMoveEvent* e)
{
  if (e->mimeData()->hasFormat("application/ezEditor.AssetGuid") && !m_DraggedObjects.IsEmpty())
  {
    const ezTime tNow = ezTime::Now();

    if (tNow - m_LastDragMoveEvent < ezTime::Seconds(1.0 / 25.0))
      return;

    m_LastDragMoveEvent = tNow;

    ezObjectPickingResult res = m_pDocumentWindow->PickObject(e->pos().x(), e->pos().y());

    MoveDraggedObjectsToPosition(res.m_vPickedPosition);
  }
}

void ezScene3DWidget::dropEvent(QDropEvent * e)
{
  if (e->mimeData()->hasFormat("application/ezEditor.AssetGuid") && !m_DraggedObjects.IsEmpty())
  {
    m_pDocumentWindow->GetDocument()->GetCommandHistory()->FinishTemporaryCommands();

    m_pDocumentWindow->GetDocument()->GetCommandHistory()->MergeLastTwoTransactions();

    ezDeque<const ezDocumentObjectBase*> NewSelection;

    for (const auto& guid : m_DraggedObjects)
    {
      NewSelection.PushBack(m_pDocumentWindow->GetDocument()->GetObjectManager()->GetObject(guid));
    }

    m_pDocumentWindow->GetDocument()->GetSelectionManager()->SetSelection(NewSelection);

    m_DraggedObjects.Clear();
  }
}

ezUuid ezScene3DWidget::CreateDropObject(const ezVec3& vPosition, const char* szType, const char* szProperty, const char* szValue)
{
  ezUuid ObjectGuid, CmpGuid;
  ObjectGuid.CreateNewUuid();
  CmpGuid.CreateNewUuid();

  ezAddObjectCommand cmd;
  cmd.SetType("ezGameObject");
  cmd.m_NewObjectGuid = ObjectGuid;
  cmd.m_bEditorProperty = false;
  cmd.m_Index = -1;
  cmd.m_sParentProperty = "RootObjects";

  auto history = m_pDocumentWindow->GetDocument()->GetCommandHistory();

  history->AddCommand(cmd);

  ezSetObjectPropertyCommand cmd2;
  cmd2.m_bEditorProperty = false;
  cmd2.m_Object = ObjectGuid;

  cmd2.SetPropertyPath("LocalPosition");
  cmd2.m_NewValue = vPosition;
  history->AddCommand(cmd2);

  cmd2.SetPropertyPath("LocalScaling");
  cmd2.m_NewValue = ezVec3(1.0f);
  history->AddCommand(cmd2);

  cmd.SetType(szType);
  cmd.m_sParentProperty = "Components";
  cmd.m_Index = -1;
  cmd.m_NewObjectGuid = CmpGuid;
  cmd.m_Parent = ObjectGuid;
  history->AddCommand(cmd);

  cmd2.m_Object = CmpGuid;
  cmd2.SetPropertyPath(szProperty);
  cmd2.m_NewValue = szValue;
  history->AddCommand(cmd2);

  return ObjectGuid;
}

void ezScene3DWidget::MoveObjectToPosition(const ezUuid& guid, const ezVec3& vPosition)
{
  auto history = m_pDocumentWindow->GetDocument()->GetCommandHistory();

  ezSetObjectPropertyCommand cmd2;
  cmd2.m_bEditorProperty = false;
  cmd2.m_Object = guid;

  cmd2.SetPropertyPath("LocalPosition");
  cmd2.m_NewValue = vPosition;
  history->AddCommand(cmd2);

}

void ezScene3DWidget::MoveDraggedObjectsToPosition(const ezVec3 & vPosition)
{
  if (m_DraggedObjects.IsEmpty() || vPosition.IsNaN())
    return;

  auto history = m_pDocumentWindow->GetDocument()->GetCommandHistory();

  history->StartTransaction();

  for (const auto& guid : m_DraggedObjects)
  {
    MoveObjectToPosition(guid, vPosition);
  }

  history->FinishTransaction();
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

