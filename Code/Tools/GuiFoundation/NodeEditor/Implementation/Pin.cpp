#include <PCH.h>
#include <GuiFoundation/NodeEditor/Pin.h>
#include <GuiFoundation/NodeEditor/Connection.h>
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

ezQtPin::~ezQtPin()
{

}

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
  m_Connections.RemoveSwap(pConnection);

  if (m_Connections.IsEmpty())
    ConnectedStateChanged(false);

  UpdateConnections();
}

void ezQtPin::ConnectedStateChanged(bool bConnected)
{
  if (bConnected)
  {
    auto palette = QApplication::palette();
    setBrush(palette.highlightedText());
  }
  else
  {
    auto palette = QApplication::palette();
    setBrush(palette.base());
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
    return QPointF(1.0f, 0.0f);;
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

QVariant ezQtPin::itemChange(GraphicsItemChange change, const QVariant& value)
{
  if (change == QGraphicsItem::ItemScenePositionHasChanged)
  {
    UpdateConnections();
  }

  return QGraphicsPathItem::itemChange(change, value);
}
