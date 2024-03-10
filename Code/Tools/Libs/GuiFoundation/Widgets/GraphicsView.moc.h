#pragma once

#include <Foundation/Types/Delegate.h>
#include <GuiFoundation/GuiFoundationDLL.h>
#include <QGraphicsView>

class QWheelEvent;
class QMouseEvent;
class QKeyEvent;
class ezQGridBarWidget;

class EZ_GUIFOUNDATION_DLL ezQtGraphicsView : public QGraphicsView
{
  Q_OBJECT

public:
  ezQtGraphicsView(QWidget* pParent = nullptr);

  virtual void wheelEvent(QWheelEvent* e) override;
  virtual void mousePressEvent(QMouseEvent* e) override;
  virtual void mouseReleaseEvent(QMouseEvent* e) override;
  virtual void mouseMoveEvent(QMouseEvent* e) override;
  virtual void keyPressEvent(QKeyEvent* e) override;

  void SetZoom(float fZoom);
  float GetZoom() const { return m_fZoom; }

  void SetZoomLimits(float fMinZoom, float fMaxZoom);

Q_SIGNALS:
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
  QPoint m_LastGlobalMouseMovePos;
};
