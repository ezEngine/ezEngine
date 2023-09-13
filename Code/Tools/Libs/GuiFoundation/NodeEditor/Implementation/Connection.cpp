#include <GuiFoundation/GuiFoundationPCH.h>

#include <GuiFoundation/NodeEditor/Connection.h>
#include <GuiFoundation/NodeEditor/NodeScene.moc.h>
#include <QApplication>
#include <QPalette>

ezQtConnection::ezQtConnection(QGraphicsItem* pParent)
  : QGraphicsPathItem(pParent)
{
  QPen pen(ezToQtColor(ezColor::White), 3, Qt::SolidLine);
  setPen(pen);
  setBrush(Qt::NoBrush);

  m_InDir = QPointF(-1.0f, 0.0f);
  m_OutDir = QPointF(1.0f, 0.0f);
  setZValue(-1);
}

ezQtConnection::~ezQtConnection() = default;

void ezQtConnection::InitConnection(const ezDocumentObject* pObject, const ezConnection* pConnection)
{
  m_pObject = pObject;
  m_pConnection = pConnection;
}

void ezQtConnection::SetPosIn(const QPointF& point)
{
  m_InPoint = point;
  UpdateGeometry();
}

void ezQtConnection::SetPosOut(const QPointF& point)
{
  m_OutPoint = point;
  UpdateGeometry();
}

void ezQtConnection::SetDirIn(const QPointF& dir)
{
  m_InDir = dir;
  UpdateGeometry();
}

void ezQtConnection::SetDirOut(const QPointF& dir)
{
  m_OutDir = dir;
  UpdateGeometry();
}

void ezQtConnection::UpdateGeometry()
{
  constexpr float arrowHalfSize = 8.0f;

  prepareGeometryChange();

  QPainterPath p;
  QPointF dir = m_InPoint - m_OutPoint;

  auto pScene = static_cast<ezQtNodeScene*>(scene());
  if (pScene->GetConnectionStyle() == ezQtNodeScene::ConnectionStyle::StraightLine)
  {
    QPointF startPoint = m_OutPoint;
    QPointF endPoint = m_InPoint;

    if (pScene->GetConnectionDecorationFlags().IsSet(ezQtNodeScene::ConnectionDecorationFlags::DirectionArrows))
    {
      const float length = ezMath::Sqrt(dir.x() * dir.x() + dir.y() * dir.y());
      const float invLength = length != 0.0f ? 1.0f / length : 1.0f;
      const QPointF dirNorm = dir * invLength;
      const QPointF normal = QPointF(dirNorm.y(), -dirNorm.x());

      // offset start and endpoint
      startPoint -= normal * (arrowHalfSize * 1.3f);
      endPoint -= normal * (arrowHalfSize * 1.3f);

      const QPointF midPoint = startPoint + dir * 0.5f;
      const QPointF tipPoint = midPoint + dirNorm * arrowHalfSize;
      const QPointF backPoint = midPoint - dirNorm * arrowHalfSize;

      QPolygonF arrow;
      arrow.append(tipPoint);
      arrow.append(backPoint + normal * arrowHalfSize);
      arrow.append(backPoint - normal * arrowHalfSize);
      arrow.append(tipPoint);

      p.addPolygon(arrow);
    }

    p.moveTo(startPoint);
    p.lineTo(endPoint);
  }
  else
  {
    p.moveTo(m_OutPoint);
    float fDotOut = ezMath::Abs(QPointF::dotProduct(m_OutDir, dir));
    float fDotIn = ezMath::Abs(QPointF::dotProduct(m_InDir, -dir));

    float fMinDistance = ezMath::Abs(QPointF::dotProduct(m_OutDir.transposed(), dir));
    fMinDistance = ezMath::Min(200.0f, fMinDistance);

    fDotOut = ezMath::Max(fMinDistance, fDotOut);
    fDotIn = ezMath::Max(fMinDistance, fDotIn);

    QPointF ctr1 = m_OutPoint + m_OutDir * (fDotOut * 0.5f);
    QPointF ctr2 = m_InPoint + m_InDir * (fDotIn * 0.5f);

    p.cubicTo(ctr1, ctr2, m_InPoint);
  }

  setPath(p);
}

QPen ezQtConnection::DeterminePen() const
{
  if (m_pConnection == nullptr)
  {
    return pen();
  }

  ezColorGammaUB color;
  const ezColorGammaUB sourceColor = m_pConnection->GetSourcePin().GetColor();
  const ezColorGammaUB targetColor = m_pConnection->GetTargetPin().GetColor();

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

  if (m_bAdjacentNodeSelected)
  {
    color = ezMath::Lerp(color, ezColorGammaUB(255, 255, 255), 0.1f);
    return QPen(QBrush(ezToQtColor(color)), 3, Qt::DashLine);
  }
  else
  {
    return QPen(QBrush(ezToQtColor(color)), 2, Qt::SolidLine);
  }
}

void ezQtConnection::paint(QPainter* painter, const QStyleOptionGraphicsItem* option, QWidget* widget)
{
  auto palette = QApplication::palette();

  QPen p = DeterminePen();
  if (isSelected())
  {
    p.setColor(palette.highlight().color());
  }
  painter->setPen(p);

  auto decorationFlags = static_cast<ezQtNodeScene*>(scene())->GetConnectionDecorationFlags();
  if (decorationFlags.IsSet(ezQtNodeScene::ConnectionDecorationFlags::DirectionArrows))
  {
    painter->setBrush(p.brush());
  }

  painter->drawPath(path());
}
