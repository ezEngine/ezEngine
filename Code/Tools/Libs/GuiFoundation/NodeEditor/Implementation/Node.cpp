#include <GuiFoundation/GuiFoundationPCH.h>

#include "Foundation/Strings/TranslationLookup.h"
#include <GuiFoundation/NodeEditor/Node.h>
#include <GuiFoundation/NodeEditor/Pin.h>
#include <QApplication>
#include <QGraphicsDropShadowEffect>
#include <QPainter>
#include <ToolsFoundation/Document/Document.h>

ezQtNode::ezQtNode()
{
  auto palette = QApplication::palette();

  setFlag(QGraphicsItem::ItemIsMovable);
  setFlag(QGraphicsItem::ItemIsSelectable);
  setFlag(QGraphicsItem::ItemSendsGeometryChanges);

  setBrush(palette.window());
  QPen pen(palette.light().color(), 3, Qt::SolidLine);
  setPen(pen);

  m_pLabel = new QGraphicsTextItem(this);
  m_pLabel->setDefaultTextColor(palette.buttonText().color());
  QFont font = QApplication::font();
  font.setBold(true);
  m_pLabel->setFont(font);

  m_HeaderColor = palette.alternateBase().color();
}

ezQtNode::~ezQtNode()
{
  EnableDropShadow(false);
}

void ezQtNode::EnableDropShadow(bool bEnable)
{
  if (bEnable && m_pShadow == nullptr)
  {
    auto palette = QApplication::palette();

    m_pShadow = new QGraphicsDropShadowEffect();
    m_pShadow->setOffset(3, 3);
    m_pShadow->setColor(palette.color(QPalette::Shadow));
    m_pShadow->setBlurRadius(10);
    setGraphicsEffect(m_pShadow);
  }

  if (!bEnable && m_pShadow != nullptr)
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
  UpdateState();

  UpdateGeometry();

  if (const ezColorAttribute* pColorAttr = pObject->GetType()->GetAttributeByType<ezColorAttribute>())
  {
    m_HeaderColor = ezToQtColor(pColorAttr->GetColor());
  }

  m_DirtyFlags.Add(ezNodeFlags::UpdateTitle);
}

void ezQtNode::UpdateGeometry()
{
  prepareGeometryChange();

  auto labelRect = m_pLabel->boundingRect();

  QFontMetrics fm(scene()->font());
  const int headerWidth = labelRect.width();
  int h = labelRect.height() + 5;

  int y = h;

  // Align inputs
  int maxInputWidth = 0;
  for (ezQtPin* pQtPin : m_Inputs)
  {
    auto rectPin = pQtPin->GetPinRect();
    pQtPin->setPos(QPointF(-rectPin.x(), y - rectPin.y()));

    maxInputWidth = ezMath::Max(maxInputWidth, (int)rectPin.width());
    y += rectPin.height();
  }

  int maxheight = y;
  y = h;

  // Align outputs
  int maxOutputWidth = 0;
  for (ezQtPin* pQtPin : m_Outputs)
  {
    auto rectPin = pQtPin->GetPinRect();
    pQtPin->setPos(QPointF(-rectPin.x(), y - rectPin.y()));

    maxOutputWidth = ezMath::Max(maxOutputWidth, (int)rectPin.width());
    y += rectPin.height();
  }

  int w = 0;

  if (maxInputWidth == 0)
    w = maxOutputWidth;
  else if (maxOutputWidth == 0)
    w = maxInputWidth;
  else
    w = ezMath::Max(maxInputWidth, maxOutputWidth) * 2;

  w += 10;
  w = ezMath::Max(w, headerWidth);


  maxheight = ezMath::Max(maxheight, y);

  // Align outputs to the right
  for (ezUInt32 i = 0; i < m_Outputs.GetCount(); ++i)
  {
    auto rectPin = m_Outputs[i]->GetPinRect();
    m_Outputs[i]->setX(w - rectPin.width());
  }

  m_HeaderRect = QRectF(-5, -5, w + 10, labelRect.height() + 10);

  {
    QPainterPath p;
    p.addRoundedRect(-5, -5, w + 10, maxheight + 10, 5, 5);
    setPath(p);
  }
}

void ezQtNode::UpdateState()
{
  auto& typeAccessor = m_pObject->GetTypeAccessor();

  ezVariant name = typeAccessor.GetValue("Name");
  if (name.IsA<ezString>() && name.Get<ezString>().IsEmpty() == false)
  {
    m_pLabel->setPlainText(name.Get<ezString>().GetData());
  }
  else
  {
    m_pLabel->setPlainText(ezTranslate(typeAccessor.GetType()->GetTypeName()));
  }
}

void ezQtNode::SetActive(bool bActive)
{
  if (m_bIsActive != bActive)
  {
    m_bIsActive = bActive;

    for (auto pInputPin : m_Inputs)
    {
      pInputPin->SetActive(bActive);
    }

    for (auto pOutputPin : m_Outputs)
    {
      pOutputPin->SetActive(bActive);
    }
  }

  update();
}

void ezQtNode::CreatePins()
{
  auto inputs = m_pManager->GetInputPins(m_pObject);
  for (auto& pPinTarget : inputs)
  {
    ezQtPin* pQtPin = ezQtNodeScene::GetPinFactory().CreateObject(pPinTarget->GetDynamicRTTI());
    if (pQtPin == nullptr)
    {
      pQtPin = new ezQtPin();
    }
    pQtPin->setParentItem(this);
    m_Inputs.PushBack(pQtPin);

    pQtPin->SetPin(*pPinTarget);
  }

  auto outputs = m_pManager->GetOutputPins(m_pObject);
  for (auto& pPinSource : outputs)
  {
    ezQtPin* pQtPin = ezQtNodeScene::GetPinFactory().CreateObject(pPinSource->GetDynamicRTTI());
    if (pQtPin == nullptr)
    {
      pQtPin = new ezQtPin();
    }

    pQtPin->setParentItem(this);
    m_Outputs.PushBack(pQtPin);

    pQtPin->SetPin(*pPinSource);
  }
}

ezQtPin* ezQtNode::GetInputPin(const ezPin& pin)
{
  for (ezQtPin* pQtPin : m_Inputs)
  {
    if (pQtPin->GetPin() == &pin)
      return pQtPin;
  }
  return nullptr;
}

ezQtPin* ezQtNode::GetOutputPin(const ezPin& pin)
{
  for (ezQtPin* pQtPin : m_Outputs)
  {
    if (pQtPin->GetPin() == &pin)
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
  m_DirtyFlags = ezNodeFlags::UpdateTitle;
}

void ezQtNode::paint(QPainter* painter, const QStyleOptionGraphicsItem* option, QWidget* widget)
{
  if (m_DirtyFlags.IsSet(ezNodeFlags::UpdateTitle))
  {
    UpdateState();
    UpdateGeometry();
    m_DirtyFlags.Remove(ezNodeFlags::UpdateTitle);
  }

  auto palette = QApplication::palette();

  // Draw background
  painter->setPen(QPen(Qt::NoPen));
  painter->setBrush(brush());
  painter->drawPath(path());

  QColor headerColor = m_HeaderColor;
  if (!m_bIsActive)
    headerColor.setAlpha(50);

  // Draw header
  painter->setClipPath(path());
  painter->setPen(QPen(Qt::NoPen));
  painter->setBrush(headerColor);
  painter->drawRect(m_HeaderRect);
  painter->setClipping(false);

  QColor labelColor;

  // Draw outline
  if (isSelected())
  {
    QPen p = pen();
    p.setColor(palette.highlight().color());
    painter->setPen(p);

    labelColor = palette.highlightedText().color();
  }
  else
  {
    painter->setPen(pen());

    labelColor = palette.buttonText().color();
  }

  // Label
  if (!m_bIsActive)
    labelColor = labelColor.darker(150);

  const bool bBackgroundIsLight = m_HeaderColor.lightnessF() > 0.6f;
  if (bBackgroundIsLight)
  {
    labelColor.setRed(255 - labelColor.red());
    labelColor.setGreen(255 - labelColor.green());
    labelColor.setBlue(255 - labelColor.blue());
  }

  m_pLabel->setDefaultTextColor(labelColor);

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
