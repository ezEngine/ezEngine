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
  SetTargetFramerate(24);

  m_DelegatePropertyEvents = ezMakeDelegate(&ezSceneDocumentWindow::PropertyEventHandler, this);
  m_DelegateDocumentTreeEvents = ezMakeDelegate(&ezSceneDocumentWindow::DocumentTreeEventHandler, this);

  GetDocument()->GetObjectManager()->m_StructureEvents.AddEventHandler(m_DelegateDocumentTreeEvents);
  GetDocument()->GetObjectManager()->m_PropertyEvents.AddEventHandler(m_DelegatePropertyEvents);

  m_Camera.SetCameraMode(ezCamera::CameraMode::PerspectiveFixedFovY, 80.0f, 0.1f, 1000.0f);
  m_Camera.LookAt(ezVec3(0.5f, 1.5f, 2.0f), ezVec3(0.0f, 0.5f, 0.0f), ezVec3(0.0f, 1.0f, 0.0f));

  m_pSelectionContext = EZ_DEFAULT_NEW(ezSelectionContext, pDocument, this, &m_Camera);
  m_pMoveContext = EZ_DEFAULT_NEW(ezCameraMoveContext, m_pCenterWidget, pDocument, this);
  m_pCameraPositionContext = EZ_DEFAULT_NEW(ezCameraPositionContext, m_pCenterWidget, pDocument, this);

  m_pMoveContext->LoadState();
  m_pMoveContext->SetCamera(&m_Camera);

  m_pCameraPositionContext->SetCamera(&m_Camera);

  // add the input contexts in the order in which they are supposed to be processed
  m_pCenterWidget->m_InputContexts.PushBack(m_pSelectionContext);
  m_pCenterWidget->m_InputContexts.PushBack(m_pMoveContext);


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

  m_TranslateGizmo.m_BaseEvents.AddEventHandler(ezMakeDelegate(&ezSceneDocumentWindow::TransformationGizmoEventHandler, this));
  m_RotateGizmo.m_BaseEvents.AddEventHandler(ezMakeDelegate(&ezSceneDocumentWindow::TransformationGizmoEventHandler, this));
  m_ScaleGizmo.m_BaseEvents.AddEventHandler(ezMakeDelegate(&ezSceneDocumentWindow::TransformationGizmoEventHandler, this));
  m_DragToPosGizmo.m_BaseEvents.AddEventHandler(ezMakeDelegate(&ezSceneDocumentWindow::TransformationGizmoEventHandler, this));
  pSceneDoc->GetObjectManager()->m_PropertyEvents.AddEventHandler(ezMakeDelegate(&ezSceneDocumentWindow::ObjectPropertyEventHandler, this));



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
  pSceneDoc->GetObjectManager()->m_PropertyEvents.RemoveEventHandler(ezMakeDelegate(&ezSceneDocumentWindow::ObjectPropertyEventHandler, this));

  GetDocument()->GetSelectionManager()->m_Events.RemoveEventHandler(ezMakeDelegate(&ezSceneDocumentWindow::SelectionManagerEventHandler, this));

  GetDocument()->GetObjectManager()->m_PropertyEvents.RemoveEventHandler(m_DelegatePropertyEvents);
  GetDocument()->GetObjectManager()->m_StructureEvents.RemoveEventHandler(m_DelegateDocumentTreeEvents);

  EZ_DEFAULT_DELETE(m_pSelectionContext);
  EZ_DEFAULT_DELETE(m_pMoveContext);
  EZ_DEFAULT_DELETE(m_pCameraPositionContext);
}

void ezSceneDocumentWindow::PropertyEventHandler(const ezDocumentObjectPropertyEvent& e)
{
  m_pEngineView->SendObjectProperties(e);
}

void ezSceneDocumentWindow::ObjectPropertyEventHandler(const ezDocumentObjectPropertyEvent& e)
{
  if (m_bInGizmoInteraction)
    return;

  if (!m_TranslateGizmo.IsVisible() && !m_RotateGizmo.IsVisible() && !m_ScaleGizmo.IsVisible() && !m_DragToPosGizmo.IsVisible())
    return;

  if (e.m_bEditorProperty)
    return;

  if (GetDocument()->GetSelectionManager()->GetSelection().IsEmpty())
    return;

  if (e.m_pObject != GetDocument()->GetSelectionManager()->GetSelection()[0])
    return;

  if (e.m_sPropertyPath == "GlobalPosition" ||
      e.m_sPropertyPath == "GlobalRotation" ||
      e.m_sPropertyPath == "GlobalScaling")
  {
    /// \todo Make sure this isn't called three times for every update...
    UpdateGizmoPosition();
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
      if (!sel[0]->GetTypeAccessor().GetType()->IsDerivedFrom<ezGameObject>())
        return;

      const ezVec3 vPosition = sel[0]->GetTypeAccessor().GetValue("GlobalPosition").ConvertTo<ezVec3>();

      ezVec3 vDiff = vPosition - m_Camera.GetCenterPosition();
      if (vDiff.NormalizeIfNotZero().Failed())
        return;

      /// \todo The distance value of 10 is a hack
      m_pCameraPositionContext->MoveToTarget(vPosition - vDiff * 10.0f, vDiff);
    }
    break;
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

  m_pEngineView->SendMessage(&msg);
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
      m_GizmoSelection.IsEmpty();
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
    e->acceptProposedAction();
}

void ezScene3DWidget::dropEvent(QDropEvent * e)
{
  if (e->mimeData()->hasFormat("application/ezEditor.AssetGuid"))
  {
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

      const ezObjectPickingResult& res = m_pDocumentWindow->PickObject(e->pos().x(), e->pos().y());

      if (res.m_vPickedPosition.IsNaN())
        return;

      ezAddObjectCommand cmd;
      cmd.SetType("ezGameObject");

      auto history = m_pDocumentWindow->GetDocument()->GetCommandHistory();

      history->StartTransaction();

      history->AddCommand(cmd);

      /// \todo Cannot set additional properties, add components, etc. because I do not have a guid for the newly added object

      //ezSetObjectPropertyCommand cmd2;
      //cmd2.m_bEditorProperty = false;
      //cmd2.SetPropertyPath("GlobalPosition");
      //cmd2.m_NewValue = res.m_vPickedPosition;
      //cmd2.m_Object = cmd.GetObject()->GetGuid();

      //history->AddCommand(cmd2);

      history->EndTransaction(false);
    }
  }

}

