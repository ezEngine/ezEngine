#include <GuiFoundation/PCH.h>
#include <GuiFoundation/NodeEditor/Connection.h>
#include <QApplication>
#include <QPalette>

ezQtConnection::ezQtConnection(QGraphicsItem* parent) : QGraphicsPathItem(parent)
{
  auto palette = QApplication::palette();

  QPen pen(palette.highlightedText().color(), 3, Qt::SolidLine);
  setPen(pen);
  setBrush(Qt::NoBrush);

  m_InDir = QPointF(-1.0f, 0.0f);
  m_OutDir = QPointF(1.0f, 0.0f);
  setZValue(-1);
}

ezQtConnection::~ezQtConnection()
{
}

void ezQtConnection::SetPosIn(const QPointF& point)
{
  m_InPoint = point;
  UpdateConnection();
}

void ezQtConnection::SetPosOut(const QPointF& point)
{
  m_OutPoint = point;
  UpdateConnection();
}

void ezQtConnection::SetDirIn(const QPointF& dir)
{
  m_InDir = dir;
  //m_InDir
  UpdateConnection();
}

void ezQtConnection::SetDirOut(const QPointF& dir)
{
  m_OutDir = dir;
  UpdateConnection();
}

QPen ezQtConnection::DeterminePen() const
{
  auto palette = QApplication::palette();
  QPen pen(palette.highlightedText().color(), 3, Qt::SolidLine);

  return pen;
}

void ezQtConnection::UpdateConnection()
{
  prepareGeometryChange();

  setPen(DeterminePen());

  QPainterPath p;

  p.moveTo(m_OutPoint);

  QPointF dir = m_InPoint - m_OutPoint;
  float fDotOut = QPointF::dotProduct(m_OutDir, dir);
  float fDotIn = QPointF::dotProduct(m_InDir, -dir);

  fDotOut = ezMath::Max(100.0f, ezMath::Abs(fDotOut));
  fDotIn = ezMath::Max(100.0f, ezMath::Abs(fDotIn));

  QPointF ctr1 = m_OutPoint + m_OutDir * (fDotOut * 0.5f);
  QPointF ctr2 = m_InPoint + m_InDir * (fDotIn * 0.5f);

  p.cubicTo(ctr1, ctr2, m_InPoint);

  setPath(p);
}
