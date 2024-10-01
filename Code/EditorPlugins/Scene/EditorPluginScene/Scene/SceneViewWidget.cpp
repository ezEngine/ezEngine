#include <EditorPluginScene/EditorPluginScenePCH.h>

#include <EditorFramework/Actions/GameObjectSelectionActions.h>
#include <EditorFramework/DocumentWindow/GameObjectDocumentWindow.moc.h>
#include <EditorFramework/DragDrop/DragDropHandler.h>
#include <EditorFramework/DragDrop/DragDropInfo.h>
#include <EditorPluginScene/Actions/SceneActions.h>
#include <EditorPluginScene/Actions/SelectionActions.h>
#include <EditorPluginScene/InputContexts/SceneSelectionContext.h>
#include <EditorPluginScene/Scene/Scene2Document.h>
#include <EditorPluginScene/Scene/SceneViewWidget.moc.h>
#include <GuiFoundation/Action/ActionMapManager.h>
#include <GuiFoundation/Action/EditActions.h>
#include <GuiFoundation/ActionViews/MenuActionMapView.moc.h>
#include <QKeyEvent>

bool ezQtSceneViewWidget::s_bContextMenuInitialized = false;

ezQtSceneViewWidget::ezQtSceneViewWidget(QWidget* pParent, ezQtGameObjectDocumentWindow* pOwnerWindow, ezEngineViewConfig* pViewConfig)
  : ezQtGameObjectViewWidget(pParent, pOwnerWindow, pViewConfig)
{
  setAcceptDrops(true);

  m_bAllowPickSelectedWhileDragging = false;

  if (ezDynamicCast<ezScene2Document*>(pOwnerWindow->GetDocument()))
  {
    // #TODO Not the cleanest solution but this replaces the default selection context of the base class.
    const ezUInt32 uiSelectionIndex = m_InputContexts.IndexOf(m_pSelectionContext);
    EZ_DEFAULT_DELETE(m_pSelectionContext);
    m_pSelectionContext = EZ_DEFAULT_NEW(ezSceneSelectionContext, pOwnerWindow, this, &m_pViewConfig->m_Camera);
    m_InputContexts[uiSelectionIndex] = m_pSelectionContext;
  }
}

ezQtSceneViewWidget::~ezQtSceneViewWidget() = default;

bool ezQtSceneViewWidget::IsPickingAgainstSelectionAllowed() const
{
  if (m_bInDragAndDropOperation && m_bAllowPickSelectedWhileDragging)
  {
    return true;
  }

  return ezQtEngineViewWidget::IsPickingAgainstSelectionAllowed();
}

void ezQtSceneViewWidget::OnOpenContextMenu(QPoint globalPos)
{
  if (!s_bContextMenuInitialized)
  {
    s_bContextMenuInitialized = true;

    ezActionMapManager::RegisterActionMap("SceneViewContextMenu");

    ezGameObjectSelectionActions::MapViewContextMenuActions("SceneViewContextMenu");
    ezSelectionActions::MapViewContextMenuActions("SceneViewContextMenu");
    ezEditActions::MapViewContextMenuActions("SceneViewContextMenu");
    ezSceneActions::MapViewContextMenuActions("SceneViewContextMenu");
  }

  {
    ezQtMenuActionMapView menu(nullptr);

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
  // if (m_pViewConfig->m_Perspective != ezSceneViewPerspective::Perspective)
  // return;

  m_LastDragMoveEvent = ezTime::Now();
  m_bAllowPickSelectedWhileDragging = false;

  {
    const QPoint screenPos = e->position().toPoint();
    ezObjectPickingResult res = PickObject(screenPos.x(), screenPos.y());

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
    info.m_bShiftKeyDown = e->modifiers() & Qt::ShiftModifier;
    info.m_bCtrlKeyDown = e->modifiers() & Qt::ControlModifier;

    if (ezSceneDocument* pSceneDoc = ezDynamicCast<ezSceneDocument*>(m_pDocumentWindow->GetDocument()))
    {
      const ezUuid guid = pSceneDoc->GetActiveParent();

      // the object may not exist anymore
      if (pSceneDoc->GetObjectManager()->GetObject(guid) != nullptr)
      {
        info.m_ActiveParentObject = guid;
      }
    }

    ezDragDropConfig cfg;
    if (ezDragDropHandler::BeginDragDropOperation(&info, &cfg))
    {
      m_bAllowPickSelectedWhileDragging = cfg.m_bPickSelectedObjects;

      e->acceptProposedAction();
      return;
    }
  }

  m_bInDragAndDropOperation = false;
}

void ezQtSceneViewWidget::dragLeaveEvent(QDragLeaveEvent* e)
{
  ezDragDropHandler::CancelDragDrop();

  ezQtEngineViewWidget::dragLeaveEvent(e);
}

void ezQtSceneViewWidget::dragMoveEvent(QDragMoveEvent* e)
{
  const ezTime tNow = ezTime::Now();

  if (tNow - m_LastDragMoveEvent < ezTime::MakeFromSeconds(1.0 / 25.0))
    return;

  m_LastDragMoveEvent = tNow;

  if (ezDragDropHandler::IsHandlerActive())
  {
    const QPoint screenPos = e->position().toPoint();
    ezObjectPickingResult res = PickObject(screenPos.x(), screenPos.y());

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
    info.m_bShiftKeyDown = e->modifiers() & Qt::ShiftModifier;
    info.m_bCtrlKeyDown = e->modifiers() & Qt::ControlModifier;

    ezDragDropHandler::UpdateDragDropOperation(&info);
  }
}

void ezQtSceneViewWidget::dropEvent(QDropEvent* e)
{
  if (ezDragDropHandler::IsHandlerActive())
  {
    const QPoint screenPos = e->position().toPoint();
    ezObjectPickingResult res = PickObject(screenPos.x(), screenPos.y());

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
    info.m_bShiftKeyDown = e->modifiers() & Qt::ShiftModifier;
    info.m_bCtrlKeyDown = e->modifiers() & Qt::ControlModifier;

    ezDragDropHandler::FinishDragDrop(&info);
  }

  ezQtEngineViewWidget::dropEvent(e);
}
