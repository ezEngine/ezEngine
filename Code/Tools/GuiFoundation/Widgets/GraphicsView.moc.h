#pragma once

#include <GuiFoundation/Basics.h>
#include <QGraphicsView>

class QWheelEvent;
class QMouseEvent;
class QKeyEvent;

class EZ_GUIFOUNDATION_DLL ezQGraphicsView : public QGraphicsView
{
  Q_OBJECT

public:
  ezQGraphicsView(QWidget* parent = nullptr);

  virtual void wheelEvent(QWheelEvent* e) override;
  virtual void mousePressEvent(QMouseEvent* e) override;
  virtual void mouseReleaseEvent(QMouseEvent* e) override;
  virtual void mouseMoveEvent(QMouseEvent* e) override;
  virtual void keyPressEvent(QKeyEvent* e) override;

  void SetZoom(float zoom);
  float GetZoom() const { return m_fZoom; }

  void SetZoomLimits(float minZoom, float maxZoom);

signals:
  void BeginDrag();
  void EndDrag();
  void DeleteCPs();

protected:
  void UpdateTransform();

  bool m_bPanning;
  bool m_bForwardMouseEvents;
  bool m_bDragging;

  float m_fMinZoom, m_fMaxZoom;
  float m_fZoom;
  QPoint m_lastGlobalMouseMovePos;
};