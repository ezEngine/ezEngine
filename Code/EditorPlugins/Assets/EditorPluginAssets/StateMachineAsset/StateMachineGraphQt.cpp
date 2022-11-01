#include <EditorPluginAssets/EditorPluginAssetsPCH.h>

#include <EditorPluginAssets/StateMachineAsset/StateMachineAsset.h>
#include <EditorPluginAssets/StateMachineAsset/StateMachineGraph.h>
#include <EditorPluginAssets/StateMachineAsset/StateMachineGraphQt.moc.h>
#include <Foundation/Math/ColorScheme.h>

ezQtStateMachinePin::ezQtStateMachinePin() = default;

void ezQtStateMachinePin::SetPin(const ezPin& pin)
{
  ezQtPin::SetPin(pin);

  constexpr int padding = 3;

  m_pLabel->setPlainText("     +     ");
  m_pLabel->setPos(0, 0);

  auto bounds = m_pLabel->boundingRect();
  m_PinCenter = bounds.center();

  QPainterPath p;
  p.addRect(bounds);
  setPath(p);

  if (pin.GetType() == ezPin::Type::Input)
  {
    m_pLabel->setPlainText("");
  }
  else
  {
    m_pLabel->setToolTip("Add Transition");
  }
}

QRectF ezQtStateMachinePin::GetPinRect() const
{
  auto rect = path().boundingRect();
  rect.translate(pos());
  return rect;
}

//////////////////////////////////////////////////////////////////////////

ezQtStateMachineConnection::ezQtStateMachineConnection(QGraphicsItem* parent /*= nullptr*/)
  : ezQtConnection(parent)
{
  setFlag(QGraphicsItem::ItemIsSelectable);
}

//////////////////////////////////////////////////////////////////////////

ezQtStateMachineNode::ezQtStateMachineNode() = default;

void ezQtStateMachineNode::InitNode(const ezDocumentNodeManager* pManager, const ezDocumentObject* pObject)
{
  ezQtNode::InitNode(pManager, pObject);

  UpdateHeaderColor();
}

void ezQtStateMachineNode::UpdateGeometry()
{
  prepareGeometryChange();

  auto labelRect = m_pLabel->boundingRect();

  constexpr int padding = 5;
  const int headerWidth = labelRect.width();
  const int headerHeight = labelRect.height() + padding * 2;

  int h = headerHeight;
  int w = headerWidth;

  for (ezQtPin* pQtPin : GetInputPins())
  {
    auto rectPin = pQtPin->GetPinRect();
    w = ezMath::Max(w, (int)rectPin.width());

    pQtPin->setPos((w - rectPin.width()) / 2.0, h);
  }

  for (ezQtPin* pQtPin : GetOutputPins())
  {
    auto rectPin = pQtPin->GetPinRect();
    w = ezMath::Max(w, (int)rectPin.width());

    pQtPin->setPos((w - rectPin.width()) / 2.0, h);
    h += rectPin.height();
  }

  w += padding * 2;
  h += padding * 2;

  m_HeaderRect = QRectF(-padding, -padding, w, headerHeight);

  {
    QPainterPath p;
    p.addRoundedRect(-padding, -padding, w, h, padding, padding);
    setPath(p);
  }
}

void ezQtStateMachineNode::UpdateState()
{
  UpdateHeaderColor();

  if (IsAnyState())
  {
    m_pLabel->setPlainText("Any State");
  }
  else
  {
    ezQtNode::UpdateState();
  }
}

void ezQtStateMachineNode::ExtendContextMenu(QMenu& menu)
{
  if (IsAnyState())
    return;

  QAction* pAction = new QAction("Set as Initial State", &menu);
  pAction->setEnabled(IsInitialState() == false);
  pAction->connect(pAction, &QAction::triggered,
    [this]() {
      auto pScene = static_cast<ezQtStateMachineAssetScene*>(scene());
      pScene->SetInitialState(this);
    });

  menu.addAction(pAction);
}

bool ezQtStateMachineNode::IsInitialState() const
{
  auto pManager = static_cast<const ezStateMachineNodeManager*>(GetObject()->GetDocumentObjectManager());
  return pManager->IsInitialState(GetObject());
}

bool ezQtStateMachineNode::IsAnyState() const
{
  auto pManager = static_cast<const ezStateMachineNodeManager*>(GetObject()->GetDocumentObjectManager());
  return pManager->IsAnyState(GetObject());
}

void ezQtStateMachineNode::UpdateHeaderColor()
{
  ezColorScheme::Enum schemeColor = ezColorScheme::Gray;
  
  if (IsAnyState())
  {
    schemeColor = ezColorScheme::Violet;
  }
  else if (IsInitialState())
  {
    schemeColor = ezColorScheme::Teal;
  }

  ezColorGammaUB c = ezColorScheme::GetColorForUI(schemeColor);
  m_HeaderColor = qRgb(c.r, c.g, c.b);

  update();
}

//////////////////////////////////////////////////////////////////////////

ezQtStateMachineAssetScene::ezQtStateMachineAssetScene(QObject* parent /*= nullptr*/)
  : ezQtNodeScene(parent)
{
  SetConnectionStyle(ezQtNodeScene::ConnectionStyle::StraightLine);
  SetConnectionDecorationFlags(ezQtNodeScene::ConnectionDecorationFlags::DirectionArrows);
}

ezQtStateMachineAssetScene::~ezQtStateMachineAssetScene() = default;

void ezQtStateMachineAssetScene::SetInitialState(ezQtStateMachineNode* pNode)
{
  ezCommandHistory* history = GetDocumentNodeManager()->GetDocument()->GetCommandHistory();
  history->StartTransaction("Set Initial State");

  ezStateMachine_SetInitialStateCommand cmd;
  cmd.m_NewInitialStateObject = pNode->GetObject()->GetGuid();

  ezStatus res = history->AddCommand(cmd);

  if (res.m_Result.Failed())
    history->CancelTransaction();
  else
    history->FinishTransaction();
}

ezStatus ezQtStateMachineAssetScene::RemoveNode(ezQtNode* pNode)
{
  auto pManager = static_cast<const ezStateMachineNodeManager*>(GetDocumentNodeManager());
  const bool bWasInitialState = pManager->IsInitialState(pNode->GetObject());

  auto res = ezQtNodeScene::RemoveNode(pNode);
  if (res.m_Result.Succeeded() && bWasInitialState)
  {
    // Find another node
    ezUuid newInitialStateObject;
    for (auto it : m_Nodes)
    {
      if (it.Value() != pNode && pManager->IsAnyState(it.Key()) == false)
      {
        newInitialStateObject = it.Key()->GetGuid();
      }
    }

    ezCommandHistory* history = pManager->GetDocument()->GetCommandHistory();

    ezStateMachine_SetInitialStateCommand cmd;
    cmd.m_NewInitialStateObject = newInitialStateObject;

    ezStatus res = history->AddCommand(cmd);
  }

  return res;
}
