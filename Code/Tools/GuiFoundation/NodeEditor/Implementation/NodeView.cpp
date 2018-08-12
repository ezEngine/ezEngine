#include <PCH.h>

#include <GuiFoundation/NodeEditor/NodeScene.moc.h>
#include <GuiFoundation/NodeEditor/NodeView.moc.h>
#include <QMouseEvent>

ezQtNodeView::ezQtNodeView(QWidget* parent)
    : QGraphicsView(parent)
    , m_pScene(nullptr)
    , m_bPanning(false)
    , m_iPanCounter(0)
{
  setRenderHints(QPainter::Antialiasing | QPainter::SmoothPixmapTransform);
  setDragMode(QGraphicsView::DragMode::RubberBandDrag);

  setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
  setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
  m_ViewPos = QPointF(0, 0);
  m_ViewScale = QPointF(1, 1);
}

ezQtNodeView::~ezQtNodeView() {}

void ezQtNodeView::SetScene(ezQtNodeScene* pScene)
{
  m_pScene = pScene;
  setScene(pScene);

  QRectF sceneRect = m_pScene->sceneRect();
  m_ViewPos = sceneRect.topLeft();
  m_ViewScale = QPointF(1, 1);
  UpdateView();
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
    m_vStartDragView = event->pos();
    m_vStartDragScene = m_ViewPos;
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
    QPoint vViewDelta = m_vStartDragView - event->pos();
    QPointF vSceneDelta = QPointF(vViewDelta.x() / m_ViewScale.x(), vViewDelta.y() / m_ViewScale.y());
    m_ViewPos = m_vStartDragScene + vSceneDelta;
    UpdateView();
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
  QPointF centerA(event->pos().x() / m_ViewScale.x(), event->pos().y() / m_ViewScale.y());

  const qreal fScaleFactor = 1.15;
  const qreal fScale = (event->delta() > 0) ? fScaleFactor : (1.0 / fScaleFactor);

  m_ViewScale *= fScale;
  m_ViewScale.setX(ezMath::Clamp(m_ViewScale.x(), 0.01, 2.0));
  m_ViewScale.setY(ezMath::Clamp(m_ViewScale.y(), 0.01, 2.0));

  QPointF centerB(event->pos().x() / m_ViewScale.x(), event->pos().y() / m_ViewScale.y());
  m_ViewPos -= (centerB - centerA);

  m_vStartDragView = event->pos();
  m_vStartDragScene = m_ViewPos;

  UpdateView();
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

void ezQtNodeView::resizeEvent(QResizeEvent* event)
{
  QGraphicsView::resizeEvent(event);
  UpdateView();
}

void ezQtNodeView::UpdateView()
{
  QRectF sceneRect(m_ViewPos.x(), m_ViewPos.y(), width() / m_ViewScale.x(), height() / m_ViewScale.y());
  setSceneRect(sceneRect);
  fitInView(sceneRect, Qt::KeepAspectRatio);
}
