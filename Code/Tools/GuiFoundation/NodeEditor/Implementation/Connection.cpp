#include <PCH.h>
#include <GuiFoundation/NodeEditor/Connection.h>
#include <QApplication>
#include <QPalette>

ezQtConnection::ezQtConnection(QGraphicsItem* parent)
  : QGraphicsPathItem(parent)
  , m_pConnection(nullptr)
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
  if (m_pConnection == nullptr)
  {
    return pen();
  }

  ezColorGammaUB color;
  const ezColorGammaUB sourceColor = m_pConnection->GetSourcePin()->GetColor();
  const ezColorGammaUB targetColor = m_pConnection->GetTargetPin()->GetColor();

  const bool isSourceGrey = (sourceColor.r == sourceColor.g && sourceColor.r == sourceColor.b);
  const bool isTargetGrey = (targetColor.r == targetColor.g && targetColor.r == targetColor.b);

  if (!isSourceGrey)
  {
    color = ezMath::Lerp(sourceColor, targetColor, 0.2f);
  }
  else if (!isTargetGrey)
  {
    color = ezMath::Lerp(sourceColor, targetColor, 0.8f);
  }
  else
  {
    color = ezMath::Lerp(sourceColor, targetColor, 0.5f);
  }

  QPen pen(QBrush(qRgb(color.r, color.g, color.b)), 3, Qt::SolidLine);

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
