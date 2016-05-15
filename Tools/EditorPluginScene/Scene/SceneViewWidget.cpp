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

ezQtSceneViewWidget::ezQtSceneViewWidget(QWidget* pParent, ezQtSceneDocumentWindow* pOwnerWindow, ezCameraMoveContextSettings* pCameraMoveSettings, ezSceneViewConfig* pViewConfig)
  : ezQtEngineViewWidget(pParent, pOwnerWindow, pViewConfig)
{
  setAcceptDrops(true);

  m_bAllowPickSelectedWhileDragging = false;

  ezQtSceneDocumentWindow* pSceneWindow = pOwnerWindow;

  m_pSelectionContext = EZ_DEFAULT_NEW(ezSelectionContext, pOwnerWindow, this, &m_pViewConfig->m_Camera);
  m_pCameraMoveContext = EZ_DEFAULT_NEW(ezCameraMoveContext, pOwnerWindow, this, pCameraMoveSettings);
  m_pOrthoGizmoContext = EZ_DEFAULT_NEW(ezOrthoGizmoContext, pOwnerWindow, this, &m_pViewConfig->m_Camera);

  m_pCameraMoveContext->LoadState();
  m_pCameraMoveContext->SetCamera(&m_pViewConfig->m_Camera);

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

void ezQtSceneViewWidget::dragEnterEvent(QDragEnterEvent* e)
{
  ezQtEngineViewWidget::dragEnterEvent(e);

  // can only drag & drop objects around in perspective mode
  if (m_pViewConfig->m_Perspective != ezSceneViewPerspective::Perspective)
    return;

  m_bAllowPickSelectedWhileDragging = false;

  if (e->mimeData()->hasFormat("application/ezEditor.AssetGuid"))
  {
    m_LastDragMoveEvent = ezTime::Now();

    m_DraggedObjects.Clear();
    e->acceptProposedAction();

    ezObjectPickingResult res = m_pDocumentWindow->PickObject(e->pos().x(), e->pos().y());

    if (res.m_vPickedPosition.IsNaN() || !res.m_PickedObject.IsValid())
      res.m_vPickedPosition.SetZero();

    QByteArray ba = e->mimeData()->data("application/ezEditor.AssetGuid");
    QDataStream stream(&ba, QIODevice::ReadOnly);

    int iGuids = 0;
    stream >> iGuids;

    ezStringBuilder sTemp;

    m_pDocumentWindow->GetDocument()->GetCommandHistory()->StartTransaction();

    for (ezInt32 i = 0; i < iGuids; ++i)
    {
      QString sGuid;
      stream >> sGuid;

      sTemp = sGuid.toUtf8().data();
      ezUuid AssetGuid = ezConversionUtils::ConvertStringToUuid(sTemp);

      if (ezAssetCurator::GetSingleton()->GetAssetInfo(AssetGuid)->m_Info.m_sAssetTypeName == "Material")
      {
        m_bAllowPickSelectedWhileDragging = true;

        m_pDocumentWindow->GetDocument()->GetCommandHistory()->CancelTransaction();
        m_sDragMaterial = sTemp;
        return;
      }
      else if (ezAssetCurator::GetSingleton()->GetAssetInfo(AssetGuid)->m_Info.m_sAssetTypeName == "Prefab")
      {
        CreatePrefab(res.m_vPickedPosition, AssetGuid);
      }
      else if (ezAssetCurator::GetSingleton()->GetAssetInfo(AssetGuid)->m_Info.m_sAssetTypeName == "Mesh")
      {
        CreateDropObject(res.m_vPickedPosition, "ezMeshComponent", "Mesh", sTemp);
      }
    }

    if (m_DraggedObjects.IsEmpty())
      m_pDocumentWindow->GetDocument()->GetCommandHistory()->CancelTransaction();
    else
    {
      /// \todo We would need nested transactions that can be entirely canceled in dragLeaveEvent. The way it is currently implemented, we are destroying the Redo-history here.

      auto pDoc = GetDocumentWindow()->GetDocument();

      ezDeque<const ezDocumentObject*> NewSel;
      for (const auto& id : m_DraggedObjects)
      {
        NewSel.PushBack(pDoc->GetObjectManager()->GetObject(id));
      }

      pDoc->GetSelectionManager()->SetSelection(NewSel);

      pDoc->GetCommandHistory()->FinishTransaction();
      pDoc->GetCommandHistory()->BeginTemporaryCommands();
    }
  }
}

void ezQtSceneViewWidget::dragLeaveEvent(QDragLeaveEvent * e)
{
  ezQtEngineViewWidget::dragLeaveEvent(e);

  m_sDragMaterial.Clear();

  // can only drag & drop objects around in perspective mode
  if (m_pViewConfig->m_Perspective != ezSceneViewPerspective::Perspective)
    return;

  if (!m_DraggedObjects.IsEmpty())
  {
    m_pDocumentWindow->GetDocument()->GetSelectionManager()->Clear();

    m_pDocumentWindow->GetDocument()->GetCommandHistory()->CancelTemporaryCommands();
    m_pDocumentWindow->GetDocument()->GetCommandHistory()->Undo();
    m_DraggedObjects.Clear();
  }
}

void ezQtSceneViewWidget::dragMoveEvent(QDragMoveEvent* e)
{
  // can only drag & drop objects around in perspective mode
  if (m_pViewConfig->m_Perspective != ezSceneViewPerspective::Perspective)
    return;

  const ezTime tNow = ezTime::Now();

  if (tNow - m_LastDragMoveEvent < ezTime::Seconds(1.0 / 25.0))
    return;

  m_LastDragMoveEvent = tNow;

  /// \todo workaround for bad picking behavior: update picking data every frame to get a decent result once we drop
  ezObjectPickingResult res = m_pDocumentWindow->PickObject(e->pos().x(), e->pos().y());

  if (!m_sDragMaterial.IsEmpty())
  {
    return;
  }

  if (e->mimeData()->hasFormat("application/ezEditor.AssetGuid") && !m_DraggedObjects.IsEmpty())
  {
    if (res.m_vPickedPosition.IsNaN() || !res.m_PickedObject.IsValid())
      res.m_vPickedPosition.SetZero();

    MoveDraggedObjectsToPosition(res.m_vPickedPosition);
  }
}

void ezQtSceneViewWidget::dropEvent(QDropEvent * e)
{
  ezQtEngineViewWidget::dropEvent(e);

  // can only drag & drop objects around in perspective mode
  if (m_pViewConfig->m_Perspective != ezSceneViewPerspective::Perspective)
    return;

  if (!e->mimeData()->hasFormat("application/ezEditor.AssetGuid"))
    return;

  if (!m_sDragMaterial.IsEmpty())
  {
    ezStringBuilder sMaterial = m_sDragMaterial;
    m_sDragMaterial.Clear();

    ezObjectPickingResult res = m_pDocumentWindow->PickObject(e->pos().x(), e->pos().y());

    if (!res.m_PickedComponent.IsValid())
      return;

    const ezDocumentObject* pComponent = GetDocumentWindow()->GetDocument()->GetObjectManager()->GetObject(res.m_PickedComponent);

    if (!pComponent || pComponent->GetTypeAccessor().GetType() != ezRTTI::FindTypeByName("ezMeshComponent"))
      return;

    ezResizeAndSetObjectPropertyCommand cmd;
    cmd.m_Object = res.m_PickedComponent;
    cmd.m_Index = res.m_uiPartIndex;
    cmd.SetPropertyPath("Materials");
    cmd.m_NewValue = sMaterial.GetData();

    m_pDocumentWindow->GetDocument()->GetCommandHistory()->StartTransaction();
    m_pDocumentWindow->GetDocument()->GetCommandHistory()->AddCommand(cmd);
    m_pDocumentWindow->GetDocument()->GetCommandHistory()->FinishTransaction();
  }
  else if (!m_DraggedObjects.IsEmpty())
  {
    m_pDocumentWindow->GetDocument()->GetCommandHistory()->FinishTemporaryCommands();

    m_pDocumentWindow->GetDocument()->GetCommandHistory()->MergeLastTwoTransactions();

    ezDeque<const ezDocumentObject*> NewSelection;

    for (const auto& guid : m_DraggedObjects)
    {
      auto pObj = m_pDocumentWindow->GetDocument()->GetObjectManager()->GetObject(guid);

      if (pObj != nullptr)
        NewSelection.PushBack(pObj);
    }

    m_pDocumentWindow->GetDocument()->GetSelectionManager()->SetSelection(NewSelection);

    m_DraggedObjects.Clear();
  }
}

void ezQtSceneViewWidget::CreateDropObject(const ezVec3& vPosition, const char* szType, const char* szProperty, const char* szValue)
{
  ezUuid ObjectGuid, CmpGuid;
  ObjectGuid.CreateNewUuid();
  CmpGuid.CreateNewUuid();

  ezAddObjectCommand cmd;
  cmd.SetType("ezGameObject");
  cmd.m_NewObjectGuid = ObjectGuid;
  cmd.m_Index = -1;
  cmd.m_sParentProperty = "Children";

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

  m_DraggedObjects.PushBack(ObjectGuid);
}

void ezQtSceneViewWidget::CreatePrefab(const ezVec3& vPosition, const ezUuid& AssetGuid)
{
  auto* pDocument = static_cast<ezSceneDocument*>(GetDocumentWindow()->GetDocument());
  auto pCmdHistory = pDocument->GetCommandHistory();

  ezInstantiatePrefabCommand PasteCmd;
  PasteCmd.m_sJsonGraph = pDocument->GetCachedPrefabGraph(AssetGuid);
  PasteCmd.m_RemapGuid.CreateNewUuid();

  if (PasteCmd.m_sJsonGraph.IsEmpty())
    return; // error

  pCmdHistory->AddCommand(PasteCmd);

  if (PasteCmd.m_CreatedRootObject.IsValid())
  {
    auto pMeta = pDocument->m_DocumentObjectMetaData.BeginModifyMetaData(PasteCmd.m_CreatedRootObject);
    pMeta->m_CreateFromPrefab = AssetGuid;
    pMeta->m_PrefabSeedGuid = PasteCmd.m_RemapGuid;
    pMeta->m_sBasePrefab = PasteCmd.m_sJsonGraph;
    pDocument->m_DocumentObjectMetaData.EndModifyMetaData(ezDocumentObjectMetaData::PrefabFlag);

    MoveObjectToPosition(PasteCmd.m_CreatedRootObject, vPosition);

    m_DraggedObjects.PushBack(PasteCmd.m_CreatedRootObject);
  }
}



void ezQtSceneViewWidget::MoveObjectToPosition(const ezUuid& guid, const ezVec3& vPosition)
{
  auto history = m_pDocumentWindow->GetDocument()->GetCommandHistory();

  ezSetObjectPropertyCommand cmd2;
  cmd2.m_Object = guid;

  cmd2.SetPropertyPath("LocalPosition");
  cmd2.m_NewValue = vPosition;
  history->AddCommand(cmd2);

}

void ezQtSceneViewWidget::MoveDraggedObjectsToPosition(const ezVec3 & vPosition)
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


ezQtSceneViewWidgetContainer::ezQtSceneViewWidgetContainer(QWidget* pParent, ezQtSceneDocumentWindow* pDocument, ezCameraMoveContextSettings* pCameraMoveSettings, ezSceneViewConfig* pViewConfig)
{
  setBackgroundRole(QPalette::Base);
  setAutoFillBackground(true);

  QVBoxLayout* pLayout = new QVBoxLayout(this);
  pLayout->setMargin(1);
  pLayout->setSpacing(0);
  setLayout(pLayout);

  m_pViewWidget = new ezQtSceneViewWidget(this, pDocument, pCameraMoveSettings, pViewConfig);

  {
    // Tool Bar
    ezToolBarActionMapView* pToolBar = new ezToolBarActionMapView("Toolbar", this);
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

ezQtSceneViewWidgetContainer::~ezQtSceneViewWidgetContainer()
{

}

