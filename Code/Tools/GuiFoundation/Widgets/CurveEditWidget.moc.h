#pragma once

#include <GuiFoundation/Basics.h>
#include <Foundation/Math/Curve1D.h>
#include <GuiFoundation/Widgets/GraphicsView.moc.h>

#include <QWidget>
#include <QPen>
#include <QBrush>

class EZ_GUIFOUNDATION_DLL ezQtCurveEditWidget : public QWidget
{
  Q_OBJECT

public:
  ezQtCurveEditWidget(QWidget* parent);

  void SetCurves(const ezArrayPtr<ezCurve1D>& curves);

  QPoint MapFromScene(const QPointF& pos) const;
  QPointF MapToScene(const QPoint& pos) const;

protected:
  virtual void paintEvent(QPaintEvent* e) override;
  virtual void mousePressEvent(QMouseEvent *event) override;
  virtual void mouseReleaseEvent(QMouseEvent *event) override;
  virtual void mouseMoveEvent(QMouseEvent *event) override;
  virtual void mouseDoubleClickEvent(QMouseEvent *event) override;
  virtual void wheelEvent(QWheelEvent *event) override;

private:
  void PaintCurveSegments(QPainter* painter) const;
  void PaintControlPoints(QPainter* painter) const;
  void RenderVerticalGrid(QPainter* painter, const QRectF& viewportSceneRect, double fRoughGridDensity);
  void RenderSideLinesAndText(QPainter* painter, const QRectF& viewportSceneRect);
  QRectF ComputeViewportSceneRect() const;

  ezQGridBarWidget* m_pGridBar = nullptr;

  ezHybridArray<ezCurve1D, 4> m_Curves;
  ezHybridArray<ezCurve1D, 4> m_CurvesSorted;

  QPointF m_SceneTranslation;
  QPointF m_SceneToPixelScale;
  QPoint m_LastMousePos;

  QPen m_ControlPointPen;
  QBrush m_ControlPointBrush;
};
