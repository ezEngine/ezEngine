#include <PCH.h>
#include <GuiFoundation/Widgets/GraphicsView.moc.h>
#include <GuiFoundation/Widgets/Curve1DEditorWidget.moc.h>
#include <Foundation/Math/Math.h>
#include <QScrollBar>
#include <qevent.h>
#include <QTextOption>

ezQtGraphicsView::ezQtGraphicsView(QWidget* parent /*= nullptr*/)
  : QGraphicsView(parent)
{
  m_fZoom = 50.0f;
  m_fMinZoom = 10.0f;
  m_fMaxZoom = 1000.0f;
  m_bPanning = false;
  m_bDragging = false;
  m_bForwardMouseEvents = true;

  setRenderHints(QPainter::Antialiasing | QPainter::SmoothPixmapTransform | QPainter::HighQualityAntialiasing);
  setDragMode(QGraphicsView::DragMode::RubberBandDrag);
  setTransformationAnchor(QGraphicsView::ViewportAnchor::AnchorUnderMouse);
  setResizeAnchor(QGraphicsView::ViewportAnchor::AnchorUnderMouse);

  UpdateTransform();
}

void ezQtGraphicsView::wheelEvent(QWheelEvent* e)
{
  if (e->delta() > 0)
  {
    m_fZoom *= 1.2f;
  }
  else
  {
    m_fZoom *= 1.0f / 1.2f;
  }

  UpdateTransform();
}


void ezQtGraphicsView::mousePressEvent(QMouseEvent* e)
{
  if (e->buttons() == Qt::RightButton) // right button only
  {
    m_lastGlobalMouseMovePos = e->globalPos();
    m_bPanning = true;
    m_bForwardMouseEvents = false;
  }

  if (!m_bForwardMouseEvents || e->button() != Qt::LeftButton)
  {
    e->accept();
    return;
  }

  if (!m_bDragging)
  {
    m_bDragging = true;
    emit BeginDrag();
  }

  QGraphicsView::mousePressEvent(e);
}

void ezQtGraphicsView::mouseReleaseEvent(QMouseEvent* e)
{
  if (!e->buttons().testFlag(Qt::RightButton))
  {
    m_bPanning = false;
  }

  if (!m_bForwardMouseEvents || e->button() != Qt::LeftButton)
  {
    if (e->buttons() == Qt::NoButton)
    {
      // reset, but don't forward this one
      m_bForwardMouseEvents = true;
    }

    e->accept();
    return;
  }

  QGraphicsView::mouseReleaseEvent(e);

  if (m_bDragging)
  {
    m_bDragging = false;
    emit EndDrag();
  }
}

void ezQtGraphicsView::mouseMoveEvent(QMouseEvent* e)
{
  if (m_bPanning)
  {
    const QPoint diff = e->globalPos() - m_lastGlobalMouseMovePos;
    m_lastGlobalMouseMovePos = e->globalPos();

    // Copied from QGraphicsView
    QScrollBar* hBar = horizontalScrollBar();
    QScrollBar* vBar = verticalScrollBar();
    hBar->setValue(hBar->value() + (isRightToLeft() ? diff.x() : -diff.x()));
    vBar->setValue(vBar->value() - diff.y());
  }

  if (!m_bForwardMouseEvents /*|| (e->buttons() != Qt::LeftButton && e->buttons() != Qt::NoButton)*/) // only forward if EXACTLY the left button is down
  {
    e->accept();
    return;
  }

  QGraphicsView::mouseMoveEvent(e);
}

void ezQtGraphicsView::keyPressEvent(QKeyEvent* e)
{
  if (e->key() == Qt::Key_Delete)
  {
    emit DeleteCPs();

    e->accept();
    return;
  }
}

void ezQtGraphicsView::SetZoom(float zoom)
{
  m_fZoom = zoom;
  UpdateTransform();
}

void ezQtGraphicsView::SetZoomLimits(float minZoom, float maxZoom)
{
  m_fMinZoom = ezMath::Min(minZoom, maxZoom);
  m_fMaxZoom = ezMath::Max(minZoom, maxZoom);

  UpdateTransform();
}

void ezQtGraphicsView::UpdateTransform()
{
  m_fZoom = ezMath::Clamp(m_fZoom, m_fMinZoom, m_fMaxZoom);

  setTransform(QTransform::fromScale(m_fZoom, -m_fZoom));
}



//////////////////////////////////////////////////////////////////////////

static void AdjustGridDensity(double& fFinestDensity, double& fRoughDensity, ezUInt32 uiWindowWidth, double fOrthoDimX, ezUInt32 uiMinPixelsForStep)
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


ezQCurveView::ezQCurveView(QWidget* parent)
  : ezQtGraphicsView(parent)
{
  setBackgroundBrush(palette().base());

  setFrameShape(QFrame::NoFrame);
  setFrameShadow(QFrame::Plain);
  setContentsMargins(0, 0, 0, 0);
  setCacheMode(QGraphicsView::CacheNone);
  setViewportUpdateMode(QGraphicsView::FullViewportUpdate);
}


static void ComputeGridExtentsX(const QRectF& viewportSceneRect, double fGridStops, double& out_fMinX, double& out_fMaxX)
{
  out_fMinX = ezMath::Floor((double)viewportSceneRect.left(), fGridStops);
  out_fMaxX = ezMath::Ceil((double)viewportSceneRect.right(), fGridStops);
}

static void ComputeGridExtentsY(const QRectF& viewportSceneRect, double fGridStops, double& out_fMinX, double& out_fMaxX)
{
  out_fMinX = ezMath::Floor((double)viewportSceneRect.top(), fGridStops);
  out_fMaxX = ezMath::Ceil((double)viewportSceneRect.bottom(), fGridStops);
}

void ezQCurveView::drawBackground(QPainter *painter, const QRectF& viewportSceneRect)
{
  ezQtGraphicsView::drawBackground(painter, viewportSceneRect);

  painter->setRenderHint(QPainter::Antialiasing, false);

  double fFineGridDensity = 0.01;
  double fRoughGridDensity = 0.01;
  AdjustGridDensity(fFineGridDensity, fRoughGridDensity, rect().width(), viewportSceneRect.width(), 20);

  RenderVerticalGrid(painter, viewportSceneRect, fRoughGridDensity);

  if (m_pGridBar)
  {
    m_pGridBar->SetConfig(this, viewportSceneRect, fRoughGridDensity, fFineGridDensity);
  }

  RenderSideLinesAndText(painter, viewportSceneRect);
}


void ezQCurveView::RenderVerticalGrid(QPainter* painter, const QRectF& viewportSceneRect, double fRoughGridDensity)
{
  double lowX, highX;
  ComputeGridExtentsX(viewportSceneRect, fRoughGridDensity, lowX, highX);

  double lowY, highY;
  ComputeGridExtentsY(viewportSceneRect, fRoughGridDensity, lowY, highY);

  // render grid lines
  {
    QPen pen(palette().light(), 1.0f);
    pen.setCosmetic(true);
    painter->setPen(pen);

    ezHybridArray<QLineF, 100> lines;

    for (double x = lowX; x <= highX; x += fRoughGridDensity)
    {
      QLineF& l = lines.ExpandAndGetRef();
      l.setLine(x, lowY, x, highY);
    }

    painter->drawLines(lines.GetData(), lines.GetCount());
  }
}


void ezQCurveView::RenderSideLinesAndText(QPainter* painter, const QRectF& viewportSceneRect)
{
  double fFineGridDensity = 0.01;
  double fRoughGridDensity = 0.01;
  AdjustGridDensity(fFineGridDensity, fRoughGridDensity, rect().width(), viewportSceneRect.width(), 20);

  painter->save();
  painter->setTransform(QTransform());

  const ezInt32 iFineLineLength = 10;
  const ezInt32 iRoughLineLength = 20;

  QRect areaRect = rect();
  areaRect.setRight(areaRect.left() + 20);

  // render fine grid stop lines
  {
    double lowY, highY;
    ComputeGridExtentsY(viewportSceneRect, fFineGridDensity, lowY, highY);

    QPen pen(palette().light(), 1.0f);
    pen.setCosmetic(true);
    painter->setPen(pen);

    ezHybridArray<QLine, 100> lines;

    for (double y = lowY; y <= highY; y += fFineGridDensity)
    {
      const QPoint pos = mapFromScene(0, y);

      QLine& l = lines.ExpandAndGetRef();
      l.setLine(0, pos.y(), iFineLineLength, pos.y());
    }

    painter->drawLines(lines.GetData(), lines.GetCount());
  }

  // render rough grid stop lines
  {
    double lowY, highY;
    ComputeGridExtentsY(viewportSceneRect, fRoughGridDensity, lowY, highY);

    QPen pen(palette().light(), 1.0f);
    pen.setCosmetic(true);
    painter->setPen(pen);

    ezHybridArray<QLine, 100> lines;

    for (double y = lowY; y <= highY; y += fRoughGridDensity)
    {
      const QPoint pos = mapFromScene(0, y);

      QLine& l = lines.ExpandAndGetRef();
      l.setLine(0, pos.y(), iRoughLineLength, pos.y());
    }

    painter->drawLines(lines.GetData(), lines.GetCount());
  }

  // Grid Stop Value Text
  {
    double lowY, highY;
    ComputeGridExtentsY(viewportSceneRect, fRoughGridDensity, lowY, highY);

    QTextOption textOpt(Qt::AlignCenter);
    QRectF textRect;

    painter->setPen(palette().buttonText().color());

    for (double y = lowY; y <= highY; y += fRoughGridDensity)
    {
      const QPoint pos = mapFromScene(0, y);

      textRect.setRect(0, pos.y() - 15, areaRect.width(), 15);
      painter->drawText(textRect, QString("%1").arg(y, 2), textOpt);
    }
  }

  painter->restore();
}


void ezQCurveView::mouseDoubleClickEvent(QMouseEvent* e)
{
  if (e->button() != Qt::LeftButton)
    return;

  QPointF epsilon = mapToScene(QPoint(15, 15)) - mapToScene(QPoint(0, 0));
  QPointF scenePos = mapToScene(e->pos());

  ezQtCurve1DEditorWidget* pCurveEditor = qobject_cast<ezQtCurve1DEditorWidget*>(parent());

  if (pCurveEditor)
  {
    pCurveEditor->InsertControlPointAt(scenePos.x(), scenePos.y(), epsilon.x());
  }
}

//////////////////////////////////////////////////////////////////////////

ezQGridBarWidget::ezQGridBarWidget(QWidget* parent)
  : QWidget(parent)
{
  m_viewportSceneRect.setRect(0, 1, 1, 1);
  m_fFineGridStops = 10;
  m_fTextGridStops = 100;
}

void ezQGridBarWidget::SetConfig(QGraphicsView* pView, const QRectF& viewportSceneRect, double fTextGridStops, double fFineGridStops)
{
  m_pView = pView;

  bool bUpdate = false;
  if (m_viewportSceneRect != viewportSceneRect)
  {
    m_viewportSceneRect = viewportSceneRect;
    bUpdate = true;
  }

  if (m_fTextGridStops != fTextGridStops)
  {
    m_fTextGridStops = fTextGridStops;
    bUpdate = true;
  }

  if (m_fFineGridStops != fFineGridStops)
  {
    m_fFineGridStops = fFineGridStops;
    bUpdate = true;
  }

  if (bUpdate)
  {
    update();
  }
}

void ezQGridBarWidget::paintEvent(QPaintEvent* e)
{
  if (m_pView == nullptr)
  {
    QWidget::paintEvent(e);
    return;
  }

  QPainter Painter(this);
  QPainter* painter = &Painter;

  QRect areaRect = rect();

  // background
  painter->fillRect(areaRect, palette().button());

  // render fine grid stop lines
  {
    double fSceneMinX, fSceneMaxX;
    ComputeGridExtentsX(m_viewportSceneRect, m_fFineGridStops, fSceneMinX, fSceneMaxX);

    painter->setPen(palette().buttonText().color());

    ezHybridArray<QLine, 100> lines;

    // some overcompensation for the case that the GraphicsView displays a scrollbar at the side
    for (double x = fSceneMinX; x <= fSceneMaxX + m_fTextGridStops; x += m_fFineGridStops)
    {
      const QPoint pos = m_pView->mapFromScene(x, 0);

      QLine& l = lines.ExpandAndGetRef();
      l.setLine(pos.x(), areaRect.bottom() - 3, pos.x(), areaRect.bottom());
    }

    painter->drawLines(lines.GetData(), lines.GetCount());
  }

  // Grid Stop Value Text
  {
    double fSceneMinX, fSceneMaxX;
    ComputeGridExtentsX(m_viewportSceneRect, m_fTextGridStops, fSceneMinX, fSceneMaxX);

    QTextOption textOpt(Qt::AlignCenter);
    QRectF textRect;

    painter->setPen(palette().buttonText().color());

    for (double x = fSceneMinX; x <= fSceneMaxX; x += m_fTextGridStops)
    {
      const QPoint pos = m_pView->mapFromScene(x, 0);

      textRect.setRect(pos.x() - 50, areaRect.top(), 100, areaRect.height());
      painter->drawText(textRect, QString("%1").arg(x, 2), textOpt);
    }
  }
}

