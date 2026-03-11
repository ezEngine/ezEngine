#pragma once

#include <Foundation/Math/Vec2.h>
#include <GuiFoundation/GuiFoundationDLL.h>
#include <QGraphicsView>

class ezQtVisualGraphScene;

/// Qt view widget for displaying a visual graph scene with panning and zooming support.
///
/// Provides viewport controls for navigating the graph, including mouse-based panning and zooming.
/// Renders a grid background and handles the display of the graph scene.
class EZ_GUIFOUNDATION_DLL ezQtVisualGraphView : public QGraphicsView
{
  Q_OBJECT
public:
  explicit ezQtVisualGraphView(QWidget* pParent = nullptr);
  ~ezQtVisualGraphView();

  void SetScene(ezQtVisualGraphScene* pScene);
  ezQtVisualGraphScene* GetScene();

protected:
  virtual void mousePressEvent(QMouseEvent* event) override;
  virtual void mouseMoveEvent(QMouseEvent* event) override;
  virtual void mouseReleaseEvent(QMouseEvent* event) override;
  virtual void wheelEvent(QWheelEvent* event) override;
  virtual void contextMenuEvent(QContextMenuEvent* event) override;
  virtual void resizeEvent(QResizeEvent*) override;
  virtual void drawBackground(QPainter* painter, const QRectF& r) override;


private:
  void UpdateView();

  void DrawGrid(QPainter* painter, const double gridStep);

private:
  ezQtVisualGraphScene* m_pScene = nullptr;
  bool m_bPanning = false;
  ezInt32 m_iPanCounter = 0;

  QPointF m_ViewPos;
  QPointF m_ViewScale;

#if QT_VERSION >= QT_VERSION_CHECK(5, 15, 0)
  QPointF m_StartDragView;
#else
  QPoint m_vStartDragView;
#endif

  QPointF m_StartDragScene;
};
