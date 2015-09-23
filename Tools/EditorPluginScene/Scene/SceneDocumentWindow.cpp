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
#include <GuiFoundation/PropertyGrid/PropertyGridWidget.moc.h>
#include <EditorFramework/EngineProcess/EngineProcessMessages.h>
#include <QTimer>
#include <QPushButton>
#include <qlayout.h>
#include <QKeyEvent>
#include <QMimeData>

ezSceneDocumentWindow::ezSceneDocumentWindow(ezDocumentBase* pDocument)
  : ezDocumentWindow3D(pDocument)
{
  m_ViewWidgets.PushBack(new ezSceneViewWidget(this, this, &m_CameraMoveSettings));
  m_ViewWidgets.PushBack(new ezSceneViewWidget(this, this, &m_CameraMoveSettings));

  m_ViewWidgets[1]->m_ViewRenderMode = ezViewRenderMode::WireframeMonochrome;

  QWidget* pCenter = new QWidget(this);
  pCenter->setLayout(new QHBoxLayout(pCenter));
  pCenter->layout()->addWidget(m_ViewWidgets[0]);
  pCenter->layout()->addWidget(m_ViewWidgets[1]);

  setCentralWidget(pCenter);

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

      ezQuerySelectionBBoxMsgToEngine msg;
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

  for (auto pView : m_ViewWidgets)
  {
    pView->SyncToEngine();

    // TODO: Batch render views

    ezViewRedrawMsgToEngine msg;
    msg.m_uiViewID = pView->GetViewID();
    msg.m_uiHWND = (ezUInt64)(pView->winId());
    msg.m_uiWindowWidth = pView->width();
    msg.m_uiWindowHeight = pView->height();

    m_pEngineConnection->SendMessage(&msg, true);

    ezEditorEngineProcessConnection::GetInstance()->WaitForMessage(ezGetStaticRTTI<ezViewRedrawFinishedMsgToEditor>(), ezTime::Seconds(1.0));
  }
}

bool ezSceneDocumentWindow::HandleEngineMessage(const ezEditorEngineDocumentMsg* pMsg)
{
  if (pMsg->GetDynamicRTTI()->IsDerivedFrom<ezQuerySelectionBBoxResultMsgToEditor>())
  {
    const ezQuerySelectionBBoxResultMsgToEditor* msg = static_cast<const ezQuerySelectionBBoxResultMsgToEditor*>(pMsg);

    const auto& LatestSelection = GetDocument()->GetSelectionManager()->GetSelection().PeekBack();
    const ezTransform tGlobal = GetSceneDocument()->GetGlobalTransform(LatestSelection);

    /// \todo Pivot point
    //const ezVec3 vPivotPoint = msg->m_vCenter;
    const ezVec3 vPivotPoint = tGlobal.m_vPosition + tGlobal.m_Rotation * ezVec3::ZeroVector(); // LatestSelection->GetEditorTypeAccessor().GetValue("Pivot").ConvertTo<ezVec3>();

    // TODO: focus all or one window?
    //ezSceneViewWidget* pFocusedView = GetFocusedViewWidget();
    //if (pFocusedView != nullptr)
    for (auto pView : m_ViewWidgets)
    {
      ezSceneViewWidget* pSceneView = static_cast<ezSceneViewWidget*>(pView);

      ezVec3 vDiff = vPivotPoint - pView->m_Camera.GetCenterPosition();
      if (vDiff.NormalizeIfNotZero().Failed())
        continue;

      /// \todo The distance value of 5 is a hack, we need the bounding box of the selection for this
      const ezVec3 vTargetPos = vPivotPoint - vDiff * 5.0f;

      pSceneView->m_pCameraMoveContext->SetOrbitPoint(vPivotPoint);
      pSceneView->m_pCameraPositionContext->MoveToTarget(vTargetPos, vDiff);
    }

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


ezSceneViewWidget::ezSceneViewWidget(QWidget* pParent, ezDocumentWindow3D* pOwnerWindow, ezCameraMoveContextSettings* pCameraMoveSettings) : ezEngineViewWidget(pParent, pOwnerWindow)
{
  setAcceptDrops(true);

  ezSceneDocumentWindow* pSceneWindow = static_cast<ezSceneDocumentWindow*>(pOwnerWindow);

  m_Camera.SetCameraMode(ezCamera::CameraMode::PerspectiveFixedFovY, 80.0f, 0.1f, 1000.0f);
  m_Camera.LookAt(ezVec3(0.5f, 2.0f, 1.5f), ezVec3(0.0f, 0.0f, 0.5f), ezVec3(0.0f, 0.0f, 1.0f));

  m_pSelectionContext = EZ_DEFAULT_NEW(ezSelectionContext, pOwnerWindow, this, &m_Camera);
  m_pCameraMoveContext = EZ_DEFAULT_NEW(ezCameraMoveContext, pOwnerWindow, this, pCameraMoveSettings);
  m_pCameraPositionContext = EZ_DEFAULT_NEW(ezCameraPositionContext, pOwnerWindow, this);

  m_pCameraMoveContext->LoadState();
  m_pCameraMoveContext->SetCamera(&m_Camera);

  m_pCameraPositionContext->SetCamera(&m_Camera);

  // add the input contexts in the order in which they are supposed to be processed
  m_InputContexts.PushBack(m_pSelectionContext);
  m_InputContexts.PushBack(m_pCameraMoveContext);
}

ezSceneViewWidget::~ezSceneViewWidget()
{
  EZ_DEFAULT_DELETE(m_pSelectionContext);
  EZ_DEFAULT_DELETE(m_pCameraMoveContext);
  EZ_DEFAULT_DELETE(m_pCameraPositionContext);
}

void ezSceneViewWidget::SyncToEngine()
{
  m_pSelectionContext->SetWindowConfig(ezVec2I32(width(), height()));

  ezEngineViewWidget::SyncToEngine();
}

void ezSceneViewWidget::dragEnterEvent(QDragEnterEvent* e)
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

void ezSceneViewWidget::dragLeaveEvent(QDragLeaveEvent * e)
{
  if (!m_DraggedObjects.IsEmpty())
  {
    m_pDocumentWindow->GetDocument()->GetCommandHistory()->CancelTemporaryCommands();
    m_pDocumentWindow->GetDocument()->GetCommandHistory()->Undo();
    m_DraggedObjects.Clear();
  }
}

void ezSceneViewWidget::dragMoveEvent(QDragMoveEvent* e)
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

void ezSceneViewWidget::dropEvent(QDropEvent * e)
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

ezUuid ezSceneViewWidget::CreateDropObject(const ezVec3& vPosition, const char* szType, const char* szProperty, const char* szValue)
{
  ezUuid ObjectGuid, CmpGuid;
  ObjectGuid.CreateNewUuid();
  CmpGuid.CreateNewUuid();

  ezAddObjectCommand cmd;
  cmd.SetType("ezGameObject");
  cmd.m_NewObjectGuid = ObjectGuid;
  cmd.m_Index = -1;
  cmd.m_sParentProperty = "RootObjects";

  auto history = m_pDocumentWindow->GetDocument()->GetCommandHistory();

  history->AddCommand(cmd);

  ezSetObjectPropertyCommand cmd2;
  cmd2.m_Object = ObjectGuid;

  cmd2.SetPropertyPath("LocalPosition");
  cmd2.m_NewValue = vPosition;
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

void ezSceneViewWidget::MoveObjectToPosition(const ezUuid& guid, const ezVec3& vPosition)
{
  auto history = m_pDocumentWindow->GetDocument()->GetCommandHistory();

  ezSetObjectPropertyCommand cmd2;
  cmd2.m_Object = guid;

  cmd2.SetPropertyPath("LocalPosition");
  cmd2.m_NewValue = vPosition;
  history->AddCommand(cmd2);

}

void ezSceneViewWidget::MoveDraggedObjectsToPosition(const ezVec3 & vPosition)
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

