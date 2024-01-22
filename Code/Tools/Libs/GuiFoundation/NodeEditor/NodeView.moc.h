#pragma once

#include <Foundation/Math/Vec2.h>
#include <GuiFoundation/GuiFoundationDLL.h>
#include <QGraphicsView>

class ezQtNodeScene;

class EZ_GUIFOUNDATION_DLL ezQtNodeView : public QGraphicsView
{
  Q_OBJECT
public:
  explicit ezQtNodeView(QWidget* pParent = nullptr);
  ~ezQtNodeView();

  void SetScene(ezQtNodeScene* pScene);
  ezQtNodeScene* GetScene();

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
  ezQtNodeScene* m_pScene = nullptr;
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
