#pragma once

#include <GuiFoundation/Basics.h>
#include <Foundation/Math/Curve1D.h>
#include <GuiFoundation/Widgets/GraphicsView.moc.h>
#include <Foundation/Math/Vec2.h>
#include <Foundation/Containers/DynamicArray.h>

#include <QWidget>
#include <QPen>
#include <QBrush>

struct ezSelectedCurveCP
{
  EZ_DECLARE_POD_TYPE();

  ezUInt16 m_uiCurve;
  ezUInt16 m_uiPoint;
};

class ezQGridBarWidget;
class QRubberBand;

class EZ_GUIFOUNDATION_DLL ezQtCurveEditWidget : public QWidget
{
  Q_OBJECT

public:
  ezQtCurveEditWidget(QWidget* parent);

  void SetCurves(const ezArrayPtr<ezCurve1D>& curves);
  void SetGridBarWidget(ezQGridBarWidget* pGridBar) { m_pGridBar = pGridBar; }

  QPoint MapFromScene(const QPointF& pos) const;
  QPoint MapFromScene(const ezVec2& pos) const { return MapFromScene(QPointF(pos.x, pos.y)); }
  QPointF MapToScene(const QPoint& pos) const;

  void ClearSelection();
  const ezDynamicArray<ezSelectedCurveCP>& GetSelection() const { return m_SelectedCPs; }
  bool IsSelected(const ezSelectedCurveCP& cp) const;
  void SetSelection(const ezSelectedCurveCP& cp);
  void ToggleSelected(const ezSelectedCurveCP& cp);
  void SetSelected(const ezSelectedCurveCP& cp, bool set);

  bool GetSelectedTangent(ezInt32& out_iCurve, ezInt32& out_iPoint, bool& out_bLeftTangent) const;

signals:
  void DoubleClickEvent(const QPointF& scenePos, const QPointF& epsilon);
  void DeleteControlPointsEvent();
  void MoveControlPointsEvent(double moveX, double moveY);
  void MoveTangentsEvent(double moveX, double moveY);
  void BeginOperation(QString name);
  void EndOperation(bool bCommit);

protected:
  virtual void paintEvent(QPaintEvent* e) override;
  virtual void mousePressEvent(QMouseEvent* e) override;
  virtual void mouseReleaseEvent(QMouseEvent* e) override;
  virtual void mouseMoveEvent(QMouseEvent* e) override;
  virtual void mouseDoubleClickEvent(QMouseEvent* e) override;
  virtual void wheelEvent(QWheelEvent* e) override;
  virtual void keyPressEvent(QKeyEvent* e) override;

private:
  enum class ClickTarget { Nothing, SelectedPoint, TangentHandle };
  enum class EditState { None, DraggingPoints, DraggingTangents, MultiSelect, Panning };

  void PaintCurveSegments(QPainter* painter) const;
  void PaintControlPoints(QPainter* painter) const;
  void PaintSelectedControlPoints(QPainter* painter) const;
  void PaintSelectedTangentLines(QPainter* painter) const;
  void PaintSelectedTangentHandles(QPainter* painter) const;
  void RenderVerticalGrid(QPainter* painter, const QRectF& viewportSceneRect, double fRoughGridDensity);
  void RenderSideLinesAndText(QPainter* painter, const QRectF& viewportSceneRect);
  QRectF ComputeViewportSceneRect() const;
  bool PickCpAt(const QPoint& pos, float fMaxPixelDistance, ezSelectedCurveCP& out_Result) const;
  ClickTarget DetectClickTarget(const QPoint& pos);
  void ExecMultiSelection(ezDynamicArray<ezSelectedCurveCP>& out_Selection);
  bool CombineSelection(ezDynamicArray<ezSelectedCurveCP>& inout_Selection, const ezDynamicArray<ezSelectedCurveCP>& change, bool add);

  ezQGridBarWidget* m_pGridBar = nullptr;

  EditState m_State = EditState::None;

  ezHybridArray<ezCurve1D, 4> m_Curves;
  ezHybridArray<ezCurve1D, 4> m_CurvesSorted;
  ezHybridArray<ezVec2, 4> m_CurveExtents;

  QPointF m_SceneTranslation;
  QPointF m_SceneToPixelScale;
  QPoint m_LastMousePos;

  QPen m_ControlPointPen;
  QBrush m_ControlPointBrush;
  QPen m_SelectedControlPointPen;
  QBrush m_SelectedControlPointBrush;
  QPen m_TangentLinePen;
  QPen m_TangentHandlePen;
  QBrush m_TangentHandleBrush;

  ezDynamicArray<ezSelectedCurveCP> m_SelectedCPs;
  ezInt32 m_iSelectedTangentCurve = -1;
  ezInt32 m_iSelectedTangentPoint = -1;
  bool m_bSelectedTangentLeft = false;
  bool m_bBegunChanges = false;

  QPoint m_multiSelectionStart;
  QRect m_multiSelectRect;
  QRubberBand* m_pRubberband = nullptr;
};
