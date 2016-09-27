#include <PCH.h>
#include <EditorFramework/EditorApp/EditorApp.moc.h>
#include <EditorPluginScene/Scene/SceneViewWidget.moc.h>
#include <EditorPluginScene/Scene/SceneDocumentWindow.moc.h>
#include <GuiFoundation/ActionViews/ToolBarActionMapView.moc.h>
#include <ToolsFoundation/Command/TreeCommands.h>

#include <QKeyEvent>
#include <QMimeData>
#include <QVBoxLayout>
#include <Foundation/IO/FileSystem/FileReader.h>
#include <Foundation/Serialization/JsonSerializer.h>
#include <Foundation/Serialization/RttiConverter.h>
#include <ToolsFoundation/Serialization/DocumentObjectConverter.h>
#include <InputContexts/OrthoGizmoContext.h>
#include <EditorFramework/Gizmos/SnapProvider.h>
#include <EditorFramework/DragDrop/DragDropInfo.h>
#include <EditorFramework/DragDrop/DragDropHandler.h>
#include <GuiFoundation/Action/ActionMapManager.h>
#include <EditorPluginScene/Actions/SelectionActions.h>
#include <GuiFoundation/Action/EditActions.h>
#include <GuiFoundation/ActionViews/MenuActionMapView.moc.h>

bool ezQtSceneViewWidget::s_bContextMenuInitialized = false;

ezQtSceneViewWidget::ezQtSceneViewWidget(QWidget* pParent, ezQtSceneDocumentWindow* pOwnerWindow, ezCameraMoveContextSettings* pCameraMoveSettings, ezSceneViewConfig* pViewConfig)
  : ezQtEngineViewWidget(pParent, pOwnerWindow, pViewConfig)
{
  setAcceptDrops(true);

  m_bAllowPickSelectedWhileDragging = false;

  ezQtSceneDocumentWindow* pSceneWindow = pOwnerWindow;

  m_pSelectionContext = EZ_DEFAULT_NEW(ezSelectionContext, pOwnerWindow, this, &m_pViewConfig->m_Camera);
  m_pCameraMoveContext = EZ_DEFAULT_NEW(ezCameraMoveContext, pOwnerWindow, this, pCameraMoveSettings);
  m_pOrthoGizmoContext = EZ_DEFAULT_NEW(ezOrthoGizmoContext, pOwnerWindow, this, &m_pViewConfig->m_Camera);

  m_pCameraMoveContext->SetCamera(&m_pViewConfig->m_Camera);
  m_pCameraMoveContext->LoadState();

  // add the input contexts in the order in which they are supposed to be processed
  m_InputContexts.PushBack(m_pOrthoGizmoContext);
  m_InputContexts.PushBack(m_pSelectionContext);
  m_InputContexts.PushBack(m_pCameraMoveContext);
}

ezQtSceneViewWidget::~ezQtSceneViewWidget()
{
  EZ_DEFAULT_DELETE(m_pOrthoGizmoContext);
  EZ_DEFAULT_DELETE(m_pSelectionContext);
  EZ_DEFAULT_DELETE(m_pCameraMoveContext);
}

void ezQtSceneViewWidget::SyncToEngine()
{
  m_pSelectionContext->SetWindowConfig(ezVec2I32(width(), height()));

  ezQtEngineViewWidget::SyncToEngine();
}

bool ezQtSceneViewWidget::IsPickingAgainstSelectionAllowed() const
{
  if (m_bInDragAndDropOperation && m_bAllowPickSelectedWhileDragging)
  {
    return true;
  }

  return ezQtEngineViewWidget::IsPickingAgainstSelectionAllowed();
}

void ezQtSceneViewWidget::OpenContextMenu(QPoint globalPos)
{
  if (!s_bContextMenuInitialized)
  {
    s_bContextMenuInitialized = true;

    ezActionMapManager::RegisterActionMap("SceneViewContextMenu");

    ezSelectionActions::MapViewContextMenuActions("SceneViewContextMenu", "");
    ezEditActions::MapViewContextMenuActions("SceneViewContextMenu", "");
  }

  {
    ezMenuActionMapView menu(nullptr);

    ezActionContext context;
    context.m_sMapping = "SceneViewContextMenu";
    context.m_pDocument = GetDocumentWindow()->GetDocument();
    context.m_pWindow = this;
    menu.SetActionContext(context);

    menu.exec(globalPos);
  }
}

void ezQtSceneViewWidget::dragEnterEvent(QDragEnterEvent* e)
{
  ezQtEngineViewWidget::dragEnterEvent(e);

  // can only drag & drop objects around in perspective mode
  // when dragging between two windows, the editor crashes
  // can be reproduced with two perspective windows as well
  //if (m_pViewConfig->m_Perspective != ezSceneViewPerspective::Perspective)
    //return;

  m_LastDragMoveEvent = ezTime::Now();
  m_bAllowPickSelectedWhileDragging = false;

  {
    ezObjectPickingResult res = m_pDocumentWindow->PickObject(e->pos().x(), e->pos().y());

    ezDragDropInfo info;
    info.m_pMimeData = e->mimeData();
    info.m_TargetDocument = GetDocumentWindow()->GetDocument()->GetGuid();
    info.m_sTargetContext = "viewport";
    info.m_iTargetObjectInsertChildIndex = -1;
    info.m_vDropPosition = res.m_vPickedPosition;
    info.m_vDropNormal = res.m_vPickedNormal;
    info.m_iTargetObjectSubID = res.m_uiPartIndex;
    info.m_TargetObject = res.m_PickedObject;
    info.m_TargetComponent = res.m_PickedComponent;
    info.m_bShiftKeyDown = e->keyboardModifiers() & Qt::ShiftModifier;
    info.m_bCtrlKeyDown = e->keyboardModifiers() & Qt::ControlModifier;

    ezDragDropConfig cfg;
    if (ezDragDropHandler::BeginDragDropOperation(&info, &cfg))
    {
      m_bAllowPickSelectedWhileDragging = cfg.m_bPickSelectedObjects;

      e->acceptProposedAction();
      return;
    }
  }
  }

void ezQtSceneViewWidget::dragLeaveEvent(QDragLeaveEvent * e)
{
  ezDragDropHandler::CancelDragDrop();

  ezQtEngineViewWidget::dragLeaveEvent(e);
}

void ezQtSceneViewWidget::dragMoveEvent(QDragMoveEvent* e)
{
  const ezTime tNow = ezTime::Now();

  if (tNow - m_LastDragMoveEvent < ezTime::Seconds(1.0 / 25.0))
    return;

  m_LastDragMoveEvent = tNow;

  if (ezDragDropHandler::IsHandlerActive())
  {
    ezObjectPickingResult res = m_pDocumentWindow->PickObject(e->pos().x(), e->pos().y());

    ezDragDropInfo info;
    info.m_pMimeData = e->mimeData();
    info.m_TargetDocument = GetDocumentWindow()->GetDocument()->GetGuid();
    info.m_sTargetContext = "viewport";
    info.m_iTargetObjectInsertChildIndex = -1;
    info.m_vDropPosition = res.m_vPickedPosition;
    info.m_vDropNormal = res.m_vPickedNormal;
    info.m_iTargetObjectSubID = res.m_uiPartIndex;
    info.m_TargetObject = res.m_PickedObject;
    info.m_TargetComponent = res.m_PickedComponent;
    info.m_bShiftKeyDown = e->keyboardModifiers() & Qt::ShiftModifier;
    info.m_bCtrlKeyDown = e->keyboardModifiers() & Qt::ControlModifier;

    ezDragDropHandler::UpdateDragDropOperation(&info);
  }
}

void ezQtSceneViewWidget::dropEvent(QDropEvent * e)
{
  if (ezDragDropHandler::IsHandlerActive())
  {
    ezObjectPickingResult res = m_pDocumentWindow->PickObject(e->pos().x(), e->pos().y());

    ezDragDropInfo info;
    info.m_pMimeData = e->mimeData();
    info.m_TargetDocument = GetDocumentWindow()->GetDocument()->GetGuid();
    info.m_sTargetContext = "viewport";
    info.m_iTargetObjectInsertChildIndex = -1;
    info.m_vDropPosition = res.m_vPickedPosition;
    info.m_vDropNormal = res.m_vPickedNormal;
    info.m_iTargetObjectSubID = res.m_uiPartIndex;
    info.m_TargetObject = res.m_PickedObject;
    info.m_TargetComponent = res.m_PickedComponent;
    info.m_bShiftKeyDown = e->keyboardModifiers() & Qt::ShiftModifier;
    info.m_bCtrlKeyDown = e->keyboardModifiers() & Qt::ControlModifier;

    ezDragDropHandler::FinishDragDrop(&info);
  }

  ezQtEngineViewWidget::dropEvent(e);
}



