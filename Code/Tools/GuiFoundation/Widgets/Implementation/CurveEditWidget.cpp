#include <PCH.h>
#include <GuiFoundation/Widgets/CurveEditWidget.moc.h>
#include <QPainter>
#include <qevent.h>

ezQtCurveEditWidget::ezQtCurveEditWidget(QWidget* parent)
  : QWidget(parent)
{
  m_SceneTranslation = QPointF(-8, 8);
  m_SceneToPixelScale = QPointF(40, -40);

  m_ControlPointPen.setCosmetic(true);
  m_ControlPointPen.setColor(QColor(200, 150, 0));
  m_ControlPointPen.setStyle(Qt::PenStyle::SolidLine);

  m_ControlPointBrush.setColor(QColor(200, 150, 0));
  m_ControlPointBrush.setStyle(Qt::BrushStyle::SolidPattern);
}

void ezQtCurveEditWidget::SetCurves(const ezArrayPtr<ezCurve1D>& curves)
{
  m_Curves = curves;
  m_CurvesSorted = curves;

  for (ezCurve1D& curve : m_CurvesSorted)
  {
    curve.SortControlPoints();
    curve.CreateLinearApproximation();
  }

  update();
}

QPoint ezQtCurveEditWidget::MapFromScene(const QPointF& pos) const
{
  double x = pos.x() - m_SceneTranslation.x();
  double y = pos.y() - m_SceneTranslation.y();
  x *= m_SceneToPixelScale.x();
  y *= m_SceneToPixelScale.y();

  return QPoint((int)x, (int)y);
}

QPointF ezQtCurveEditWidget::MapToScene(const QPoint& pos) const
{
  double x = pos.x();
  double y = pos.y();
  x /= m_SceneToPixelScale.x();
  y /= m_SceneToPixelScale.y();

  return QPointF(x, y) + m_SceneTranslation;
}

void ezQtCurveEditWidget::paintEvent(QPaintEvent* e)
{
  QPainter painter(this);
  painter.fillRect(rect(), palette().base());

  PaintCurveSegments(&painter);
  PaintControlPoints(&painter);
}

void ezQtCurveEditWidget::mousePressEvent(QMouseEvent* e)
{
  QWidget::mousePressEvent(e);

  if (e->button() == Qt::RightButton)
  {
    m_LastMousePos = e->pos();
  }
}

void ezQtCurveEditWidget::mouseReleaseEvent(QMouseEvent* e)
{
  QWidget::mouseReleaseEvent(e);

}

void ezQtCurveEditWidget::mouseMoveEvent(QMouseEvent* e)
{
  QWidget::mouseMoveEvent(e);

  if (e->buttons() == Qt::RightButton)
  {
    const QPoint diff = e->pos() - m_LastMousePos;

    const double moveX = (double)diff.x() / m_SceneToPixelScale.x();
    const double moveY = (double)diff.y() / m_SceneToPixelScale.y();

    m_SceneTranslation.setX(m_SceneTranslation.x() - moveX);
    m_SceneTranslation.setY(m_SceneTranslation.y() - moveY);

    update();
  }

  m_LastMousePos = e->pos();
}

void ezQtCurveEditWidget::mouseDoubleClickEvent(QMouseEvent* e)
{
  QWidget::mouseDoubleClickEvent(e);

}

void ezQtCurveEditWidget::wheelEvent(QWheelEvent* e)
{
  const QPointF ptAt = MapToScene(mapFromGlobal(e->globalPos()));
  QPointF posDiff = m_SceneTranslation - ptAt;

  if (e->delta() > 0)
  {
    m_SceneToPixelScale.setX(m_SceneToPixelScale.x() * 1.2);
    m_SceneToPixelScale.setY(m_SceneToPixelScale.y() * 1.2);
    posDiff.setX(posDiff.x() * (1.0 / 1.2));
    posDiff.setY(posDiff.y() * (1.0 / 1.2));
  }
  else
  {
    m_SceneToPixelScale.setX(m_SceneToPixelScale.x() * (1.0 / 1.2));
    m_SceneToPixelScale.setY(m_SceneToPixelScale.y() * (1.0 / 1.2));
    posDiff.setX(posDiff.x() * 1.2);
    posDiff.setY(posDiff.y() * 1.2);
  }

  m_SceneTranslation = ptAt + posDiff;

  update();
}


void ezQtCurveEditWidget::PaintCurveSegments(QPainter* painter) const
{
  painter->save();
  painter->setBrush(Qt::NoBrush);
  painter->setPen(m_ControlPointPen);

  QPen pen;
  pen.setCosmetic(true);
  pen.setStyle(Qt::PenStyle::SolidLine);

  for (ezUInt32 curveIdx = 0; curveIdx < m_Curves.GetCount(); ++curveIdx)
  {
    const ezCurve1D& curve = m_CurvesSorted[curveIdx];
    const ezColorGammaUB curveColor = curve.GetCurveColor();

    pen.setColor(QColor(curveColor.r, curveColor.g, curveColor.b));
    painter->setPen(pen);

    QPainterPath path;

    const ezUInt32 numCps = curve.GetNumControlPoints();
    for (ezUInt32 cpIdx = 1; cpIdx < numCps; ++cpIdx)
    {
      const ezCurve1D::ControlPoint& cpPrev = curve.GetControlPoint(cpIdx - 1);
      const ezCurve1D::ControlPoint& cpThis = curve.GetControlPoint(cpIdx);

      const QPointF startPt = QPointF(cpPrev.m_Position.x, cpPrev.m_Position.y);
      const QPointF endPt = QPointF(cpThis.m_Position.x, cpThis.m_Position.y);
      const QPointF tangent1 = QPointF(cpPrev.m_RightTangent.x, cpPrev.m_RightTangent.y);
      const QPointF tangent2 = QPointF(cpThis.m_LeftTangent.x, cpThis.m_LeftTangent.y);
      const QPointF ctrlPt1 = startPt + tangent1;
      const QPointF ctrlPt2 = endPt + tangent2;

      path.moveTo(MapFromScene(startPt));
      path.cubicTo(MapFromScene(ctrlPt1), MapFromScene(ctrlPt2), MapFromScene(endPt));
    }

    painter->drawPath(path);
  }

  painter->restore();
}

void ezQtCurveEditWidget::PaintControlPoints(QPainter* painter) const
{
  painter->save();
  painter->setBrush(m_ControlPointBrush);
  painter->setPen(m_ControlPointPen);

  for (ezUInt32 curveIdx = 0; curveIdx < m_Curves.GetCount(); ++curveIdx)
  {
    const ezCurve1D& curve = m_CurvesSorted[curveIdx];

    const ezUInt32 numCps = curve.GetNumControlPoints();
    for (ezUInt32 cpIdx = 0; cpIdx < numCps; ++cpIdx)
    {
      const ezCurve1D::ControlPoint& cp = curve.GetControlPoint(cpIdx);

      const QPoint ptPos = MapFromScene(QPointF(cp.m_Position.x, cp.m_Position.y));

      painter->drawEllipse(ptPos, 3, 3);
    }
  }

  painter->restore();
}
