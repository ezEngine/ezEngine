#include <GuiFoundation/GuiFoundationPCH.h>

#include <Foundation/Strings/TranslationLookup.h>
#include <GuiFoundation/GuiFoundationDLL.h>
#include <GuiFoundation/NodeEditor/Connection.h>
#include <GuiFoundation/NodeEditor/Node.h>
#include <GuiFoundation/NodeEditor/Pin.h>
#include <GuiFoundation/UIServices/UIServices.moc.h>
#include <GuiFoundation/Widgets/SearchableMenu.moc.h>
#include <QGraphicsSceneMouseEvent>
#include <QMenu>
#include <ToolsFoundation/Command/NodeCommands.h>
#include <ToolsFoundation/Command/TreeCommands.h>

ezRttiMappedObjectFactory<ezQtNode> ezQtNodeScene::s_NodeFactory;
ezRttiMappedObjectFactory<ezQtPin> ezQtNodeScene::s_PinFactory;
ezRttiMappedObjectFactory<ezQtConnection> ezQtNodeScene::s_ConnectionFactory;
ezVec2 ezQtNodeScene::s_vLastMouseInteraction(0);

ezQtNodeScene::ezQtNodeScene(QObject* pParent)
  : QGraphicsScene(pParent)
{
  setItemIndexMethod(QGraphicsScene::NoIndex);

  connect(this, &QGraphicsScene::selectionChanged, this, &ezQtNodeScene::OnSelectionChanged);
}

ezQtNodeScene::~ezQtNodeScene()
{
  disconnect(this, &QGraphicsScene::selectionChanged, this, &ezQtNodeScene::OnSelectionChanged);

  Clear();

  if (m_pManager != nullptr)
  {
    m_pManager->m_NodeEvents.RemoveEventHandler(ezMakeDelegate(&ezQtNodeScene::NodeEventsHandler, this));
    m_pManager->GetDocument()->GetSelectionManager()->m_Events.RemoveEventHandler(ezMakeDelegate(&ezQtNodeScene::SelectionEventsHandler, this));
    m_pManager->m_PropertyEvents.RemoveEventHandler(ezMakeDelegate(&ezQtNodeScene::PropertyEventsHandler, this));
  }
}

void ezQtNodeScene::InitScene(const ezDocumentNodeManager* pManager)
{
  EZ_ASSERT_DEV(pManager != nullptr, "Invalid node manager");

  m_pManager = pManager;

  m_pManager->m_NodeEvents.AddEventHandler(ezMakeDelegate(&ezQtNodeScene::NodeEventsHandler, this));
  m_pManager->GetDocument()->GetSelectionManager()->m_Events.AddEventHandler(ezMakeDelegate(&ezQtNodeScene::SelectionEventsHandler, this));
  m_pManager->m_PropertyEvents.AddEventHandler(ezMakeDelegate(&ezQtNodeScene::PropertyEventsHandler, this));

  // Create Nodes
  const auto& rootObjects = pManager->GetRootObject()->GetChildren();
  for (const auto& pObject : rootObjects)
  {
    if (pManager->IsNode(pObject))
    {
      CreateQtNode(pObject);
    }
  }
  for (const auto& pObject : rootObjects)
  {
    if (pManager->IsConnection(pObject))
    {
      CreateQtConnection(pObject);
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

void ezQtNodeScene::SetConnectionStyle(ezEnum<ConnectionStyle> style)
{
  m_ConnectionStyle = style;
  invalidate();
}

void ezQtNodeScene::SetConnectionDecorationFlags(ezBitflags<ConnectionDecorationFlags> flags)
{
  m_ConnectionDecorationFlags = flags;
  invalidate();
}

void ezQtNodeScene::mouseMoveEvent(QGraphicsSceneMouseEvent* event)
{
  m_vMousePos = ezVec2(event->scenePos().x(), event->scenePos().y());
  s_vLastMouseInteraction = m_vMousePos;

  if (m_pTempConnection)
  {
    event->accept();

    ezVec2 bestPos = m_vMousePos;

    // snap to the closest pin that we can connect to
    if (!m_ConnectablePins.IsEmpty())
    {
      const float fPinSize = m_ConnectablePins[0]->sceneBoundingRect().height();

      // this is also the threshold at which we snap to another position
      float fDistToBest = ezMath::Square(fPinSize * 2.5f);

      for (auto pin : m_ConnectablePins)
      {
        const QPointF center = pin->sceneBoundingRect().center();
        const ezVec2 pt = ezVec2(center.x(), center.y());
        const float lenSqr = (pt - s_vLastMouseInteraction).GetLengthSquared();

        if (lenSqr < fDistToBest)
        {
          fDistToBest = lenSqr;
          bestPos = pt;
        }
      }
    }

    if (m_pStartPin->GetPin()->GetType() == ezPin::Type::Input)
    {
      m_pTempConnection->SetPosOut(QPointF(bestPos.x, bestPos.y));
    }
    else
    {
      m_pTempConnection->SetPosIn(QPointF(bestPos.x, bestPos.y));
    }
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
      QList<QGraphicsItem*> itemList = items(event->scenePos(), Qt::IntersectsItemBoundingRect);
      for (QGraphicsItem* item : itemList)
      {
        if (item->type() != Type::Pin)
          continue;

        event->accept();
        ezQtPin* pPin = static_cast<ezQtPin*>(item);
        m_pStartPin = pPin;
        m_pTempConnection = new ezQtConnection(nullptr);
        addItem(m_pTempConnection);
        m_pTempConnection->SetPosIn(pPin->GetPinPos());
        m_pTempConnection->SetPosOut(pPin->GetPinPos());

        if (pPin->GetPin()->GetType() == ezPin::Type::Input)
        {
          m_pTempConnection->SetDirIn(pPin->GetPinDir());
          m_pTempConnection->SetDirOut(-pPin->GetPinDir());
        }
        else
        {
          m_pTempConnection->SetDirIn(-pPin->GetPinDir());
          m_pTempConnection->SetDirOut(pPin->GetPinDir());
        }

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

  QGraphicsScene::mousePressEvent(event);
}

void ezQtNodeScene::mouseReleaseEvent(QGraphicsSceneMouseEvent* event)
{
  if (m_pTempConnection && event->button() == Qt::LeftButton)
  {
    event->accept();

    const bool startWasInput = m_pStartPin->GetPin()->GetType() == ezPin::Type::Input;
    const QPointF releasePos = startWasInput ? m_pTempConnection->GetOutPos() : m_pTempConnection->GetInPos();

    QList<QGraphicsItem*> itemList = items(releasePos, Qt::IntersectsItemBoundingRect);
    for (QGraphicsItem* item : itemList)
    {
      if (item->type() != Type::Pin)
        continue;

      ezQtPin* pPin = static_cast<ezQtPin*>(item);
      if (pPin != m_pStartPin && pPin->GetPin()->GetType() != m_pStartPin->GetPin()->GetType())
      {
        const ezPin* pSourcePin = startWasInput ? pPin->GetPin() : m_pStartPin->GetPin();
        const ezPin* pTargetPin = startWasInput ? m_pStartPin->GetPin() : pPin->GetPin();
        ConnectPinsAction(*pSourcePin, *pTargetPin);
        goto cleanup;
      }
    }

    OpenSearchMenu(QCursor::pos());

    if (m_pTempNode)
    {
      const auto Pins = startWasInput ? m_pTempNode->GetOutputPins() : m_pTempNode->GetInputPins();

      for (auto& pPin : Pins)
      {
        const ezPin* pSourcePin = startWasInput ? pPin->GetPin() : m_pStartPin->GetPin();
        const ezPin* pTargetPin = startWasInput ? m_pStartPin->GetPin() : pPin->GetPin();
        ezDocumentNodeManager::CanConnectResult connect;
        ezStatus res = m_pManager->CanConnect(m_pManager->GetConnectionType(), *pSourcePin, *pTargetPin, connect);
        if (res.Succeeded())
        {
          ConnectPinsAction(*pSourcePin, *pTargetPin);
          break;
        }
      }
    }

  cleanup:
    delete m_pTempConnection;
    m_pTempConnection = nullptr;
    m_pStartPin = nullptr;
    m_pTempNode = nullptr;

    ResetConnectablePinMarkup();
    return;
  }

  QGraphicsScene::mouseReleaseEvent(event);

  ezSet<const ezDocumentObject*> moved;
  for (auto it = m_Nodes.GetIterator(); it.IsValid(); ++it)
  {
    if (it.Value()->GetFlags().IsSet(ezNodeFlags::Moved))
    {
      moved.Insert(it.Key());
      it.Value()->ResetFlags();
    }
  }

  if (!moved.IsEmpty())
  {
    ezCommandHistory* history = GetDocumentNodeManager()->GetDocument()->GetCommandHistory();
    history->StartTransaction("Move Node");

    ezStatus res(EZ_SUCCESS);
    for (auto pObject : moved)
    {
      ezMoveNodeCommand move;
      move.m_Object = pObject->GetGuid();
      auto pos = m_Nodes[pObject]->pos();
      move.m_NewPos = ezVec2(pos.x(), pos.y());
      res = history->AddCommand(move);
      if (res.Failed())
        break;
    }

    if (res.Failed())
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

  QMenu menu;
  if (iType == Type::Pin)
  {
    ezQtPin* pPin = static_cast<ezQtPin*>(pItem);
    QAction* pAction = new QAction("Disconnect Pin", &menu);
    menu.addAction(pAction);
    connect(pAction, &QAction::triggered, this, [this, pPin](bool bChecked)
      { DisconnectPinsAction(pPin); });

    pPin->ExtendContextMenu(menu);
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
        { RemoveSelectedNodesAction(); });
    }

    pNode->ExtendContextMenu(menu);
  }
  else if (iType == Type::Connection)
  {
    ezQtConnection* pConnection = static_cast<ezQtConnection*>(pItem);
    QAction* pAction = new QAction("Delete Connection", &menu);
    menu.addAction(pAction);
    connect(pAction, &QAction::triggered, this, [this, pConnection](bool bChecked)
      { DisconnectPinsAction(pConnection); });

    pConnection->ExtendContextMenu(menu);
  }
  else
  {
    OpenSearchMenu(contextMenuEvent->screenPos());
    return;
  }

  menu.exec(contextMenuEvent->screenPos());
}

void ezQtNodeScene::keyPressEvent(QKeyEvent* event)
{
  QTransform id;
  QGraphicsItem* pItem = itemAt(QPointF(m_vMousePos.x, m_vMousePos.y), id);
  if (pItem && pItem->type() == Type::Pin)
  {
    ezQtPin* pin = static_cast<ezQtPin*>(pItem);
    if (event->key() == Qt::Key_Delete)
    {
      DisconnectPinsAction(pin);
    }

    pin->keyPressEvent(event);
  }

  if (event->key() == Qt::Key_Delete)
  {
    RemoveSelectedNodesAction();
  }
  else if (event->key() == Qt::Key_Space)
  {
    OpenSearchMenu(QCursor::pos());
  }

  // Pass Shortcuts/KeyPresses up the chain again, so e.g. Ctrl+S work even if inside a window
  if (event->type() == QEvent::ShortcutOverride || event->type() == QEvent::KeyPress)
  {
    event->ignore();
  }
}

void ezQtNodeScene::Clear()
{
  while (!m_Connections.IsEmpty())
  {
    DeleteQtConnection(m_Connections.GetIterator().Key());
  }

  while (!m_Nodes.IsEmpty())
  {
    DeleteQtNode(m_Nodes.GetIterator().Key());
  }
}

void ezQtNodeScene::CreateQtNode(const ezDocumentObject* pObject)
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

  // Note: We dont create connections here as it can cause recusion issues
  if (m_pTempConnection)
  {
    m_pTempNode = pNode;
  }
}

void ezQtNodeScene::DeleteQtNode(const ezDocumentObject* pObject)
{
  ezQtNode* pNode = m_Nodes[pObject];
  m_Nodes.Remove(pObject);

  removeItem(pNode);
  delete pNode;
}

void ezQtNodeScene::CreateQtConnection(const ezDocumentObject* pObject)
{
  const ezConnection& connection = m_pManager->GetConnection(pObject);
  const ezPin& pinSource = connection.GetSourcePin();
  const ezPin& pinTarget = connection.GetTargetPin();

  ezQtNode* pSource = m_Nodes[pinSource.GetParent()];
  ezQtNode* pTarget = m_Nodes[pinTarget.GetParent()];
  ezQtPin* pOutput = pSource->GetOutputPin(pinSource);
  ezQtPin* pInput = pTarget->GetInputPin(pinTarget);
  EZ_ASSERT_DEV(pOutput != nullptr && pInput != nullptr, "Node does not contain pin!");

  ezQtConnection* pQtConnection = s_ConnectionFactory.CreateObject(pObject->GetTypeAccessor().GetType());
  if (pQtConnection == nullptr)
  {
    pQtConnection = new ezQtConnection(nullptr);
  }

  addItem(pQtConnection);
  pQtConnection->InitConnection(pObject, &connection);
  pOutput->AddConnection(pQtConnection);
  pInput->AddConnection(pQtConnection);
  m_Connections[pObject] = pQtConnection;

  // reset flags to update the node's title to reflect connection changes
  pSource->ResetFlags();
  pTarget->ResetFlags();
}

void ezQtNodeScene::DeleteQtConnection(const ezDocumentObject* pObject)
{
  ezQtConnection* pQtConnection = m_Connections[pObject];
  m_Connections.Remove(pObject);

  const ezConnection* pConnection = pQtConnection->GetConnection();
  EZ_ASSERT_DEV(pConnection != nullptr, "No connection");

  const ezPin& pinSource = pConnection->GetSourcePin();
  const ezPin& pinTarget = pConnection->GetTargetPin();

  ezQtNode* pSource = m_Nodes[pinSource.GetParent()];
  ezQtNode* pTarget = m_Nodes[pinTarget.GetParent()];
  ezQtPin* pOutput = pSource->GetOutputPin(pinSource);
  ezQtPin* pInput = pTarget->GetInputPin(pinTarget);
  EZ_ASSERT_DEV(pOutput != nullptr && pInput != nullptr, "Node does not contain pin!");

  pOutput->RemoveConnection(pQtConnection);
  pInput->RemoveConnection(pQtConnection);

  removeItem(pQtConnection);
  delete pQtConnection;

  // reset flags to update the node's title to reflect connection changes
  pSource->ResetFlags();
  pTarget->ResetFlags();
}

void ezQtNodeScene::RecreateQtPins(const ezDocumentObject* pObject)
{
  ezQtNode* pNode = m_Nodes[pObject];
  pNode->CreatePins();
  pNode->UpdateState();
  pNode->UpdateGeometry();
}

void ezQtNodeScene::CreateNodeObject(const ezNodeCreationTemplate& nodeTemplate)
{
  ezCommandHistory* history = m_pManager->GetDocument()->GetCommandHistory();
  history->StartTransaction("Add Node");

  ezStatus res(EZ_SUCCESS);
  {
    ezAddObjectCommand cmd;
    cmd.m_pType = nodeTemplate.m_pType;
    cmd.m_NewObjectGuid = ezUuid::MakeUuid();
    cmd.m_Index = -1;

    res = history->AddCommand(cmd);
    if (res.Succeeded())
    {
      ezMoveNodeCommand move;
      move.m_Object = cmd.m_NewObjectGuid;
      move.m_NewPos = m_vMousePos;
      res = history->AddCommand(move);
    }

    for (auto& propValue : nodeTemplate.m_PropertyValues)
    {
      if (res.Failed())
        break;

      ezSetObjectPropertyCommand setCmd;
      setCmd.m_Object = cmd.m_NewObjectGuid;
      setCmd.m_sProperty = propValue.m_sPropertyName.GetString();
      setCmd.m_NewValue = propValue.m_Value;
      res = history->AddCommand(setCmd);
    }
  }

  if (res.Failed())
    history->CancelTransaction();
  else
    history->FinishTransaction();

  ezQtUiServices::GetSingleton()->MessageBoxStatus(res, "Adding sub-element to the property failed.");
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
      CreateQtConnection(e.m_pObject);
      break;

    case ezDocumentNodeManagerEvent::Type::BeforePinsDisonnected:
      DeleteQtConnection(e.m_pObject);
      break;

    case ezDocumentNodeManagerEvent::Type::BeforePinsChanged:
      break;

    case ezDocumentNodeManagerEvent::Type::AfterPinsChanged:
      RecreateQtPins(e.m_pObject);
      break;

    case ezDocumentNodeManagerEvent::Type::AfterNodeAdded:
      CreateQtNode(e.m_pObject);
      break;

    case ezDocumentNodeManagerEvent::Type::BeforeNodeRemoved:
      DeleteQtNode(e.m_pObject);
      break;

    default:
      break;
  }
}

void ezQtNodeScene::PropertyEventsHandler(const ezDocumentObjectPropertyEvent& e)
{
  auto it = m_Nodes.Find(e.m_pObject);
  if (it.IsValid())
  {
    it.Value()->ResetFlags();
    it.Value()->update();
  }
}

void ezQtNodeScene::SelectionEventsHandler(const ezSelectionManagerEvent& e)
{
  const ezDeque<const ezDocumentObject*>& selection = GetDocument()->GetSelectionManager()->GetSelection();

  if (!m_bIgnoreSelectionChange)
  {
    m_bIgnoreSelectionChange = true;

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

  bool bAnyPaintChanges = false;

  for (auto itCon : m_Connections)
  {
    auto pQtCon = itCon.Value();
    auto pCon = pQtCon->GetConnection();

    const bool prev = pQtCon->m_bAdjacentNodeSelected;

    pQtCon->m_bAdjacentNodeSelected = false;

    for (const ezDocumentObject* pObject : selection)
    {
      if (pCon->GetSourcePin().GetParent() == pObject || pCon->GetTargetPin().GetParent() == pObject)
      {
        pQtCon->m_bAdjacentNodeSelected = true;
        break;
      }
    }

    if (prev != pQtCon->m_bAdjacentNodeSelected)
    {
      bAnyPaintChanges = true;
    }
  }

  if (bAnyPaintChanges)
  {
    invalidate();
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

void ezQtNodeScene::MarkupConnectablePins(ezQtPin* pQtSourcePin)
{
  m_ConnectablePins.Clear();

  const ezRTTI* pConnectionType = m_pManager->GetConnectionType();

  const ezPin* pSourcePin = pQtSourcePin->GetPin();
  const bool bConnectForward = pSourcePin->GetType() == ezPin::Type::Output;

  for (auto it = m_Nodes.GetIterator(); it.IsValid(); ++it)
  {
    const ezDocumentObject* pDocObject = it.Key();
    ezQtNode* pTargetNode = it.Value();

    {
      auto pinArray = bConnectForward ? m_pManager->GetInputPins(pDocObject) : m_pManager->GetOutputPins(pDocObject);

      for (auto& pin : pinArray)
      {
        ezQtPin* pQtTargetPin = bConnectForward ? pTargetNode->GetInputPin(*pin) : pTargetNode->GetOutputPin(*pin);

        ezDocumentNodeManager::CanConnectResult res;

        if (bConnectForward)
          m_pManager->CanConnect(pConnectionType, *pSourcePin, *pin, res).IgnoreResult();
        else
          m_pManager->CanConnect(pConnectionType, *pin, *pSourcePin, res).IgnoreResult();

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
      auto pinArray = !bConnectForward ? m_pManager->GetInputPins(pDocObject) : m_pManager->GetOutputPins(pDocObject);

      for (auto& pin : pinArray)
      {
        ezQtPin* pQtTargetPin = !bConnectForward ? pTargetNode->GetInputPin(*pin) : pTargetNode->GetOutputPin(*pin);
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

    for (auto& pin : m_pManager->GetInputPins(pDocObject))
    {
      ezQtPin* pQtTargetPin = pTargetNode->GetInputPin(*pin);
      pQtTargetPin->SetHighlightState(ezQtPinHighlightState::None);
    }

    for (auto& pin : m_pManager->GetOutputPins(pDocObject))
    {
      ezQtPin* pQtTargetPin = pTargetNode->GetOutputPin(*pin);
      pQtTargetPin->SetHighlightState(ezQtPinHighlightState::None);
    }
  }
}

void ezQtNodeScene::OpenSearchMenu(QPoint screenPos)
{
  QMenu menu;
  ezQtSearchableMenu* pSearchMenu = new ezQtSearchableMenu(&menu);
  menu.addAction(pSearchMenu);

  connect(pSearchMenu, &ezQtSearchableMenu::MenuItemTriggered, this, &ezQtNodeScene::OnMenuItemTriggered);
  connect(pSearchMenu, &ezQtSearchableMenu::MenuItemTriggered, this, [&menu]()
    { menu.close(); });

  ezStringBuilder tmp;
  ezStringBuilder sFullPath;

  m_NodeCreationTemplates.Clear();
  m_pManager->GetNodeCreationTemplates(m_NodeCreationTemplates);

  for (ezUInt32 i = 0; i < m_NodeCreationTemplates.GetCount(); ++i)
  {
    const ezNodeCreationTemplate& nodeTemplate = m_NodeCreationTemplates[i];
    const ezRTTI* pRtti = nodeTemplate.m_pType;
    ezStringView sCleanName = nodeTemplate.m_sTypeName.IsEmpty() ? pRtti->GetTypeName() : nodeTemplate.m_sTypeName;

    if (const char* szUnderscore = sCleanName.FindLastSubString("_"))
    {
      sCleanName.SetStartPosition(szUnderscore + 1);
    }

    if (const char* szBracket = sCleanName.FindLastSubString("<"))
    {
      sCleanName = ezStringView(sCleanName.GetStartPointer(), szBracket);
    }

    sFullPath = nodeTemplate.m_sCategory.GetString();
    if (sFullPath.IsEmpty())
    {
      if (auto pAttr = pRtti->GetAttributeByType<ezCategoryAttribute>())
      {
        sFullPath = pAttr->GetCategory();
      }
    }

    sFullPath.AppendPath(sCleanName);

    pSearchMenu->AddItem(ezTranslate(sCleanName.GetData(tmp)), sFullPath, QVariant::fromValue(i));
  }

  pSearchMenu->Finalize(m_sContextMenuSearchText);

  menu.exec(screenPos);

  m_sContextMenuSearchText = pSearchMenu->GetSearchText();
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

    if (res.Failed())
    {
      history->CancelTransaction();

      ezQtUiServices::GetSingleton()->MessageBoxStatus(res, "Failed to remove node");
      return;
    }
  }

  history->FinishTransaction();
}

void ezQtNodeScene::ConnectPinsAction(const ezPin& sourcePin, const ezPin& targetPin)
{
  ezDocumentNodeManager::CanConnectResult connect;
  ezStatus res = m_pManager->CanConnect(m_pManager->GetConnectionType(), sourcePin, targetPin, connect);

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
    const ezArrayPtr<const ezConnection* const> connections = m_pManager->GetConnections(sourcePin);
    for (const ezConnection* pConnection : connections)
    {
      res = ezNodeCommands::DisconnectAndRemoveCommand(history, pConnection->GetParent()->GetGuid());
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
    const ezArrayPtr<const ezConnection* const> connections = m_pManager->GetConnections(targetPin);
    for (const ezConnection* pConnection : connections)
    {
      res = ezNodeCommands::DisconnectAndRemoveCommand(history, pConnection->GetParent()->GetGuid());
      if (res.Failed())
      {
        history->CancelTransaction();
        return;
      }
    }
  }

  // connect the two pins
  {
    res = ezNodeCommands::AddAndConnectCommand(history, m_pManager->GetConnectionType(), sourcePin, targetPin);
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
  if (res.Succeeded())
  {
    ezCommandHistory* history = GetDocumentNodeManager()->GetDocument()->GetCommandHistory();
    history->StartTransaction("Disconnect Pins");

    res = ezNodeCommands::DisconnectAndRemoveCommand(history, pConnection->GetConnection()->GetParent()->GetGuid());
    if (res.Failed())
      history->CancelTransaction();
    else
      history->FinishTransaction();
  }

  ezQtUiServices::GetSingleton()->MessageBoxStatus(res, "Node disconnect failed.");
}

void ezQtNodeScene::DisconnectPinsAction(ezQtPin* pPin)
{
  ezCommandHistory* history = m_pManager->GetDocument()->GetCommandHistory();
  history->StartTransaction("Disconnect Pins");

  ezStatus res = ezStatus(EZ_SUCCESS);
  for (ezQtConnection* pConnection : pPin->GetConnections())
  {
    DisconnectPinsAction(pConnection);
  }

  if (res.Failed())
    history->CancelTransaction();
  else
    history->FinishTransaction();

  ezQtUiServices::GetSingleton()->MessageBoxStatus(res, "Adding sub-element to the property failed.");
}

void ezQtNodeScene::OnMenuItemTriggered(const QString& sName, const QVariant& variant)
{
  ezUInt32 uiTypeIndex = variant.value<ezUInt32>();
  if (uiTypeIndex >= m_NodeCreationTemplates.GetCount())
    return;

  CreateNodeObject(m_NodeCreationTemplates[uiTypeIndex]);
}

void ezQtNodeScene::OnSelectionChanged()
{
  ezCommandHistory* pHistory = m_pManager->GetDocument()->GetCommandHistory();
  if (pHistory->IsInUndoRedo() || pHistory->IsInTransaction())
    return;

  m_Selection.Clear();
  auto items = selectedItems();
  for (QGraphicsItem* pItem : items)
  {
    if (pItem->type() == ezQtNodeScene::Node)
    {
      ezQtNode* pNode = static_cast<ezQtNode*>(pItem);
      m_Selection.PushBack(pNode->GetObject());
    }
    else if (pItem->type() == ezQtNodeScene::Connection)
    {
      ezQtConnection* pConnection = static_cast<ezQtConnection*>(pItem);
      m_Selection.PushBack(pConnection->GetObject());
    }
  }

  if (!m_bIgnoreSelectionChange)
  {
    m_bIgnoreSelectionChange = true;
    m_pManager->GetDocument()->GetSelectionManager()->SetSelection(m_Selection);
    m_bIgnoreSelectionChange = false;
  }
}
