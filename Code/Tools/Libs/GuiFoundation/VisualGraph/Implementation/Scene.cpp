#include <GuiFoundation/GuiFoundationPCH.h>

#include <Foundation/Strings/TranslationLookup.h>
#include <GuiFoundation/GuiFoundationDLL.h>
#include <GuiFoundation/UIServices/UIServices.moc.h>
#include <GuiFoundation/VisualGraph/Connection.h>
#include <GuiFoundation/VisualGraph/Node.h>
#include <GuiFoundation/VisualGraph/Pin.h>
#include <GuiFoundation/Widgets/SearchableMenu.moc.h>
#include <QGraphicsSceneMouseEvent>
#include <QMenu>
#include <ToolsFoundation/Command/TreeCommands.h>
#include <ToolsFoundation/Command/VisualGraphCommands.h>

ezRttiMappedObjectFactory<ezQtVisualGraphNode> ezQtVisualGraphScene::s_NodeFactory;
ezRttiMappedObjectFactory<ezQtVisualGraphPin> ezQtVisualGraphScene::s_PinFactory;
ezRttiMappedObjectFactory<ezQtVisualGraphConnection> ezQtVisualGraphScene::s_ConnectionFactory;
ezVec2 ezQtVisualGraphScene::s_vLastMouseInteraction(0);

ezQtVisualGraphScene::ezQtVisualGraphScene(QObject* pParent)
  : QGraphicsScene(pParent)
{
  setItemIndexMethod(QGraphicsScene::NoIndex);

  connect(this, &QGraphicsScene::selectionChanged, this, &ezQtVisualGraphScene::OnSelectionChanged);
}

ezQtVisualGraphScene::~ezQtVisualGraphScene()
{
  disconnect(this, &QGraphicsScene::selectionChanged, this, &ezQtVisualGraphScene::OnSelectionChanged);

  Clear();

  if (m_pManager != nullptr)
  {
    m_pManager->m_NodeEvents.RemoveEventHandler(ezMakeDelegate(&ezQtVisualGraphScene::NodeEventsHandler, this));
    m_pManager->GetDocument()->GetSelectionManager()->m_Events.RemoveEventHandler(ezMakeDelegate(&ezQtVisualGraphScene::SelectionEventsHandler, this));
    m_pManager->m_PropertyEvents.RemoveEventHandler(ezMakeDelegate(&ezQtVisualGraphScene::PropertyEventsHandler, this));
  }
}

void ezQtVisualGraphScene::InitScene(const ezVisualGraphObjectManager* pManager)
{
  EZ_ASSERT_DEV(pManager != nullptr, "Invalid node manager");

  m_pManager = pManager;

  m_pManager->m_NodeEvents.AddEventHandler(ezMakeDelegate(&ezQtVisualGraphScene::NodeEventsHandler, this));
  m_pManager->GetDocument()->GetSelectionManager()->m_Events.AddEventHandler(ezMakeDelegate(&ezQtVisualGraphScene::SelectionEventsHandler, this));
  m_pManager->m_PropertyEvents.AddEventHandler(ezMakeDelegate(&ezQtVisualGraphScene::PropertyEventsHandler, this));

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

const ezDocument* ezQtVisualGraphScene::GetDocument() const
{
  return m_pManager->GetDocument();
}

const ezVisualGraphObjectManager* ezQtVisualGraphScene::GetDocumentNodeManager() const
{
  return m_pManager;
}

ezRttiMappedObjectFactory<ezQtVisualGraphNode>& ezQtVisualGraphScene::GetNodeFactory()
{
  return s_NodeFactory;
}

ezRttiMappedObjectFactory<ezQtVisualGraphPin>& ezQtVisualGraphScene::GetPinFactory()
{
  return s_PinFactory;
}

ezRttiMappedObjectFactory<ezQtVisualGraphConnection>& ezQtVisualGraphScene::GetConnectionFactory()
{
  return s_ConnectionFactory;
}

void ezQtVisualGraphScene::SetConnectionStyle(ezEnum<ConnectionStyle> style)
{
  m_ConnectionStyle = style;
  invalidate();
}

void ezQtVisualGraphScene::SetConnectionDecorationFlags(ezBitflags<ConnectionDecorationFlags> flags)
{
  m_ConnectionDecorationFlags = flags;
  invalidate();
}

void ezQtVisualGraphScene::mouseMoveEvent(QGraphicsSceneMouseEvent* event)
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

    if (m_pStartPin->GetPin()->GetType() == ezVisualGraphPin::Type::Input)
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

void ezQtVisualGraphScene::mousePressEvent(QGraphicsSceneMouseEvent* event)
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
        ezQtVisualGraphPin* pPin = static_cast<ezQtVisualGraphPin*>(item);
        m_pStartPin = pPin;
        m_pTempConnection = new ezQtVisualGraphConnection(nullptr);
        addItem(m_pTempConnection);
        m_pTempConnection->SetPosIn(pPin->GetPinPos());
        m_pTempConnection->SetPosOut(pPin->GetPinPos());

        if (pPin->GetPin()->GetType() == ezVisualGraphPin::Type::Input)
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

void ezQtVisualGraphScene::mouseReleaseEvent(QGraphicsSceneMouseEvent* event)
{
  if (m_pTempConnection && event->button() == Qt::LeftButton)
  {
    event->accept();

    const bool startWasInput = m_pStartPin->GetPin()->GetType() == ezVisualGraphPin::Type::Input;
    const QPointF releasePos = startWasInput ? m_pTempConnection->GetOutPos() : m_pTempConnection->GetInPos();

    QList<QGraphicsItem*> itemList = items(releasePos, Qt::IntersectsItemBoundingRect);
    for (QGraphicsItem* item : itemList)
    {
      if (item->type() != Type::Pin)
        continue;

      ezQtVisualGraphPin* pPin = static_cast<ezQtVisualGraphPin*>(item);
      if (pPin != m_pStartPin && pPin->GetPin()->GetType() != m_pStartPin->GetPin()->GetType())
      {
        const ezVisualGraphPin* pSourcePin = startWasInput ? pPin->GetPin() : m_pStartPin->GetPin();
        const ezVisualGraphPin* pTargetPin = startWasInput ? m_pStartPin->GetPin() : pPin->GetPin();
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
        const ezVisualGraphPin* pSourcePin = startWasInput ? pPin->GetPin() : m_pStartPin->GetPin();
        const ezVisualGraphPin* pTargetPin = startWasInput ? m_pStartPin->GetPin() : pPin->GetPin();
        ezVisualGraphObjectManager::CanConnectResult connect;
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
    if (it.Value()->GetFlags().IsSet(ezQtVisualGraphNodeFlags::Moved))
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

void ezQtVisualGraphScene::contextMenuEvent(QGraphicsSceneContextMenuEvent* contextMenuEvent)
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
    ezQtVisualGraphPin* pPin = static_cast<ezQtVisualGraphPin*>(pItem);
    QAction* pAction = new QAction("Disconnect Pin", &menu);
    menu.addAction(pAction);
    connect(pAction, &QAction::triggered, this, [this, pPin](bool bChecked)
      { DisconnectPinsAction(pPin); });

    pPin->ExtendContextMenu(menu);
  }
  else if (iType == Type::Node)
  {
    ezQtVisualGraphNode* pNode = static_cast<ezQtVisualGraphNode*>(pItem);

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
    ezQtVisualGraphConnection* pConnection = static_cast<ezQtVisualGraphConnection*>(pItem);
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

void ezQtVisualGraphScene::keyPressEvent(QKeyEvent* event)
{
  QTransform id;
  QGraphicsItem* pItem = itemAt(QPointF(m_vMousePos.x, m_vMousePos.y), id);
  if (pItem && pItem->type() == Type::Pin)
  {
    ezQtVisualGraphPin* pin = static_cast<ezQtVisualGraphPin*>(pItem);
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

void ezQtVisualGraphScene::Clear()
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

void ezQtVisualGraphScene::CreateQtNode(const ezDocumentObject* pObject)
{
  ezVec2 vPos = m_pManager->GetNodePos(pObject);

  ezQtVisualGraphNode* pNode = s_NodeFactory.CreateObject(pObject->GetTypeAccessor().GetType());
  if (pNode == nullptr)
  {
    pNode = new ezQtVisualGraphNode();
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

void ezQtVisualGraphScene::DeleteQtNode(const ezDocumentObject* pObject)
{
  ezQtVisualGraphNode* pNode = m_Nodes[pObject];
  m_Nodes.Remove(pObject);

  removeItem(pNode);
  delete pNode;
}

void ezQtVisualGraphScene::CreateQtConnection(const ezDocumentObject* pObject)
{
  const ezVisualGraphConnection& connection = m_pManager->GetConnection(pObject);
  const ezVisualGraphPin& pinSource = connection.GetSourcePin();
  const ezVisualGraphPin& pinTarget = connection.GetTargetPin();

  ezQtVisualGraphNode* pSource = m_Nodes[pinSource.GetParent()];
  ezQtVisualGraphNode* pTarget = m_Nodes[pinTarget.GetParent()];
  ezQtVisualGraphPin* pOutput = pSource->GetOutputPin(pinSource);
  ezQtVisualGraphPin* pInput = pTarget->GetInputPin(pinTarget);
  EZ_ASSERT_DEV(pOutput != nullptr && pInput != nullptr, "Node does not contain pin!");

  ezQtVisualGraphConnection* pQtConnection = s_ConnectionFactory.CreateObject(pObject->GetTypeAccessor().GetType());
  if (pQtConnection == nullptr)
  {
    pQtConnection = new ezQtVisualGraphConnection(nullptr);
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

void ezQtVisualGraphScene::DeleteQtConnection(const ezDocumentObject* pObject)
{
  ezQtVisualGraphConnection* pQtConnection = m_Connections[pObject];
  m_Connections.Remove(pObject);

  const ezVisualGraphConnection* pConnection = pQtConnection->GetConnection();
  EZ_ASSERT_DEV(pConnection != nullptr, "No connection");

  const ezVisualGraphPin& pinSource = pConnection->GetSourcePin();
  const ezVisualGraphPin& pinTarget = pConnection->GetTargetPin();

  ezQtVisualGraphNode* pSource = m_Nodes[pinSource.GetParent()];
  ezQtVisualGraphNode* pTarget = m_Nodes[pinTarget.GetParent()];
  ezQtVisualGraphPin* pOutput = pSource->GetOutputPin(pinSource);
  ezQtVisualGraphPin* pInput = pTarget->GetInputPin(pinTarget);
  EZ_ASSERT_DEV(pOutput != nullptr && pInput != nullptr, "Node does not contain pin!");

  pOutput->RemoveConnection(pQtConnection);
  pInput->RemoveConnection(pQtConnection);

  removeItem(pQtConnection);
  delete pQtConnection;

  // reset flags to update the node's title to reflect connection changes
  pSource->ResetFlags();
  pTarget->ResetFlags();
}

void ezQtVisualGraphScene::RecreateQtPins(const ezDocumentObject* pObject)
{
  ezQtVisualGraphNode* pNode = m_Nodes[pObject];
  pNode->CreatePins();
  pNode->UpdateState();
  pNode->UpdateGeometry();
}

void ezQtVisualGraphScene::CreateNodeObject(const ezVisualGraphNodeDesc& nodeTemplate)
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

void ezQtVisualGraphScene::NodeEventsHandler(const ezVisualGraphObjectManagerEvent& e)
{
  switch (e.m_EventType)
  {
    case ezVisualGraphObjectManagerEvent::Type::NodeMoved:
    {
      ezVec2 vPos = m_pManager->GetNodePos(e.m_pObject);
      ezQtVisualGraphNode* pNode = m_Nodes[e.m_pObject];
      pNode->setPos(vPos.x, vPos.y);
    }
    break;
    case ezVisualGraphObjectManagerEvent::Type::AfterPinsConnected:
      CreateQtConnection(e.m_pObject);
      break;

    case ezVisualGraphObjectManagerEvent::Type::BeforePinsDisonnected:
      DeleteQtConnection(e.m_pObject);
      break;

    case ezVisualGraphObjectManagerEvent::Type::BeforePinsChanged:
      break;

    case ezVisualGraphObjectManagerEvent::Type::AfterPinsChanged:
      RecreateQtPins(e.m_pObject);
      break;

    case ezVisualGraphObjectManagerEvent::Type::AfterNodeAdded:
      CreateQtNode(e.m_pObject);
      break;

    case ezVisualGraphObjectManagerEvent::Type::BeforeNodeRemoved:
      DeleteQtNode(e.m_pObject);
      break;

    default:
      break;
  }
}

void ezQtVisualGraphScene::PropertyEventsHandler(const ezDocumentObjectPropertyEvent& e)
{
  auto it = m_Nodes.Find(e.m_pObject);
  if (it.IsValid())
  {
    it.Value()->ResetFlags();
    it.Value()->update();
  }
}

void ezQtVisualGraphScene::SelectionEventsHandler(const ezSelectionManagerEvent& e)
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

void ezQtVisualGraphScene::GetSelectedNodes(ezDeque<ezQtVisualGraphNode*>& selection) const
{
  selection.Clear();
  auto items = selectedItems();
  for (QGraphicsItem* pItem : items)
  {
    if (pItem->type() == ezQtVisualGraphScene::Node)
    {
      ezQtVisualGraphNode* pNode = static_cast<ezQtVisualGraphNode*>(pItem);
      selection.PushBack(pNode);
    }
  }
}

void ezQtVisualGraphScene::MarkupConnectablePins(ezQtVisualGraphPin* pQtSourcePin)
{
  m_ConnectablePins.Clear();

  const ezRTTI* pConnectionType = m_pManager->GetConnectionType();

  const ezVisualGraphPin* pSourcePin = pQtSourcePin->GetPin();
  const bool bConnectForward = pSourcePin->GetType() == ezVisualGraphPin::Type::Output;

  for (auto it = m_Nodes.GetIterator(); it.IsValid(); ++it)
  {
    const ezDocumentObject* pDocObject = it.Key();
    ezQtVisualGraphNode* pTargetNode = it.Value();

    {
      auto pinArray = bConnectForward ? m_pManager->GetInputPins(pDocObject) : m_pManager->GetOutputPins(pDocObject);

      for (auto& pin : pinArray)
      {
        ezQtVisualGraphPin* pQtTargetPin = bConnectForward ? pTargetNode->GetInputPin(*pin) : pTargetNode->GetOutputPin(*pin);

        ezVisualGraphObjectManager::CanConnectResult res;

        if (bConnectForward)
          m_pManager->CanConnect(pConnectionType, *pSourcePin, *pin, res).IgnoreResult();
        else
          m_pManager->CanConnect(pConnectionType, *pin, *pSourcePin, res).IgnoreResult();

        if (res == ezVisualGraphObjectManager::CanConnectResult::ConnectNever)
        {
          pQtTargetPin->SetHighlightState(ezQtVisualGraphPinHighlight::CannotConnect);
        }
        else
        {
          m_ConnectablePins.PushBack(pQtTargetPin);

          if (res == ezVisualGraphObjectManager::CanConnectResult::Connect1toN || res == ezVisualGraphObjectManager::CanConnectResult::ConnectNtoN)
          {
            pQtTargetPin->SetHighlightState(ezQtVisualGraphPinHighlight::CanAddConnection);
          }
          else
          {
            pQtTargetPin->SetHighlightState(ezQtVisualGraphPinHighlight::CanReplaceConnection);
          }
        }
      }
    }

    {
      auto pinArray = !bConnectForward ? m_pManager->GetInputPins(pDocObject) : m_pManager->GetOutputPins(pDocObject);

      for (auto& pin : pinArray)
      {
        ezQtVisualGraphPin* pQtTargetPin = !bConnectForward ? pTargetNode->GetInputPin(*pin) : pTargetNode->GetOutputPin(*pin);
        pQtTargetPin->SetHighlightState(ezQtVisualGraphPinHighlight::CannotConnectSameDirection);
      }
    }
  }
}

void ezQtVisualGraphScene::ResetConnectablePinMarkup()
{
  m_ConnectablePins.Clear();

  for (auto it = m_Nodes.GetIterator(); it.IsValid(); ++it)
  {
    const ezDocumentObject* pDocObject = it.Key();
    ezQtVisualGraphNode* pTargetNode = it.Value();

    for (auto& pin : m_pManager->GetInputPins(pDocObject))
    {
      ezQtVisualGraphPin* pQtTargetPin = pTargetNode->GetInputPin(*pin);
      pQtTargetPin->SetHighlightState(ezQtVisualGraphPinHighlight::None);
    }

    for (auto& pin : m_pManager->GetOutputPins(pDocObject))
    {
      ezQtVisualGraphPin* pQtTargetPin = pTargetNode->GetOutputPin(*pin);
      pQtTargetPin->SetHighlightState(ezQtVisualGraphPinHighlight::None);
    }
  }
}

void ezQtVisualGraphScene::OpenSearchMenu(QPoint screenPos)
{
  QMenu menu;
  ezQtSearchableMenu* pSearchMenu = new ezQtSearchableMenu(&menu);
  menu.addAction(pSearchMenu);

  connect(pSearchMenu, &ezQtSearchableMenu::MenuItemTriggered, this, &ezQtVisualGraphScene::OnMenuItemTriggered);
  connect(pSearchMenu, &ezQtSearchableMenu::MenuItemTriggered, this, [&menu]()
    { menu.close(); });

  ezStringBuilder tmp;
  ezStringBuilder sFullPath;

  m_NodeCreationTemplates.Clear();
  m_pManager->GetNodeCreationTemplates(m_NodeCreationTemplates);

  for (ezUInt32 i = 0; i < m_NodeCreationTemplates.GetCount(); ++i)
  {
    const ezVisualGraphNodeDesc& nodeTemplate = m_NodeCreationTemplates[i];
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

ezStatus ezQtVisualGraphScene::RemoveNode(ezQtVisualGraphNode* pNode)
{
  EZ_SUCCEED_OR_RETURN(m_pManager->CanRemove(pNode->GetObject()));

  ezRemoveNodeCommand cmd;
  cmd.m_Object = pNode->GetObject()->GetGuid();

  ezCommandHistory* history = GetDocumentNodeManager()->GetDocument()->GetCommandHistory();
  return history->AddCommand(cmd);
}

void ezQtVisualGraphScene::RemoveSelectedNodesAction()
{
  ezDeque<ezQtVisualGraphNode*> selection;
  GetSelectedNodes(selection);

  if (selection.IsEmpty())
    return;

  ezCommandHistory* history = GetDocumentNodeManager()->GetDocument()->GetCommandHistory();
  history->StartTransaction("Remove Nodes");

  for (ezQtVisualGraphNode* pNode : selection)
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

void ezQtVisualGraphScene::ConnectPinsAction(const ezVisualGraphPin& sourcePin, const ezVisualGraphPin& targetPin)
{
  ezVisualGraphObjectManager::CanConnectResult connect;
  ezStatus res = m_pManager->CanConnect(m_pManager->GetConnectionType(), sourcePin, targetPin, connect);

  if (connect == ezVisualGraphObjectManager::CanConnectResult::ConnectNever)
  {
    ezQtUiServices::GetSingleton()->MessageBoxStatus(res, "Failed to connect nodes.");
    return;
  }

  ezCommandHistory* history = GetDocumentNodeManager()->GetDocument()->GetCommandHistory();
  history->StartTransaction("Connect Pins");

  // disconnect everything from the source pin
  if (connect == ezVisualGraphObjectManager::CanConnectResult::Connect1to1 || connect == ezVisualGraphObjectManager::CanConnectResult::Connect1toN)
  {
    const ezArrayPtr<const ezVisualGraphConnection* const> connections = m_pManager->GetConnections(sourcePin);
    for (const ezVisualGraphConnection* pConnection : connections)
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
  if (connect == ezVisualGraphObjectManager::CanConnectResult::Connect1to1 || connect == ezVisualGraphObjectManager::CanConnectResult::ConnectNto1)
  {
    const ezArrayPtr<const ezVisualGraphConnection* const> connections = m_pManager->GetConnections(targetPin);
    for (const ezVisualGraphConnection* pConnection : connections)
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

void ezQtVisualGraphScene::DisconnectPinsAction(ezQtVisualGraphConnection* pConnection)
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

void ezQtVisualGraphScene::DisconnectPinsAction(ezQtVisualGraphPin* pPin)
{
  ezCommandHistory* history = m_pManager->GetDocument()->GetCommandHistory();
  history->StartTransaction("Disconnect Pins");

  ezStatus res = ezStatus(EZ_SUCCESS);
  for (ezQtVisualGraphConnection* pConnection : pPin->GetConnections())
  {
    DisconnectPinsAction(pConnection);
  }

  if (res.Failed())
    history->CancelTransaction();
  else
    history->FinishTransaction();

  ezQtUiServices::GetSingleton()->MessageBoxStatus(res, "Adding sub-element to the property failed.");
}

void ezQtVisualGraphScene::OnMenuItemTriggered(const QString& sName, const QVariant& variant)
{
  ezUInt32 uiTypeIndex = variant.value<ezUInt32>();
  if (uiTypeIndex >= m_NodeCreationTemplates.GetCount())
    return;

  CreateNodeObject(m_NodeCreationTemplates[uiTypeIndex]);
}

void ezQtVisualGraphScene::OnSelectionChanged()
{
  ezCommandHistory* pHistory = m_pManager->GetDocument()->GetCommandHistory();
  if (pHistory->IsInUndoRedo() || pHistory->IsInTransaction())
    return;

  m_Selection.Clear();
  auto items = selectedItems();
  for (QGraphicsItem* pItem : items)
  {
    if (pItem->type() == ezQtVisualGraphScene::Node)
    {
      ezQtVisualGraphNode* pNode = static_cast<ezQtVisualGraphNode*>(pItem);
      m_Selection.PushBack(pNode->GetObject());
    }
    else if (pItem->type() == ezQtVisualGraphScene::Connection)
    {
      ezQtVisualGraphConnection* pConnection = static_cast<ezQtVisualGraphConnection*>(pItem);
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
