#include <GuiFoundation/PCH.h>
#include <ToolsFoundation/Document/Document.h>
#include <ToolsFoundation/Selection/SelectionManager.h>
#include <ToolsFoundation/CommandHistory/CommandHistory.h>
#include <ToolsFoundation/Command/TreeCommands.h>
#include <ToolsFoundation/Command/NodeCommands.h>
#include <GuiFoundation/UIServices/UIServices.moc.h>
#include <GuiFoundation/NodeEditor/NodeScene.moc.h>
#include <GuiFoundation/NodeEditor/Node.h>
#include <GuiFoundation/NodeEditor/Pin.h>
#include <GuiFoundation/NodeEditor/Connection.h>
#include <GuiFoundation/NodeEditor/NodeScene.moc.h>
#include <GuiFoundation/Widgets/SearchableMenu.moc.h>
#include <QGraphicsSceneMouseEvent>
#include <QMenu>

ezRttiMappedObjectFactory<ezQtNode> ezQtNodeScene::s_NodeFactory;
ezRttiMappedObjectFactory<ezQtPin> ezQtNodeScene::s_PinFactory;
ezRttiMappedObjectFactory<ezQtConnection> ezQtNodeScene::s_ConnectionFactory;

ezQtNodeScene::ezQtNodeScene(QObject* parent)
  : QGraphicsScene(parent), m_pManager(nullptr), m_pStartPin(nullptr), m_pTempConnection(nullptr)
{
  setItemIndexMethod(QGraphicsScene::NoIndex);
  m_vPos = ezVec2::ZeroVector();

  //setSceneRect(-1000, -1000, 2000, 2000);
}

ezQtNodeScene::~ezQtNodeScene()
{
  SetDocumentNodeManager(nullptr);
}

void ezQtNodeScene::SetDocumentNodeManager(const ezDocumentNodeManager* pManager)
{
  if (pManager == m_pManager)
    return;

  Clear();
  if (m_pManager != nullptr)
  {
    m_pManager->m_NodeEvents.RemoveEventHandler(ezMakeDelegate(&ezQtNodeScene::NodeEventsHandler, this));
  }

  m_pManager = pManager;

  if (pManager != nullptr)
  {
    pManager->m_NodeEvents.AddEventHandler(ezMakeDelegate(&ezQtNodeScene::NodeEventsHandler, this));

    // Create Nodes
    const auto& rootObjects = pManager->GetRootObject()->GetChildren();
    for (const auto& pObject : rootObjects)
    {
      if (pManager->IsNode(pObject))
      {
        CreateNode(pObject);
      }
    }

    // Connect Pins
    for (auto it = m_Nodes.GetIterator(); it.IsValid(); ++it)
    {
      auto outputs = pManager->GetOutputPins(it.Value()->GetObject());
      for (const ezPin* pOutPin : outputs)
      {
        auto connections = pOutPin->GetConnections();
        for (const ezConnection* pConnection : connections)
        {
          ConnectPins(pConnection);
        }
      }
    }
  }
}

const ezDocumentNodeManager* ezQtNodeScene::GetDocumentNodeManager()
{
  return m_pManager;
}

ezRttiMappedObjectFactory<ezQtNode>& ezQtNodeScene::GetNodeFactory()
{
  return s_NodeFactory;
}

ezRttiMappedObjectFactory<ezQtPin>& ezQtNodeScene::GetPinFactory()
{
  return s_PinFactory;
}

ezRttiMappedObjectFactory<ezQtConnection>& ezQtNodeScene::GetConnectionFactory()
{
  return s_ConnectionFactory;
}

void ezQtNodeScene::mouseMoveEvent(QGraphicsSceneMouseEvent* event)
{
  if (m_pTempConnection)
  {
    event->accept();
    m_pTempConnection->SetPosOut(event->scenePos());
    return;
  }

  QGraphicsScene::mouseMoveEvent(event);
}

void ezQtNodeScene::mousePressEvent(QGraphicsSceneMouseEvent* event)
{
  switch (event->button())
  {
  case Qt::LeftButton:
    {
      QTransform id;

      QGraphicsItem* item = itemAt(event->scenePos(), id);
      if (item && item->type() == Type::Pin)
      {
        event->accept();
        ezQtPin* pPin = static_cast<ezQtPin*>(item);
        m_pStartPin = pPin;
        m_pTempConnection = new ezQtConnection(nullptr);
        addItem(m_pTempConnection);
        m_pTempConnection->SetPosIn(pPin->GetPinPos());
        m_pTempConnection->SetDirIn(pPin->GetPinDir());
        m_pTempConnection->SetPosOut(pPin->GetPinPos());
        m_pTempConnection->SetDirOut(-pPin->GetPinDir());
        return;
      }
    }
    break;
  case Qt::RightButton:
    {
      event->accept();
      return;
    }

  default:
    break;
  }
  GetSelection(m_Selection);

  QGraphicsScene::mousePressEvent(event);
}

void ezQtNodeScene::mouseReleaseEvent(QGraphicsSceneMouseEvent* event)
{
  if (m_pTempConnection && event->button() == Qt::LeftButton)
  {
    event->accept();
    QTransform id;
    QGraphicsItem* item = itemAt(event->scenePos(), id);
    if (item && item->type() == Type::Pin)
    {
      ezQtPin* pPin = static_cast<ezQtPin*>(item);
      if (pPin != m_pStartPin && pPin->GetPin()->GetType() != m_pStartPin->GetPin()->GetType())
      {
        const ezPin* pSourcePin = (m_pStartPin->GetPin()->GetType() == ezPin::Type::Input) ? pPin->GetPin() : m_pStartPin->GetPin();
        const ezPin* pTargetPin = (m_pStartPin->GetPin()->GetType() == ezPin::Type::Input) ? m_pStartPin->GetPin() : pPin->GetPin();
        ConnectPinsAction(pSourcePin, pTargetPin);

      }
    }

    delete m_pTempConnection;
    m_pTempConnection = nullptr;
    m_pStartPin = nullptr;
    return;
  }

  QGraphicsScene::mouseReleaseEvent(event);

  bool bSelectionChanged = false;
  ezSet<const ezDocumentObject*> moved;
  for (auto it = m_Nodes.GetIterator(); it.IsValid(); ++it)
  {
    if (it.Value()->GetFlags().IsSet(ezNodeFlags::Moved))
    {
      moved.Insert(it.Key());
    }
    if (it.Value()->GetFlags().IsSet(ezNodeFlags::SelectionChanged))
    {
      bSelectionChanged = true;
    }
    it.Value()->ResetFlags();
  }

  if (bSelectionChanged)
  {
    ezDeque<const ezDocumentObject*> selection;
    GetSelection(selection);
    // TODO const cast
    ((ezSelectionManager*)m_pManager->GetDocument()->GetSelectionManager())->SetSelection(selection);
  }

  if (!moved.IsEmpty())
  {
    ezCommandHistory* history = GetDocumentNodeManager()->GetDocument()->GetCommandHistory();
    history->StartTransaction("Move Node");

    ezStatus res;
    for (auto pObject : moved)
    {
      ezMoveNodeCommand move;
      move.m_Object = pObject->GetGuid();
      auto pos = m_Nodes[pObject]->pos();
      move.m_NewPos = ezVec2(pos.x(), pos.y());
      res = history->AddCommand(move);
      if (res.m_Result.Failed())
        break;
    }

    if (res.m_Result.Failed())
      history->CancelTransaction();
    else
      history->FinishTransaction();

    ezQtUiServices::GetSingleton()->MessageBoxStatus(res, "Move node failed");
  }
}

void ezQtNodeScene::contextMenuEvent(QGraphicsSceneContextMenuEvent* contextMenuEvent)
{
  QTransform id;

  QGraphicsItem* pItem = itemAt(contextMenuEvent->scenePos(), id);
  int iType = pItem != nullptr ? pItem->type() : -1;
  while (pItem && !(iType >= Type::Node && iType <= Type::Connection))
  {
    pItem = pItem->parentItem();
    iType = pItem != nullptr ? pItem->type() : -1;
  }

  ezQtSearchableMenu* pSearchMenu = nullptr;

  QMenu menu;
  if (iType == Type::Pin)
  {
    ezQtPin* pPin = static_cast<ezQtPin*>(pItem);
    QAction* pAction = new QAction("Disconnect Pin", &menu);
    menu.addAction(pAction);
    connect(pAction, &QAction::triggered, this, [this, pPin](bool bChecked)
    {
      DisconnectPinsAction(pPin);
    });
  }
  else if (iType == Type::Node)
  {
    ezQtNode* pNode = static_cast<ezQtNode*>(pItem);

    // if we clicked on an unselected item, make it the only selected item
    if (!pNode->isSelected())
    {
      clearSelection();
      pNode->setSelected(true);
    }

    QAction* pAction = new QAction("Remove Node", &menu);
    menu.addAction(pAction);
    connect(pAction, &QAction::triggered, this, [this](bool bChecked)
    {
      RemoveSelectedNodesAction();
    });
  }
  else if (iType == Type::Connection)
  {
    ezQtConnection* pConnection = static_cast<ezQtConnection*>(pItem);
    QAction* pAction = new QAction("Delete Connection", &menu);
    menu.addAction(pAction);
    connect(pAction, &QAction::triggered, this, [this, pConnection](bool bChecked)
    {
      DisconnectPinsAction(pConnection);
    });
  }
  else
  {
    pSearchMenu = new ezQtSearchableMenu(&menu);
    menu.addAction(pSearchMenu);

    connect(pSearchMenu, &ezQtSearchableMenu::MenuItemTriggered, this, &ezQtNodeScene::OnMenuItemTriggered);
    connect(pSearchMenu, &ezQtSearchableMenu::MenuItemTriggered, this, [&menu]() {menu.close(); });

    ezStringBuilder sFullName;

    ezHybridArray<const ezRTTI*, 32> types;
    m_pManager->GetCreateableTypes(types);
    auto pos = contextMenuEvent->scenePos();
    m_vPos = ezVec2(pos.x(), pos.y());
    for (const ezRTTI* pRtti : types)
    {
      const char* szCleanName = pRtti->GetTypeName();
      const char* szColonColon = ezStringUtils::FindLastSubString(szCleanName, "::");
      if (szColonColon != nullptr)
        szCleanName = szColonColon + 2;

      sFullName = m_pManager->GetTypeCategory(pRtti);
      sFullName.AppendPath(szCleanName);

      pSearchMenu->AddItem(sFullName, qVariantFromValue((void*)pRtti));
    }

    pSearchMenu->Finalize(m_sContextMenuSearchText);
  }

  menu.exec(contextMenuEvent->screenPos());

  if (pSearchMenu)
  {
    m_sContextMenuSearchText = pSearchMenu->GetSearchText();
  }
}

void ezQtNodeScene::keyPressEvent(QKeyEvent* event)

{
  if (event->key() == Qt::Key_Delete)
  {
    RemoveSelectedNodesAction();
    return;
  }
}

void ezQtNodeScene::Clear()
{
  while (!m_ConnectionsSourceTarget.IsEmpty())
  {
    DisconnectPins(m_ConnectionsSourceTarget.GetIterator().Key());
  }

  m_ConnectionsSourceTarget.Clear();

  while (!m_Nodes.IsEmpty())
  {
    DeleteNode(m_Nodes.GetIterator().Key());
  }
}

void ezQtNodeScene::CreateNode(const ezDocumentObject* pObject)
{
  ezVec2 vPos = m_pManager->GetNodePos(pObject);

  ezQtNode* pNode = s_NodeFactory.CreateObject(pObject->GetTypeAccessor().GetType());
  if (pNode == nullptr)
  {
    pNode = new ezQtNode();
  }
  m_Nodes[pObject] = pNode;
  addItem(pNode);
  pNode->InitNode(m_pManager, pObject);
  pNode->setPos(vPos.x, vPos.y);

  pNode->ResetFlags();
}

void ezQtNodeScene::CreateNode(const ezRTTI* pRtti)
{
  ezCommandHistory* history = m_pManager->GetDocument()->GetCommandHistory();
  history->StartTransaction("Add Node");

  ezStatus res;
  {
    ezAddObjectCommand cmd;
    cmd.m_pType = pRtti;
    cmd.m_NewObjectGuid.CreateNewUuid();
    // cmd.m_sParentProperty
    // cmd.m_Parent
    cmd.m_Index = -1;

    res = history->AddCommand(cmd);
    if (res.m_Result.Succeeded())
    {
      ezMoveNodeCommand move;
      move.m_Object = cmd.m_NewObjectGuid;
      move.m_NewPos = m_vPos;
      res = history->AddCommand(move);
    }
  }

  if (res.m_Result.Failed())
    history->CancelTransaction();
  else
    history->FinishTransaction();

  ezQtUiServices::GetSingleton()->MessageBoxStatus(res, "Adding sub-element to the property failed.");
}

void ezQtNodeScene::DeleteNode(const ezDocumentObject* pObject)
{
  ezQtNode* pNode = m_Nodes[pObject];
  m_Nodes.Remove(pObject);

  removeItem(pNode);
  delete pNode;
}

void ezQtNodeScene::NodeEventsHandler(const ezDocumentNodeManagerEvent& e)
{
  switch (e.m_EventType)
  {
  case ezDocumentNodeManagerEvent::Type::NodeMoved:
    {
      ezVec2 vPos = m_pManager->GetNodePos(e.m_pObject);
      ezQtNode* pNode = m_Nodes[e.m_pObject];
      pNode->setPos(vPos.x, vPos.y);
    }
    break;
  case ezDocumentNodeManagerEvent::Type::AfterPinsConnected:
    {
      ConnectPins(e.m_pConnection);
    }
    break;
  case ezDocumentNodeManagerEvent::Type::BeforePinsDisonnected:
    {
      DisconnectPins(e.m_pConnection);
    }
    break;
  case ezDocumentNodeManagerEvent::Type::BeforePinsChanged:
    {

    }
    break;
  case ezDocumentNodeManagerEvent::Type::AfterPinsChanged:
    {

    }
    break;
    //case ezDocumentNodeManagerEvent::Type::BeforeNodeAdded:
    //  {
    //  }
    //  break;
  case ezDocumentNodeManagerEvent::Type::AfterNodeAdded:
    {
      CreateNode(e.m_pObject);
    }
    break;
  case ezDocumentNodeManagerEvent::Type::BeforeNodeRemoved:
    {
      DeleteNode(e.m_pObject);
    }
    break;
    //case ezDocumentNodeManagerEvent::Type::AfterNodeRemoved:
    //  {
    //  }
    //  break;

  default:
    break;
  }
}

void ezQtNodeScene::GetSelection(ezDeque<const ezDocumentObject*>& selection) const
{
  selection.Clear();
  auto items = selectedItems();
  for (QGraphicsItem* pItem : items)
  {
    if (pItem->type() == ezQtNodeScene::Node)
    {
      ezQtNode* pNode = static_cast<ezQtNode*>(pItem);
      selection.PushBack(pNode->GetObject());
    }
  }
}

void ezQtNodeScene::GetSelectedNodes(ezDeque<ezQtNode*>& selection) const
{
  selection.Clear();
  auto items = selectedItems();
  for (QGraphicsItem* pItem : items)
  {
    if (pItem->type() == ezQtNodeScene::Node)
    {
      ezQtNode* pNode = static_cast<ezQtNode*>(pItem);
      selection.PushBack(pNode);
    }
  }
}

void ezQtNodeScene::ConnectPins(const ezConnection* pConnection)
{
  const ezPin* pPinSource = pConnection->GetSourcePin();
  const ezPin* pPinTarget = pConnection->GetTargetPin();

  ezQtNode* pSource = m_Nodes[pPinSource->GetParent()];
  ezQtNode* pTarget = m_Nodes[pPinTarget->GetParent()];
  ezQtPin* pOutput = pSource->GetOutputPin(pPinSource);
  ezQtPin* pInput = pTarget->GetInputPin(pPinTarget);
  EZ_ASSERT_DEV(pOutput != nullptr && pInput != nullptr, "Node does not contain pin!");

  ezQtConnection* pQtConnection = s_ConnectionFactory.CreateObject(pConnection->GetDynamicRTTI());
  if (pQtConnection == nullptr)
  {
    pQtConnection = new ezQtConnection(nullptr);
  }

  addItem(pQtConnection);
  pQtConnection->SetConnection(pConnection);
  pOutput->AddConnection(pQtConnection);
  pInput->AddConnection(pQtConnection);
  m_ConnectionsSourceTarget[pConnection] = pQtConnection;
}

void ezQtNodeScene::DisconnectPins(const ezConnection* pConnection)
{
  const ezPin* pPinSource = pConnection->GetSourcePin();
  const ezPin* pPinTarget = pConnection->GetTargetPin();

  ezQtNode* pSource = m_Nodes[pPinSource->GetParent()];
  ezQtNode* pTarget = m_Nodes[pPinTarget->GetParent()];
  ezQtPin* pOutput = pSource->GetOutputPin(pPinSource);
  ezQtPin* pInput = pTarget->GetInputPin(pPinTarget);
  EZ_ASSERT_DEV(pOutput != nullptr && pInput != nullptr, "Node does not contain pin!");

  ezQtConnection* pQtConnection = m_ConnectionsSourceTarget[pConnection];
  m_ConnectionsSourceTarget.Remove(pConnection);
  pOutput->RemoveConnection(pQtConnection);
  pInput->RemoveConnection(pQtConnection);
  removeItem(pQtConnection);
  delete pQtConnection;
}

ezStatus ezQtNodeScene::RemoveNode(ezQtNode* pNode)
{
  EZ_SUCCEED_OR_RETURN(m_pManager->CanRemove(pNode->GetObject()));

  ezRemoveNodeCommand cmd;
  cmd.m_Object = pNode->GetObject()->GetGuid();

  ezCommandHistory* history = GetDocumentNodeManager()->GetDocument()->GetCommandHistory();
  return history->AddCommand(cmd);
}

void ezQtNodeScene::RemoveSelectedNodesAction()
{
  ezDeque<ezQtNode*> selection;
  GetSelectedNodes(selection);

  if (selection.IsEmpty())
    return;

  if (ezQtUiServices::MessageBoxQuestion("Delete the selected Nodes?", QMessageBox::Yes | QMessageBox::No, QMessageBox::Yes) != QMessageBox::Yes)
    return;

  ezCommandHistory* history = GetDocumentNodeManager()->GetDocument()->GetCommandHistory();
  history->StartTransaction("Remove Nodes");

  for (ezQtNode* pNode : selection)
  {
    ezStatus res = RemoveNode(pNode);

    if (res.m_Result.Failed())
    {
      history->CancelTransaction();

      ezQtUiServices::GetSingleton()->MessageBoxStatus(res, "Failed to remove node");
      return;
    }
  }

  history->FinishTransaction();
}

void ezQtNodeScene::ConnectPinsAction(const ezPin* pSourcePin, const ezPin* pTargetPin)
{
  ezStatus res = m_pManager->CanConnect(pSourcePin, pTargetPin);
  if (res.m_Result.Succeeded())
  {
    ezCommandHistory* history = GetDocumentNodeManager()->GetDocument()->GetCommandHistory();
    history->StartTransaction("Connect Pins");

    ezConnectNodePinsCommand cmd;
    cmd.m_ObjectSource = pSourcePin->GetParent()->GetGuid();
    cmd.m_ObjectTarget = pTargetPin->GetParent()->GetGuid();
    cmd.m_sSourcePin = pSourcePin->GetName();
    cmd.m_sTargetPin = pTargetPin->GetName();

    res = history->AddCommand(cmd);
    if (res.m_Result.Failed())
      history->CancelTransaction();
    else
      history->FinishTransaction();

    ezQtUiServices::GetSingleton()->MessageBoxStatus(res, "Node connect failed.");
  }
  else
  {
    ezQtUiServices::GetSingleton()->MessageBoxStatus(res, "Node connect failed.");
  }
}

void ezQtNodeScene::DisconnectPinsAction(ezQtConnection* pConnection)
{
  ezStatus res = m_pManager->CanDisconnect(pConnection->GetConnection());
  if (res.m_Result.Succeeded())
  {
    ezCommandHistory* history = GetDocumentNodeManager()->GetDocument()->GetCommandHistory();
    history->StartTransaction("Disconnect Pins");

    ezDisconnectNodePinsCommand cmd;
    cmd.m_ObjectSource = pConnection->GetConnection()->GetSourcePin()->GetParent()->GetGuid();
    cmd.m_ObjectTarget = pConnection->GetConnection()->GetTargetPin()->GetParent()->GetGuid();
    cmd.m_sSourcePin = pConnection->GetConnection()->GetSourcePin()->GetName();
    cmd.m_sTargetPin = pConnection->GetConnection()->GetTargetPin()->GetName();

    res = history->AddCommand(cmd);
    if (res.m_Result.Failed())
      history->CancelTransaction();
    else
      history->FinishTransaction();

    ezQtUiServices::GetSingleton()->MessageBoxStatus(res, "Node disconnect failed.");
  }
  else
  {
    ezQtUiServices::GetSingleton()->MessageBoxStatus(res, "Node disconnect failed.");
  }
}

void ezQtNodeScene::DisconnectPinsAction(ezQtPin* pPin)
{
  ezHybridArray<ezQtConnection*, 6> connections;
  for (ezQtConnection* pConnection : pPin->GetConnections())
  {
    connections.PushBack(pConnection);
  }

  ezCommandHistory* history = m_pManager->GetDocument()->GetCommandHistory();
  history->StartTransaction("Disconnect Pins");

  ezStatus res = ezStatus(EZ_SUCCESS);
  for (ezQtConnection* pConnection : connections)
  {
    DisconnectPinsAction(pConnection);
  }

  if (res.m_Result.Failed())
    history->CancelTransaction();
  else
    history->FinishTransaction();

  ezQtUiServices::GetSingleton()->MessageBoxStatus(res, "Adding sub-element to the property failed.");
}


void ezQtNodeScene::OnMenuAction()
{
  const ezRTTI* pRtti = static_cast<const ezRTTI*>(sender()->property("type").value<void*>());

  CreateNode(pRtti);
}

void ezQtNodeScene::OnMenuItemTriggered(const QString& sName, const QVariant& variant)
{
  const ezRTTI* pRtti = static_cast<const ezRTTI*>(variant.value<void*>());

  CreateNode(pRtti);
}

