#include <GuiFoundationPCH.h>

#include <GuiFoundation/NodeEditor/Connection.h>
#include <GuiFoundation/NodeEditor/Pin.h>
#include <QApplication>
#include <QPalette>

ezQtPin::ezQtPin()
{
  auto palette = QApplication::palette();

  QPen pen(palette.light().color(), 3, Qt::SolidLine);
  setPen(pen);
  setBrush(palette.base());

  setFlag(QGraphicsItem::ItemSendsGeometryChanges);
  setFlag(QGraphicsItem::ItemSendsScenePositionChanges);

  m_pLabel = new QGraphicsTextItem(this);
}

ezQtPin::~ezQtPin() {}

void ezQtPin::AddConnection(ezQtConnection* pConnection)
{
  EZ_ASSERT_DEBUG(!m_Connections.Contains(pConnection), "Connection already present!");
  m_Connections.PushBack(pConnection);

  ConnectedStateChanged(true);
  UpdateConnections();
}

void ezQtPin::RemoveConnection(ezQtConnection* pConnection)
{
  EZ_ASSERT_DEBUG(m_Connections.Contains(pConnection), "Connection not present!");
  m_Connections.RemoveAndSwap(pConnection);

  if (m_Connections.IsEmpty())
    ConnectedStateChanged(false);

  UpdateConnections();
}

void ezQtPin::ConnectedStateChanged(bool bConnected)
{
  if (bConnected)
  {
    setBrush(pen().color().darker(125));
  }
  else
  {
    setBrush(QApplication::palette().base());
  }
}

void ezQtPin::SetPin(const ezPin* pPin)
{
  m_pPin = pPin;

  m_pLabel->setPlainText(pPin->GetName());
  auto rectLabel = m_pLabel->boundingRect();

  int iRadus = rectLabel.height();
  if (pPin->GetType() == ezPin::Type::Input)
  {
    m_pLabel->setPos(iRadus, 0);
    QPainterPath p;
    QRectF bounds(0, 0, iRadus, iRadus);
    bounds.adjust(3, 3, -3, -3);
    m_PinCenter = bounds.center();
    p.addEllipse(bounds);
    setPath(p);
  }
  else
  {
    m_pLabel->setPos(0, 0);
    QPainterPath p;
    QRectF bounds(rectLabel.width(), 0, iRadus, iRadus);
    bounds.adjust(3, 3, -3, -3);
    m_PinCenter = bounds.center();
    p.addEllipse(bounds);
    setPath(p);
  }

  const ezColorGammaUB col = pPin->GetColor();

  QPen p = pen();
  p.setColor(qRgb(col.r, col.g, col.b));
  setPen(p);
}

QPointF ezQtPin::GetPinPos() const
{
  return mapToScene(m_PinCenter);
}

QPointF ezQtPin::GetPinDir() const
{
  if (m_pPin->GetType() == ezPin::Type::Input)
  {
    return QPointF(-1.0f, 0.0f);
  }
  else
  {
    return QPointF(1.0f, 0.0f);
    ;
  }
}

QRectF ezQtPin::GetPinRect() const
{
  auto rectLabel = m_pLabel->boundingRect();
  rectLabel.translate(m_pLabel->pos());

  if (m_pPin->GetType() == ezPin::Type::Input)
  {
    rectLabel.adjust(-5, 0, 0, 0);
  }
  else
  {
    rectLabel.adjust(0, 0, 5, 0);
  }
  return rectLabel;
}

void ezQtPin::UpdateConnections()
{
  for (ezQtConnection* pConnection : m_Connections)
  {
    if (m_pPin->GetType() == ezPin::Type::Input)
    {
      pConnection->SetDirIn(GetPinDir());
      pConnection->SetPosIn(GetPinPos());
    }
    else
    {
      pConnection->SetDirOut(GetPinDir());
      pConnection->SetPosOut(GetPinPos());
    }
  }
}

void ezQtPin::SetHighlightState(ezQtPinHighlightState state)
{
  if (m_HighlightState != state)
  {
    m_HighlightState = state;

    if (AdjustRenderingForHighlight(state))
    {
      update();
    }
  }
}

bool ezQtPin::AdjustRenderingForHighlight(ezQtPinHighlightState state)
{
  const ezColorGammaUB pinColor = GetPin()->GetColor();

  switch (state)
  {
    case ezQtPinHighlightState::None:
    {
      QPen p = pen();
      p.setColor(qRgb(pinColor.r, pinColor.g, pinColor.b));
      setPen(p);

      setBrush(GetConnections().IsEmpty() ? QApplication::palette().base() : pen().color().darker(125));
    }
    break;

    case ezQtPinHighlightState::CannotConnect:
    case ezQtPinHighlightState::CannotConnectSameDirection:
    {
      QPen p = pen();
      p.setColor(QApplication::palette().base().color().lighter());
      setPen(p);

      setBrush(QApplication::palette().base());
    }
    break;

    case ezQtPinHighlightState::CanReplaceConnection:
    case ezQtPinHighlightState::CanAddConnection:
    {
      QPen p = pen();
      p.setColor(qRgb(pinColor.r, pinColor.g, pinColor.b));
      setPen(p);

      setBrush(QApplication::palette().base());
    }
    break;
  }

  return true;
}

QVariant ezQtPin::itemChange(GraphicsItemChange change, const QVariant& value)
{
  if (change == QGraphicsItem::ItemScenePositionHasChanged)
  {
    UpdateConnections();
  }

  return QGraphicsPathItem::itemChange(change, value);
}
