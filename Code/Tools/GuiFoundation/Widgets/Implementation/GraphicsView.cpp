#include <GuiFoundation/PCH.h>
#include <GuiFoundation/Widgets/GraphicsView.moc.h>
#include <QScrollBar>
#include <qevent.h>

ezQGraphicsView::ezQGraphicsView(QWidget* parent /*= nullptr*/)
  : QGraphicsView(parent)
{
  m_fZoom = 50.0f;
  m_fMinZoom = 10.0f;
  m_fMaxZoom = 100.0f;
  m_bPanning = false;
  m_bDragging = false;
  m_bForwardMouseEvents = true;

  setRenderHints(QPainter::Antialiasing | QPainter::SmoothPixmapTransform | QPainter::HighQualityAntialiasing);
  setDragMode(QGraphicsView::DragMode::RubberBandDrag);
  setTransformationAnchor(QGraphicsView::ViewportAnchor::AnchorUnderMouse);
  setResizeAnchor(QGraphicsView::ViewportAnchor::AnchorUnderMouse);

  UpdateTransform();
}

void ezQGraphicsView::wheelEvent(QWheelEvent* e)
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


void ezQGraphicsView::mousePressEvent(QMouseEvent* e)
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

void ezQGraphicsView::mouseReleaseEvent(QMouseEvent* e)
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

void ezQGraphicsView::mouseMoveEvent(QMouseEvent* e)
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

void ezQGraphicsView::keyPressEvent(QKeyEvent* e)
{
  if (e->key() == Qt::Key_Delete)
  {
    emit DeleteCPs();

    e->accept();
    return;
  }
}

void ezQGraphicsView::SetZoom(float zoom)
{
  m_fZoom = zoom;
  UpdateTransform();
}

void ezQGraphicsView::SetZoomLimits(float minZoom, float maxZoom)
{
  m_fMinZoom = ezMath::Min(minZoom, maxZoom);
  m_fMaxZoom = ezMath::Max(minZoom, maxZoom);

  UpdateTransform();
}

void ezQGraphicsView::UpdateTransform()
{
  m_fZoom = ezMath::Clamp(m_fZoom, m_fMinZoom, m_fMaxZoom);

  setTransform(QTransform::fromScale(m_fZoom, -m_fZoom));
}


