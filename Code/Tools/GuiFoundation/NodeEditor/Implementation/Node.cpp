#include <GuiFoundation/PCH.h>
#include <GuiFoundation/NodeEditor/Node.h>
#include <GuiFoundation/NodeEditor/Pin.h>
#include <GuiFoundation/NodeEditor/NodeScene.moc.h>
#include <ToolsFoundation/NodeObject/DocumentNodeManager.h>
#include <ToolsFoundation/Document/Document.h>
#include <ToolsFoundation/CommandHistory/CommandHistory.h>
#include <Foundation/Logging/Log.h>
#include <QGraphicsGridLayout>
#include <QGraphicsProxyWidget>
#include <QLabel>
#include <QApplication>
#include <QPen>
#include <QGraphicsDropShadowEffect>
#include <QPainter>
#include <QStyleOptionGraphicsItem>

ezQtNode::ezQtNode() : m_pManager(nullptr), m_pObject(nullptr)
{
  auto palette = QApplication::palette();

  setFlag(QGraphicsItem::ItemIsMovable);
  setFlag(QGraphicsItem::ItemIsSelectable);
  setFlag(QGraphicsItem::ItemSendsGeometryChanges);

  setBrush(palette.background());
  QPen pen(palette.light().color(), 3, Qt::SolidLine);
  setPen(pen);

  m_pLabel = new QGraphicsTextItem(this);
  m_pLabel->setDefaultTextColor(palette.buttonText().color());
  QFont font = QApplication::font();
  font.setPixelSize((float)font.pixelSize() * 1.5f);
  font.setBold(true);
  m_pLabel->setFont(font);

  m_pShadow = nullptr;
  EnableDropShadow(true);

  m_HeaderColor = palette.alternateBase().color();
}

ezQtNode::~ezQtNode()
{
  EnableDropShadow(false);
}

void ezQtNode::EnableDropShadow(bool enable)
{
  if (enable && m_pShadow == nullptr)
  {
    auto palette = QApplication::palette();

    m_pShadow = new QGraphicsDropShadowEffect();
    m_pShadow->setOffset(3, 3);
    m_pShadow->setColor(palette.color(QPalette::Shadow));
    m_pShadow->setBlurRadius(10);
    setGraphicsEffect(m_pShadow);
  }

  if (!enable && m_pShadow != nullptr)
  {
    delete m_pShadow;
    m_pShadow = nullptr;
  }
}

void ezQtNode::InitNode(const ezDocumentNodeManager* pManager, const ezDocumentObject* pObject)
{
  m_pManager = pManager;
  m_pObject = pObject;
  CreatePins();
  prepareGeometryChange();

  m_pLabel->setPlainText(pObject->GetTypeAccessor().GetType()->GetTypeName());
  auto labelRect = m_pLabel->boundingRect();

  QFontMetrics fm(scene()->font());
  int w = labelRect.width();
  int h = labelRect.height() + 5;

  int y = h;

  // Align inputs
  for (ezUInt32 i = 0; i < m_Inputs.GetCount(); ++i)
  {
    ezQtPin* pQtPin = m_Inputs[i];

    auto rectPin = pQtPin->GetPinRect();
    pQtPin->setPos(QPointF(-rectPin.x(), y - rectPin.y()));

    w = ezMath::Max(w, (int)rectPin.width());
    y += rectPin.height();
  }

  // Align outputs
  for (ezUInt32 i = 0; i < m_Outputs.GetCount(); ++i)
  {
    ezQtPin* pQtPin = m_Outputs[i];

    auto rectPin = pQtPin->GetPinRect();
    pQtPin->setPos(QPointF(-rectPin.x(), y - rectPin.y()));

    w = ezMath::Max(w, (int)rectPin.width());
    y += rectPin.height();
  }

  // Align outputs to the right
  for (ezUInt32 i = 0; i < m_Outputs.GetCount(); ++i)
  {
    auto rectPin = m_Outputs[i]->GetPinRect();
    m_Outputs[i]->setX(w - rectPin.width());
  }

  m_HeaderRect = QRectF(-5, -5, w + 10, labelRect.height() + 10);

  {
    QPainterPath p;
    p.addRoundedRect(-5, -5, w + 10, y + 10, 5, 5);
    setPath(p);
  }

  const ezColorAttribute* pColorAttr = pObject->GetType()->GetAttributeByType<ezColorAttribute>();
  if (pColorAttr)
  {
    ezColorGammaUB col = pColorAttr->GetColor();
    m_HeaderColor = qRgb(col.r, col.g, col.b);
  }
}

void ezQtNode::CreatePins()
{
  auto inputs = m_pManager->GetInputPins(m_pObject);
  for (ezUInt32 i = 0; i < inputs.GetCount(); ++i)
  {
    const ezPin* pPinTarget = inputs[i];
    ezQtPin* pQtPin = ezQtNodeScene::GetPinFactory().CreateObject(pPinTarget->GetDynamicRTTI());
    if (pQtPin == nullptr)
    {
      pQtPin = new ezQtPin();
    }
    pQtPin->setParentItem(this);
    m_Inputs.PushBack(pQtPin);

    pQtPin->SetPin(pPinTarget);
  }

  auto outputs = m_pManager->GetOutputPins(m_pObject);
  for (ezUInt32 i = 0; i < outputs.GetCount(); ++i)
  {
    const ezPin* pPinSource = outputs[i];
    ezQtPin* pQtPin = ezQtNodeScene::GetPinFactory().CreateObject(pPinSource->GetDynamicRTTI());
    if (pQtPin == nullptr)
    {
      pQtPin = new ezQtPin();
    }

    pQtPin->setParentItem(this);
    m_Outputs.PushBack(pQtPin);

    pQtPin->SetPin(pPinSource);
  }
}

ezQtPin* ezQtNode::GetInputPin(const ezPin* pPin)
{
  for (ezQtPin* pQtPin : m_Inputs)
  {
    if (pQtPin->GetPin() == pPin)
      return pQtPin;
  }
  return nullptr;
}

ezQtPin* ezQtNode::GetOutputPin(const ezPin* pPin)
{
  for (ezQtPin* pQtPin : m_Outputs)
  {
    if (pQtPin->GetPin() == pPin)
      return pQtPin;
  }
  return nullptr;
}

ezBitflags<ezNodeFlags> ezQtNode::GetFlags() const
{
  return m_DirtyFlags;
}

void ezQtNode::ResetFlags()
{
  m_DirtyFlags = ezNodeFlags::None;
}

void ezQtNode::paint(QPainter* painter, const QStyleOptionGraphicsItem* option, QWidget* widget)
{
  auto palette = QApplication::palette();

  // Draw background
  painter->setPen(QPen(Qt::NoPen));
  painter->setBrush(brush());
  painter->drawPath(path());

  // Draw header
  painter->setClipPath(path());
  painter->setPen(QPen(Qt::NoPen));
  painter->setBrush(m_HeaderColor);
  painter->drawRect(m_HeaderRect);
  painter->setClipping(false);

  // Draw outline

  if (isSelected())
  {
    QPen p = pen();
    p.setColor(palette.highlight().color());
    painter->setPen(p);
  }
  else
  {
    painter->setPen(pen());
  }

  painter->setBrush(QBrush(Qt::NoBrush));
  painter->drawPath(path());
}

QVariant ezQtNode::itemChange(GraphicsItemChange change, const QVariant& value)
{
  if (!m_pObject)
    return QGraphicsPathItem::itemChange(change, value);

  ezCommandHistory* pHistory = m_pManager->GetDocument()->GetCommandHistory();
  switch (change)
  {
  case QGraphicsItem::ItemSelectedHasChanged:
    {
      if (!pHistory->IsInUndoRedo() && !pHistory->IsInTransaction())
        m_DirtyFlags.Add(ezNodeFlags::SelectionChanged);

      auto palette = QApplication::palette();
      m_pLabel->setDefaultTextColor(isSelected() ? palette.highlightedText().color() : palette.buttonText().color());
    }
    break;
  case QGraphicsItem::ItemPositionHasChanged:
    {
      if (!pHistory->IsInUndoRedo() && !pHistory->IsInTransaction())
        m_DirtyFlags.Add(ezNodeFlags::Moved);
    }
    break;

  default:
    break;
  }
  return QGraphicsPathItem::itemChange(change, value);
}

