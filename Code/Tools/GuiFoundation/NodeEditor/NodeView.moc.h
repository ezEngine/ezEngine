#pragma once

#include <GuiFoundation/Basics.h>
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

private:
  ezQtNodeScene* m_pScene;
  bool m_bPanning;
  ezInt32 m_iPanCounter;
  QPoint m_vLastPos;
};
