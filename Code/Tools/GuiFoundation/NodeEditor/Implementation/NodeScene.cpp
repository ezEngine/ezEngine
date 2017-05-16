#include <PCH.h>
#include <ToolsFoundation/Command/TreeCommands.h>
#include <ToolsFoundation/Command/NodeCommands.h>
#include <GuiFoundation/UIServices/UIServices.moc.h>
#include <GuiFoundation/NodeEditor/Node.h>
#include <GuiFoundation/NodeEditor/Pin.h>
#include <GuiFoundation/NodeEditor/Connection.h>
#include <GuiFoundation/Widgets/SearchableMenu.moc.h>
#include <QGraphicsSceneMouseEvent>
#include <QMenu>

ezRttiMappedObjectFactory<ezQtNode> ezQtNodeScene::s_NodeFactory;
ezRttiMappedObjectFactory<ezQtPin> ezQtNodeScene::s_PinFactory;
ezRttiMappedObjectFactory<ezQtConnection> ezQtNodeScene::s_ConnectionFactory;
ezVec2 ezQtNodeScene::s_LastMouseInteraction(0);

ezQtNodeScene::ezQtNodeScene(QObject* parent)
  : QGraphicsScene(parent), m_pManager(nullptr), m_pStartPin(nullptr), m_pTempConnection(nullptr)
{
  setItemIndexMethod(QGraphicsScene::NoIndex);
  m_vPos = ezVec2::ZeroVector();
  m_bIgnoreSelectionChange = false;

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
    m_pManager->GetDocument()->GetSelectionManager()->m_Events.RemoveEventHandler(ezMakeDelegate(&ezQtNodeScene::SelectionEventsHandler, this));
    m_pManager->m_PropertyEvents.RemoveEventHandler(ezMakeDelegate(&ezQtNodeScene::PropertyEventsHandler, this));
  }

  m_pManager = pManager;

  if (pManager != nullptr)
  {
    pManager->m_NodeEvents.AddEventHandler(ezMakeDelegate(&ezQtNodeScene::NodeEventsHandler, this));
    m_pManager->GetDocument()->GetSelectionManager()->m_Events.AddEventHandler(ezMakeDelegate(&ezQtNodeScene::SelectionEventsHandler, this));
    m_pManager->m_PropertyEvents.AddEventHandler(ezMakeDelegate(&ezQtNodeScene::PropertyEventsHandler, this));

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

const ezDocument* ezQtNodeScene::GetDocument() const
{
  return m_pManager->GetDocument();
}

const ezDocumentNodeManager* ezQtNodeScene::GetDocumentNodeManager() const
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
  s_LastMouseInteraction = ezVec2(event->scenePos().x(), event->scenePos().y());

  if (m_pTempConnection)
  {
    event->accept();

    ezVec2 bestPos = s_LastMouseInteraction;

    // snap to the closest pin that we can connect to
    if (!m_ConnectablePins.IsEmpty())
    {
      const float fPinWidth = m_ConnectablePins[0]->sceneBoundingRect().width();

      // this is also the threshold at which we snap to another position
      float fDistToBest = ezMath::Square(fPinWidth * 2.5f);

      for (auto pin : m_ConnectablePins)
      {
        const QPointF center = pin->sceneBoundingRect().center();
        const ezVec2 pt = ezVec2(center.x(), center.y());
        const float lenSqr = (pt - s_LastMouseInteraction).GetLengthSquared();

        if (lenSqr < fDistToBest)
        {
          fDistToBest = lenSqr;
          bestPos = pt;
        }
      }
    }

    m_pTempConnection->SetPosOut(QPointF(bestPos.x, bestPos.y));
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

        MarkupConnectablePins(pPin);
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
    QGraphicsItem* item = itemAt(m_pTempConnection->GetOutPos(), id);
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

    ResetConnectablePinMarkup();
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

    m_bIgnoreSelectionChange = true;
    ((ezSelectionManager*)m_pManager->GetDocument()->GetSelectionManager())->SetSelection(selection);
    m_bIgnoreSelectionChange = false;
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

    // Delete Node
    {
      QAction* pAction = new QAction("Remove", &menu);
      menu.addAction(pAction);
      connect(pAction, &QAction::triggered, this, [this](bool bChecked)
      {
        RemoveSelectedNodesAction();
      });
    }
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

      const char* szUnderscore = ezStringUtils::FindLastSubString(szCleanName, "_");
      if (szUnderscore != nullptr)
        szCleanName = szUnderscore + 1;

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


void ezQtNodeScene::PropertyEventsHandler(const ezDocumentObjectPropertyEvent& e)
{
  auto it = m_Nodes.Find(e.m_pObject);

  if (!it.IsValid())
    return;

  it.Value()->UpdateTitle();
}

void ezQtNodeScene::SelectionEventsHandler(const ezSelectionManagerEvent& e)
{
  if (m_bIgnoreSelectionChange)
    return;

  m_bIgnoreSelectionChange = true;

  const ezDeque<const ezDocumentObject*>& selection = GetDocument()->GetSelectionManager()->GetSelection();

  clearSelection();

  QList<QGraphicsItem*> qSelection;
  for (const ezDocumentObject* pObject : selection)
  {
    auto it = m_Nodes.Find(pObject);
    if (!it.IsValid())
      continue;

    it.Value()->setSelected(true);
  }

  m_bIgnoreSelectionChange = false;
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

void ezQtNodeScene::MarkupConnectablePins(ezQtPin* pQtSourcePin)
{
  m_ConnectablePins.Clear();

  const bool bConnectForward = pQtSourcePin->GetPin()->GetType() == ezPin::Type::Output;

  const ezPin* pSourcePin = pQtSourcePin->GetPin();

  for (auto it = m_Nodes.GetIterator(); it.IsValid(); ++it)
  {
    const ezDocumentObject* pDocObject = it.Key();
    ezQtNode* pTargetNode = it.Value();

    {
      const ezArrayPtr<ezPin* const> pinArray = bConnectForward ? m_pManager->GetInputPins(pDocObject) : m_pManager->GetOutputPins(pDocObject);

      for (auto pin : pinArray)
      {
        ezQtPin* pQtTargetPin = bConnectForward ? pTargetNode->GetInputPin(pin) : pTargetNode->GetOutputPin(pin);

        ezDocumentNodeManager::CanConnectResult res;

        if (bConnectForward)
          m_pManager->CanConnect(pSourcePin, pin, res);
        else
          m_pManager->CanConnect(pin, pSourcePin, res);

        if (res == ezDocumentNodeManager::CanConnectResult::ConnectNever)
        {
          pQtTargetPin->SetHighlightState(ezQtPinHighlightState::CannotConnect);
        }
        else
        {
          m_ConnectablePins.PushBack(pQtTargetPin);

          if (res == ezDocumentNodeManager::CanConnectResult::Connect1toN || res == ezDocumentNodeManager::CanConnectResult::ConnectNtoN)
          {
            pQtTargetPin->SetHighlightState(ezQtPinHighlightState::CanAddConnection);
          }
          else
          {
            pQtTargetPin->SetHighlightState(ezQtPinHighlightState::CanReplaceConnection);
          }
        }
      }
    }

    {
      const ezArrayPtr<ezPin* const> pinArray = !bConnectForward ? m_pManager->GetInputPins(pDocObject) : m_pManager->GetOutputPins(pDocObject);

      for (auto pin : pinArray)
      {
        ezQtPin* pQtTargetPin = !bConnectForward ? pTargetNode->GetInputPin(pin) : pTargetNode->GetOutputPin(pin);
        pQtTargetPin->SetHighlightState(ezQtPinHighlightState::CannotConnectSameDirection);
      }
    }
  }
}

void ezQtNodeScene::ResetConnectablePinMarkup()
{
  m_ConnectablePins.Clear();

  for (auto it = m_Nodes.GetIterator(); it.IsValid(); ++it)
  {
    const ezDocumentObject* pDocObject = it.Key();
    ezQtNode* pTargetNode = it.Value();

    for (auto pin : m_pManager->GetInputPins(pDocObject))
    {
      ezQtPin* pQtTargetPin = pTargetNode->GetInputPin(pin);
      pQtTargetPin->SetHighlightState(ezQtPinHighlightState::None);
    }

    for (auto pin : m_pManager->GetOutputPins(pDocObject))
    {
      ezQtPin* pQtTargetPin = pTargetNode->GetOutputPin(pin);
      pQtTargetPin->SetHighlightState(ezQtPinHighlightState::None);
    }
  }
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
  ezDocumentNodeManager::CanConnectResult connect;
  ezStatus res = m_pManager->CanConnect(pSourcePin, pTargetPin, connect);

  if (connect == ezDocumentNodeManager::CanConnectResult::ConnectNever)
  {
    ezQtUiServices::GetSingleton()->MessageBoxStatus(res, "Failed to connect nodes.");
    return;
  }

  ezCommandHistory* history = GetDocumentNodeManager()->GetDocument()->GetCommandHistory();
  history->StartTransaction("Connect Pins");

  // disconnect everything from the source pin
  if (connect == ezDocumentNodeManager::CanConnectResult::Connect1to1 || connect == ezDocumentNodeManager::CanConnectResult::Connect1toN)
  {
    const ezArrayPtr<const ezConnection* const> connections = pSourcePin->GetConnections();
    for (const ezConnection* pConnection : connections)
    {
      ezDisconnectNodePinsCommand cmd;
      cmd.m_ObjectSource = pConnection->GetSourcePin()->GetParent()->GetGuid();
      cmd.m_ObjectTarget = pConnection->GetTargetPin()->GetParent()->GetGuid();
      cmd.m_sSourcePin = pConnection->GetSourcePin()->GetName();
      cmd.m_sTargetPin = pConnection->GetTargetPin()->GetName();

      res = history->AddCommand(cmd);

      if (res.Failed())
      {
        history->CancelTransaction();
        return;
      }
    }
  }

  // disconnect everything from the target pin
  if (connect == ezDocumentNodeManager::CanConnectResult::Connect1to1 || connect == ezDocumentNodeManager::CanConnectResult::ConnectNto1)
  {
    const ezArrayPtr<const ezConnection* const> connections = pTargetPin->GetConnections();
    for (const ezConnection* pConnection : connections)
    {
      ezDisconnectNodePinsCommand cmd;
      cmd.m_ObjectSource = pConnection->GetSourcePin()->GetParent()->GetGuid();
      cmd.m_ObjectTarget = pConnection->GetTargetPin()->GetParent()->GetGuid();
      cmd.m_sSourcePin = pConnection->GetSourcePin()->GetName();
      cmd.m_sTargetPin = pConnection->GetTargetPin()->GetName();

      res = history->AddCommand(cmd);

      if (res.Failed())
      {
        history->CancelTransaction();
        return;
      }
    }
  }

  // connect the two pins
  {
    ezConnectNodePinsCommand cmd;
    cmd.m_ObjectSource = pSourcePin->GetParent()->GetGuid();
    cmd.m_ObjectTarget = pTargetPin->GetParent()->GetGuid();
    cmd.m_sSourcePin = pSourcePin->GetName();
    cmd.m_sTargetPin = pTargetPin->GetName();

    res = history->AddCommand(cmd);

    if (res.Failed())
    {
      history->CancelTransaction();
      return;
    }
  }

  history->FinishTransaction();
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

