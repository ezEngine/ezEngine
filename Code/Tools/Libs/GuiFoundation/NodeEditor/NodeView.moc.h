#pragma once

#include <GuiFoundation/GuiFoundationDLL.h>
#include <Foundation/Math/Vec2.h>
#include <QGraphicsView>

class ezQtNodeScene;

class EZ_GUIFOUNDATION_DLL ezQtNodeView : public QGraphicsView
{
  Q_OBJECT
public:
  explicit ezQtNodeView(QWidget* parent = nullptr);
  ~ezQtNodeView();

  void SetScene(ezQtNodeScene* pScene);
  ezQtNodeScene* GetScene();

protected:
  virtual void mousePressEvent(QMouseEvent* event) override;
  virtual void mouseMoveEvent(QMouseEvent* event) override;
  virtual void mouseReleaseEvent(QMouseEvent* event) override;
  virtual void wheelEvent(QWheelEvent* event) override;
  virtual void contextMenuEvent(QContextMenuEvent* event) override;
  virtual void resizeEvent(QResizeEvent *) override;

private:
  void UpdateView();

private:
  ezQtNodeScene* m_pScene;
  bool m_bPanning;
  ezInt32 m_iPanCounter;

  QPointF m_ViewPos;
  QPointF m_ViewScale;

  QPoint m_vStartDragView;
  QPointF m_vStartDragScene;
};
