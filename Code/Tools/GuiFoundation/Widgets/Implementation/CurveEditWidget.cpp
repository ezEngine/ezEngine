#include <PCH.h>
#include <GuiFoundation/Widgets/CurveEditWidget.moc.h>
#include <Foundation/Math/Color.h>
#include <Foundation/Math/Color8UNorm.h>
#include <QPainter>
#include <qevent.h>

//////////////////////////////////////////////////////////////////////////

static void AdjustGridDensity2(double& fFinestDensity, double& fRoughDensity, ezUInt32 uiWindowWidth, double fOrthoDimX, ezUInt32 uiMinPixelsForStep)
{
  const double fMaxStepsFitInWindow = (double)uiWindowWidth / (double)uiMinPixelsForStep;

  const double fStartDensity = fFinestDensity;

  ezInt32 iFactor = 1;
  double fNewDensity = fFinestDensity;
  ezInt32 iFactors[2] = { 5, 2 };
  ezInt32 iLastFactor = 0;

  while (true)
  {
    const double fStepsAtDensity = fOrthoDimX / fNewDensity;

    if (fStepsAtDensity < fMaxStepsFitInWindow)
      break;

    iFactor *= iFactors[iLastFactor];
    fNewDensity = fStartDensity * iFactor;

    iLastFactor = (iLastFactor + 1) % 2;
  }

  fFinestDensity = fStartDensity * iFactor;

  iFactor *= iFactors[iLastFactor];
  fRoughDensity = fStartDensity * iFactor;
}

static void ComputeGridExtentsX2(const QRectF& viewportSceneRect, double fGridStops, double& out_fMinX, double& out_fMaxX)
{
  out_fMinX = ezMath::Floor((double)viewportSceneRect.left(), fGridStops);
  out_fMaxX = ezMath::Ceil((double)viewportSceneRect.right(), fGridStops);
}

static void ComputeGridExtentsY2(const QRectF& viewportSceneRect, double fGridStops, double& out_fMinY, double& out_fMaxY)
{
  out_fMinY = ezMath::Floor((double)viewportSceneRect.top(), fGridStops);
  out_fMaxY = ezMath::Ceil((double)viewportSceneRect.bottom(), fGridStops);
}

//////////////////////////////////////////////////////////////////////////

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

QRectF ezQtCurveEditWidget::ComputeViewportSceneRect() const
{
  const QPointF topLeft = MapToScene(rect().topLeft());
  const QPointF bottomRight = MapToScene(rect().bottomRight());

  return QRectF(topLeft, bottomRight);
}

void ezQtCurveEditWidget::paintEvent(QPaintEvent* e)
{
  QPainter painter(this);
  painter.fillRect(rect(), palette().base());

  painter.setRenderHint(QPainter::Antialiasing, false);

  const QRectF viewportSceneRect = ComputeViewportSceneRect();

  double fFineGridDensity = 0.01;
  double fRoughGridDensity = 0.01;
  AdjustGridDensity2(fFineGridDensity, fRoughGridDensity, rect().width(), viewportSceneRect.width(), 20);

  RenderVerticalGrid(&painter, viewportSceneRect, fRoughGridDensity);

  if (m_pGridBar)
  {
    m_pGridBar->SetConfig(viewportSceneRect, fRoughGridDensity, fFineGridDensity, [this](const QPointF& pt) -> QPoint
      {
        return MapFromScene(pt);
      });
  }

  RenderSideLinesAndText(&painter, viewportSceneRect);


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

void ezQtCurveEditWidget::RenderVerticalGrid(QPainter* painter, const QRectF& viewportSceneRect, double fRoughGridDensity)
{
  double lowX, highX;
  ComputeGridExtentsX2(viewportSceneRect, fRoughGridDensity, lowX, highX);

  const int iy = rect().bottom();

  // render grid lines
  {
    QPen pen(palette().light(), 1.0f);
    pen.setCosmetic(true);
    painter->setPen(pen);

    ezHybridArray<QLine, 100> lines;

    for (double x = lowX; x <= highX; x += fRoughGridDensity)
    {
      const int ix = MapFromScene(QPointF(x, x)).x();

      QLine& l = lines.ExpandAndGetRef();
      l.setLine(ix, 0, ix, iy);
    }

    painter->drawLines(lines.GetData(), lines.GetCount());
  }
}

void ezQtCurveEditWidget::RenderSideLinesAndText(QPainter* painter, const QRectF& viewportSceneRect)
{
  double fFineGridDensity = 0.01;
  double fRoughGridDensity = 0.01;
  AdjustGridDensity2(fFineGridDensity, fRoughGridDensity, rect().width(), viewportSceneRect.width(), 20);

  painter->save();

  const ezInt32 iFineLineLength = 10;
  const ezInt32 iRoughLineLength = 20;

  QRect areaRect = rect();
  areaRect.setRight(areaRect.left() + 20);

  // render fine grid stop lines
  {
    double lowY, highY;
    ComputeGridExtentsY2(viewportSceneRect, fFineGridDensity, lowY, highY);

    if (lowY > highY)
      ezMath::Swap(lowY, highY);

    QPen pen(palette().light(), 1.0f);
    pen.setCosmetic(true);
    painter->setPen(pen);

    ezHybridArray<QLine, 100> lines;

    for (double y = lowY; y <= highY; y += fFineGridDensity)
    {
      const QPoint pos = MapFromScene(QPointF(0, y));

      QLine& l = lines.ExpandAndGetRef();
      l.setLine(0, pos.y(), iFineLineLength, pos.y());
    }

    painter->drawLines(lines.GetData(), lines.GetCount());
  }

  // render rough grid stop lines
  {
    double lowY, highY;
    ComputeGridExtentsY2(viewportSceneRect, fRoughGridDensity, lowY, highY);

    if (lowY > highY)
      ezMath::Swap(lowY, highY);

    QPen pen(palette().light(), 1.0f);
    pen.setCosmetic(true);
    painter->setPen(pen);

    ezHybridArray<QLine, 100> lines;

    for (double y = lowY; y <= highY; y += fRoughGridDensity)
    {
      const QPoint pos = MapFromScene(QPointF(0, y));

      QLine& l = lines.ExpandAndGetRef();
      l.setLine(0, pos.y(), iRoughLineLength, pos.y());
    }

    painter->drawLines(lines.GetData(), lines.GetCount());
  }

  // Grid Stop Value Text
  {
    double lowY, highY;
    ComputeGridExtentsY2(viewportSceneRect, fRoughGridDensity, lowY, highY);

    if (lowY > highY)
      ezMath::Swap(lowY, highY);

    QTextOption textOpt(Qt::AlignCenter);
    QRectF textRect;

    painter->setPen(palette().buttonText().color());

    for (double y = lowY; y <= highY; y += fRoughGridDensity)
    {
      const QPoint pos = MapFromScene(QPointF(0, y));

      textRect.setRect(0, pos.y() - 15, areaRect.width(), 15);
      painter->drawText(textRect, QString("%1").arg(y, 2), textOpt);
    }
  }

  painter->restore();
}
