#include <PCH.h>
#include <EditorFramework/EditorApp/EditorApp.moc.h>
#include <EditorPluginScene/Scene/SceneViewWidget.moc.h>
#include <EditorPluginScene/Scene/SceneDocumentWindow.moc.h>
#include <GuiFoundation/ActionViews/ToolBarActionMapView.moc.h>
#include <ToolsFoundation/Command/TreeCommands.h>

#include <QKeyEvent>
#include <QMimeData>
#include <QVBoxLayout>

ezSceneViewWidget::ezSceneViewWidget(QWidget* pParent, ezSceneDocumentWindow* pOwnerWindow, ezCameraMoveContextSettings* pCameraMoveSettings, ezSceneViewConfig* pViewConfig)
  : ezEngineViewWidget(pParent, pOwnerWindow, pViewConfig)
{
  setAcceptDrops(true);

  ezSceneDocumentWindow* pSceneWindow = pOwnerWindow;

  m_pSelectionContext = EZ_DEFAULT_NEW(ezSelectionContext, pOwnerWindow, this, &m_pViewConfig->m_Camera);
  m_pCameraMoveContext = EZ_DEFAULT_NEW(ezCameraMoveContext, pOwnerWindow, this, pCameraMoveSettings);
  m_pCameraPositionContext = EZ_DEFAULT_NEW(ezCameraPositionContext, pOwnerWindow, this);

  m_pCameraMoveContext->LoadState();
  m_pCameraMoveContext->SetCamera(&m_pViewConfig->m_Camera);

  m_pCameraPositionContext->SetCamera(&m_pViewConfig->m_Camera);

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
  // can only drag & drop objects around in perspective mode
  if (m_pViewConfig->m_Perspective != ezSceneViewPerspective::Perspective)
    return;

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

      m_DraggedObjects.PushBack(CreateDropObject(res.m_vPickedPosition, "ezMeshComponent", "Mesh", sTemp));
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
  // can only drag & drop objects around in perspective mode
  if (m_pViewConfig->m_Perspective != ezSceneViewPerspective::Perspective)
    return;

  if (!m_DraggedObjects.IsEmpty())
  {
    m_pDocumentWindow->GetDocument()->GetCommandHistory()->CancelTemporaryCommands();
    m_pDocumentWindow->GetDocument()->GetCommandHistory()->Undo();
    m_DraggedObjects.Clear();
  }
}

void ezSceneViewWidget::dragMoveEvent(QDragMoveEvent* e)
{
  // can only drag & drop objects around in perspective mode
  if (m_pViewConfig->m_Perspective != ezSceneViewPerspective::Perspective)
    return;

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
  // can only drag & drop objects around in perspective mode
  if (m_pViewConfig->m_Perspective != ezSceneViewPerspective::Perspective)
    return;

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


ezSceneViewWidgetContainer::ezSceneViewWidgetContainer(QWidget* pParent, ezSceneDocumentWindow* pDocument, ezCameraMoveContextSettings* pCameraMoveSettings, ezSceneViewConfig* pViewConfig)
{
  setBackgroundRole(QPalette::Base);
  setAutoFillBackground(true);

  QVBoxLayout* pLayout = new QVBoxLayout(this);
  pLayout->setMargin(1);
  pLayout->setSpacing(0);
  setLayout(pLayout);

  m_pViewWidget = new ezSceneViewWidget(this, pDocument, pCameraMoveSettings, pViewConfig);
  
  {
    // Tool Bar
    ezToolBarActionMapView* pToolBar = new ezToolBarActionMapView(this);
    ezActionContext context;
    context.m_sMapping = "EditorPluginScene_ViewToolBar";
    context.m_pDocument = pDocument->GetDocument();
    context.m_pWindow = m_pViewWidget;
    pToolBar->SetActionContext(context);
    //pToolBar->setObjectName("SceneDocumentWindow_ToolBar");
    pLayout->addWidget(pToolBar);
  }

  pLayout->addWidget(m_pViewWidget);
}

ezSceneViewWidgetContainer::~ezSceneViewWidgetContainer()
{

}

