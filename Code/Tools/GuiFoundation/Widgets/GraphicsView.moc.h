#pragma once

#include <GuiFoundation/Basics.h>
#include <QGraphicsView>
#include <Foundation/Types/Delegate.h>

class QWheelEvent;
class QMouseEvent;
class QKeyEvent;
class ezQGridBarWidget;

class EZ_GUIFOUNDATION_DLL ezQtGraphicsView : public QGraphicsView
{
  Q_OBJECT

public:
  ezQtGraphicsView(QWidget* parent = nullptr);

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

class EZ_GUIFOUNDATION_DLL ezQCurveView : public ezQtGraphicsView
{
  Q_OBJECT

public:
  ezQCurveView(QWidget* parent);
  void SetGridBarWidget(ezQGridBarWidget* pGridBar) { m_pGridBar = pGridBar; }

protected:
  virtual void drawBackground(QPainter *painter, const QRectF &rect) override;
  virtual void mouseDoubleClickEvent(QMouseEvent *event) override;

  void RenderVerticalGrid(QPainter* painter, const QRectF& viewportSceneRect, double fRoughGridDensity);
  void RenderSideLinesAndText(QPainter* painter, const QRectF& viewportSceneRect);

  ezQGridBarWidget* m_pGridBar = nullptr;
};

class EZ_GUIFOUNDATION_DLL ezQGridBarWidget : public QWidget
{
  Q_OBJECT

public:
  ezQGridBarWidget(QWidget* parent);

  void SetConfig(const QRectF& viewportSceneRect, double fTextGridStops, double fFineGridStops, ezDelegate<QPoint (const QPointF&)> mapFromSceneFunc);

protected:
  virtual void paintEvent(QPaintEvent *event) override;

private:
  QRectF m_viewportSceneRect;
  double m_fTextGridStops;
  double m_fFineGridStops;
  ezDelegate<QPoint(const QPointF&)> MapFromSceneFunc;
};
