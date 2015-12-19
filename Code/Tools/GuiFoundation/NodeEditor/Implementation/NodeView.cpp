#include <GuiFoundation/PCH.h>
#include <GuiFoundation/NodeEditor/NodeView.moc.h>
#include <GuiFoundation/NodeEditor/NodeScene.moc.h>
#include <QMouseEvent>
#include <QWheelEvent>
#include <QScrollBar>

ezQtNodeView::ezQtNodeView(QWidget* parent) : QGraphicsView(parent), m_pScene(nullptr), m_bPanning(false), m_iPanCounter(0)
{
  setRenderHints(QPainter::Antialiasing | QPainter::SmoothPixmapTransform | QPainter::HighQualityAntialiasing);
  setDragMode(QGraphicsView::DragMode::RubberBandDrag);
}

ezQtNodeView::~ezQtNodeView()
{

}

void ezQtNodeView::SetScene(ezQtNodeScene* pScene)
{
  m_pScene = pScene;
  setScene(pScene);
}

ezQtNodeScene* ezQtNodeView::GetScene()
{
  return m_pScene;
}

void ezQtNodeView::mousePressEvent(QMouseEvent* event)
{
  QGraphicsView::mousePressEvent(event);

  if (event->button() == Qt::RightButton)
  {
    setContextMenuPolicy(Qt::NoContextMenu);
    m_vLastPos = event->pos();
    viewport()->setCursor(Qt::ClosedHandCursor);
    event->accept();
    m_bPanning = true;
    m_iPanCounter = 0;
  }
}

void ezQtNodeView::mouseMoveEvent(QMouseEvent* event)
{
  QGraphicsView::mouseMoveEvent(event);

  if (m_bPanning)
  {
    m_iPanCounter++;
    // Copied from QGraphicsView
    QScrollBar* hBar = horizontalScrollBar();
    QScrollBar* vBar = verticalScrollBar();
    QPoint delta = event->pos() - m_vLastPos;
    hBar->setValue(hBar->value() + (isRightToLeft() ? delta.x() : -delta.x()));
    vBar->setValue(vBar->value() - delta.y());
    m_vLastPos = event->pos();
  }
}

void ezQtNodeView::mouseReleaseEvent(QMouseEvent* event)
{
  if (event->button() == Qt::RightButton && m_bPanning)
  {
    viewport()->setCursor(Qt::ArrowCursor);
    event->accept();
    m_bPanning = false;
    setContextMenuPolicy(Qt::DefaultContextMenu);
    if (m_iPanCounter > 2)
      return;
  }
  QGraphicsView::mouseReleaseEvent(event);
}

void ezQtNodeView::wheelEvent(QWheelEvent* event)
{
  qreal fScaleFactor = 1.15;
  if (event->delta() > 0)
  {
    scale(fScaleFactor, fScaleFactor);
  }
  else
  {
    scale(1.0 / fScaleFactor, 1.0 / fScaleFactor);
  }
}

void ezQtNodeView::contextMenuEvent(QContextMenuEvent* event)
{
  if (m_iPanCounter > 2)
  {
    m_iPanCounter = 0;
    return;
  }
  QGraphicsView::contextMenuEvent(event);
}

