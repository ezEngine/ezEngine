#include <GuiFoundation/GuiFoundationPCH.h>

#include <GuiFoundation/NodeEditor/NodeScene.moc.h>
#include <GuiFoundation/NodeEditor/NodeView.moc.h>
#include <QMouseEvent>

ezQtNodeView::ezQtNodeView(QWidget* pParent)
  : QGraphicsView(pParent)

{
  setRenderHints(QPainter::Antialiasing | QPainter::SmoothPixmapTransform);
  setDragMode(QGraphicsView::DragMode::RubberBandDrag);

  setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
  setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
  m_ViewPos = QPointF(0, 0);
  m_ViewScale = QPointF(1, 1);
}

ezQtNodeView::~ezQtNodeView() = default;

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
    m_StartDragView = event->position();

    m_StartDragScene = m_ViewPos;
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
#if QT_VERSION >= QT_VERSION_CHECK(5, 15, 0)
    QPointF vViewDelta = m_StartDragView - event->position();
#else
    QPointF vViewDelta = m_vStartDragView - event->pos();
#endif
    QPointF vSceneDelta = QPointF(vViewDelta.x() / m_ViewScale.x(), vViewDelta.y() / m_ViewScale.y());
    m_ViewPos = m_StartDragScene + vSceneDelta;
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
#if QT_VERSION >= QT_VERSION_CHECK(5, 15, 0)
  QPointF centerA(event->position().x() / m_ViewScale.x(), event->position().y() / m_ViewScale.y());
#else
  QPointF centerA(event->pos().x() / m_ViewScale.x(), event->pos().y() / m_ViewScale.y());
#endif

  const qreal fScaleFactor = 1.15;
#if QT_VERSION >= QT_VERSION_CHECK(5, 15, 0)
  const qreal fScale = (event->angleDelta().y() > 0) ? fScaleFactor : (1.0 / fScaleFactor);
#else
  const qreal fScale = (event->delta() > 0) ? fScaleFactor : (1.0 / fScaleFactor);
#endif

  m_ViewScale *= fScale;
  m_ViewScale.setX(ezMath::Clamp(m_ViewScale.x(), 0.01, 2.0));
  m_ViewScale.setY(ezMath::Clamp(m_ViewScale.y(), 0.01, 2.0));

#if QT_VERSION >= QT_VERSION_CHECK(5, 15, 0)
  QPointF centerB(event->position().x() / m_ViewScale.x(), event->position().y() / m_ViewScale.y());
#else
  QPointF centerB(event->pos().x() / m_ViewScale.x(), event->pos().y() / m_ViewScale.y());
#endif
  m_ViewPos -= (centerB - centerA);

#if QT_VERSION >= QT_VERSION_CHECK(5, 15, 0)
  m_StartDragView = event->position();
#else
  m_vStartDragView = event->pos();
#endif
  m_StartDragScene = m_ViewPos;

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

void ezQtNodeView::drawBackground(QPainter* painter, const QRectF& r)
{
  QGraphicsView::drawBackground(painter, r);

  if (m_ViewScale.manhattanLength() > 1.0)
  {
    QPen pfine(ezToQtColor(ezColorScheme::GetColor(ezColorScheme::Gray, 0)), 1.0);

    painter->setPen(pfine);
    DrawGrid(painter, 15);
  }

  if (m_ViewScale.manhattanLength() > 0.1)
  {
    double scale = m_ViewScale.manhattanLength() < 0.25 ? 150.0 : 300.0;

    QPen p(ezToQtColor(ezColorScheme::GetColor(ezColorScheme::Gray, 1)), 1.0);

    painter->setPen(p);
    DrawGrid(painter, scale);
  }

  // Only force constant redraws when doing the debug animation.
  if (GetScene()->GetConnectionDecorationFlags().IsSet(ezQtNodeScene::ConnectionDecorationFlags::DrawDebugging))
  {
    UpdateView();
  }
}

void ezQtNodeView::UpdateView()
{
  QRectF sceneRect(m_ViewPos.x(), m_ViewPos.y(), width() / m_ViewScale.x(), height() / m_ViewScale.y());
  setSceneRect(sceneRect);
  fitInView(sceneRect, Qt::KeepAspectRatio);
}

void ezQtNodeView::DrawGrid(QPainter* painter, const double gridStep)
{
  const QRectF sceneRect(m_ViewPos.x(), m_ViewPos.y(), width() / m_ViewScale.x(), height() / m_ViewScale.y());
  const QPointF topLeft = sceneRect.topLeft();
  const QPointF bottomRight = sceneRect.bottomRight();

  const double left = ezMath::Floor(topLeft.x() / gridStep - 0.5);
  const double right = ezMath::Floor(bottomRight.x() / gridStep + 1.0);
  const double bottom = ezMath::Floor(topLeft.y() / gridStep - 0.5);
  const double top = ezMath::Floor(bottomRight.y() / gridStep + 1.0);

  // vertical lines
  for (int xi = static_cast<int>(left); xi <= static_cast<int>(right); ++xi)
  {
    QLineF line(xi * gridStep, bottom * gridStep, xi * gridStep, top * gridStep);
    painter->drawLine(line);
  }

  // horizontal lines
  for (int yi = static_cast<int>(bottom); yi <= static_cast<int>(top); ++yi)
  {
    QLineF line(left * gridStep, yi * gridStep, right * gridStep, yi * gridStep);
    painter->drawLine(line);
  }
}
