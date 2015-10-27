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

ezQtSceneViewWidget::ezQtSceneViewWidget(QWidget* pParent, ezQtSceneDocumentWindow* pOwnerWindow, ezCameraMoveContextSettings* pCameraMoveSettings, ezSceneViewConfig* pViewConfig)
  : ezQtEngineViewWidget(pParent, pOwnerWindow, pViewConfig)
{
  setAcceptDrops(true);

  ezQtSceneDocumentWindow* pSceneWindow = pOwnerWindow;

  m_pSelectionContext = EZ_DEFAULT_NEW(ezSelectionContext, pOwnerWindow, this, &m_pViewConfig->m_Camera);
  m_pCameraMoveContext = EZ_DEFAULT_NEW(ezCameraMoveContext, pOwnerWindow, this, pCameraMoveSettings);

  m_pCameraMoveContext->LoadState();
  m_pCameraMoveContext->SetCamera(&m_pViewConfig->m_Camera);

  // add the input contexts in the order in which they are supposed to be processed
  m_InputContexts.PushBack(m_pSelectionContext);
  m_InputContexts.PushBack(m_pCameraMoveContext);
}

ezQtSceneViewWidget::~ezQtSceneViewWidget()
{
  EZ_DEFAULT_DELETE(m_pSelectionContext);
  EZ_DEFAULT_DELETE(m_pCameraMoveContext);
}

void ezQtSceneViewWidget::SyncToEngine()
{
  m_pSelectionContext->SetWindowConfig(ezVec2I32(width(), height()));

  ezQtEngineViewWidget::SyncToEngine();
}

void ezQtSceneViewWidget::dragEnterEvent(QDragEnterEvent* e)
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

    if (res.m_vPickedPosition.IsNaN() || !res.m_PickedObject.IsValid())
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

      if (ezAssetCurator::GetInstance()->GetAssetInfo(AssetGuid)->m_Info.m_sAssetTypeName == "Prefab")
      {
        CreatePrefab(res.m_vPickedPosition, AssetGuid);
      }
      else
      if (ezAssetCurator::GetInstance()->GetAssetInfo(AssetGuid)->m_Info.m_sAssetTypeName == "Mesh")
      {
        CreateDropObject(res.m_vPickedPosition, "ezMeshComponent", "Mesh", sTemp);
      }
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

void ezQtSceneViewWidget::dragLeaveEvent(QDragLeaveEvent * e)
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

void ezQtSceneViewWidget::dragMoveEvent(QDragMoveEvent* e)
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

    if (res.m_vPickedPosition.IsNaN() || !res.m_PickedObject.IsValid())
      res.m_vPickedPosition.SetZero();

    MoveDraggedObjectsToPosition(res.m_vPickedPosition);
  }
}

void ezQtSceneViewWidget::dropEvent(QDropEvent * e)
{
  // can only drag & drop objects around in perspective mode
  if (m_pViewConfig->m_Perspective != ezSceneViewPerspective::Perspective)
    return;

  if (e->mimeData()->hasFormat("application/ezEditor.AssetGuid") && !m_DraggedObjects.IsEmpty())
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
  auto* pAssetInfo = ezAssetCurator::GetInstance()->GetAssetInfo(AssetGuid);

  const auto& sPrefabFile = pAssetInfo->m_sAbsolutePath;

  ezFileReader file;
  if (file.Open(sPrefabFile) == EZ_FAILURE)
  {
    ezLog::Error("Failed to open prefab file '%s'", sPrefabFile.GetData());
    return;
  }

  auto* pDocument = static_cast<ezSceneDocument*>(GetDocumentWindow()->GetDocument());
  auto pCmdHistory = pDocument->GetCommandHistory();

  ezAbstractObjectGraph graph;
  ezAbstractGraphJsonSerializer::Read(file, &graph);

  ezUuid RootObjectGuid;
  RootObjectGuid.CreateNewUuid();

  // create root object (workaround)
  ezDocumentObject* pRootObject = nullptr;
  {
    ezAddObjectCommand cmd;
    cmd.SetType("ezGameObject");
    cmd.m_NewObjectGuid = RootObjectGuid;
    cmd.m_Index = -1;
    cmd.m_sParentProperty = "Children";

    pCmdHistory->AddCommand(cmd);

    ezSetObjectPropertyCommand cmd2;
    cmd2.m_Object = RootObjectGuid;

    cmd2.SetPropertyPath("LocalPosition");
    cmd2.m_NewValue = vPosition;
    pCmdHistory->AddCommand(cmd2);

    pRootObject = pDocument->GetObjectManager()->GetObject(RootObjectGuid);
  }

  // make sure all guids are unique, use new parent node guid as seed
  graph.ReMapNodeGuids(RootObjectGuid);

  ezRttiConverterContext context;
  ezRttiConverterReader rttiConverter(&graph, &context);
  ezDocumentObjectConverterReader objectConverter(&graph, pDocument->GetObjectManager(), ezDocumentObjectConverterReader::Mode::CreateAndAddToDocumentUndoable);

  auto* pRootNode = graph.GetNodeByName("ObjectTree");


  objectConverter.ApplyPropertiesToObject(pRootNode, pRootObject); // workaround
  //objectConverter.ApplyPropertiesToObject(pRootNode, pDocument->GetObjectManager()->GetRootObject());


  // put the guids of all the new objects into the m_DraggedObjects array
  //for (const auto& RootProps : pRootNode->GetProperties())
  //{
  //  if (ezStringUtils::IsEqual(RootProps.m_szPropertyName, "Children") && RootProps.m_Value.IsA<ezVariantArray>())
  //  {
  //    const ezVariantArray& ChildProbs = RootProps.m_Value.Get<ezVariantArray>();

  //    ezInt32 iChildIndex = 0;
  //    for (const auto& child : ChildProbs)
  //    {
  //      if (child.IsA<ezUuid>())
  //      {
  //        m_DraggedObjects.PushBack(child.Get<ezUuid>());
  //      }
  //    }

  //    break;
  //  }
  //}

  // workaround
  m_DraggedObjects.PushBack(RootObjectGuid);


  /// \todo HACK: This is to get around the fact that ezDocumentObjectConverterReader::Mode::CreateAndAddToDocumentUndoable is not implemented and thus objects are not synched with the engine process
  /// This breaks undo/redo (engine is not informed of undo operations)
  GetDocumentWindow()->GetEditorEngineConnection()->SendDocument();
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

ezQtSceneViewWidgetContainer::~ezQtSceneViewWidgetContainer()
{

}

