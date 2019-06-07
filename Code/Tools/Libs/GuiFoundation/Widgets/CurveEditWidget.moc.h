#pragma once

#include <GuiFoundation/GuiFoundationDLL.h>
#include <Foundation/Tracks/Curve1D.h>
#include <Foundation/Math/Vec2.h>
#include <Foundation/Containers/DynamicArray.h>
#include <GuiFoundation/Widgets/CurveEditData.h>

#include <QWidget>
#include <QPen>
#include <QBrush>

class ezQGridBarWidget;
class QRubberBand;

class EZ_GUIFOUNDATION_DLL ezQtCurveEditWidget : public QWidget
{
  Q_OBJECT

public:
  ezQtCurveEditWidget(QWidget* parent);

  void SetCurves(ezCurveGroupData* pCurveEditData, double fMinCurveLength, bool bCurveLengthIsFixed);
  void SetGridBarWidget(ezQGridBarWidget* pGridBar) { m_pGridBar = pGridBar; }

  void SetScrubberPosition(double fPosition);
  double GetMaxCurveExtent() const { return m_fMaxCurveExtent; }

  void FrameCurve();

  QPoint MapFromScene(const QPointF& pos) const;
  QPoint MapFromScene(const ezVec2d& pos) const { return MapFromScene(QPointF(pos.x, pos.y)); }
  QPointF MapToScene(const QPoint& pos) const;
  ezVec2 MapDirFromScene(const ezVec2& pos) const;

  void ClearSelection();
  const ezDynamicArray<ezSelectedCurveCP>& GetSelection() const { return m_SelectedCPs; }
  bool IsSelected(const ezSelectedCurveCP& cp) const;
  void SetSelection(const ezSelectedCurveCP& cp);
  void ToggleSelected(const ezSelectedCurveCP& cp);
  void SetSelected(const ezSelectedCurveCP& cp, bool set);

  bool GetSelectedTangent(ezInt32& out_iCurve, ezInt32& out_iPoint, bool& out_bLeftTangent) const;

Q_SIGNALS:
  void DoubleClickEvent(const QPointF& scenePos, const QPointF& epsilon);
  void DeleteControlPointsEvent();
  void MoveControlPointsEvent(double moveX, double moveY);
  void MoveTangentsEvent(double moveX, double moveY);
  void BeginOperationEvent(QString name);
  void EndOperationEvent(bool bCommit);
  void ScaleControlPointsEvent(const QPointF& centerPos, double scaleX, double scaleY);
  void ContextMenuEvent(QPoint pos, QPointF scenePos);
  void SelectionChangedEvent();
  void MoveCurveEvent(ezInt32 iCurve, double moveY);

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
  enum class EditState {
    None, DraggingPoints, DraggingPointsHorz, DraggingPointsVert, DraggingTangents, MultiSelect, RightClick, Panning, ScaleLeftRight, ScaleUpDown, DraggingCurve };
  enum class SelectArea { None, Center, Top, Bottom, Left, Right };

  void PaintCurveSegments(QPainter* painter, float fOffsetX, ezUInt8 alpha) const;
  void PaintOutsideAreaOverlay(QPainter* painter) const;
  void PaintControlPoints(QPainter* painter) const;
  void PaintSelectedControlPoints(QPainter* painter) const;
  void PaintSelectedTangentLines(QPainter* painter) const;
  void PaintSelectedTangentHandles(QPainter* painter) const;
  void PaintMultiSelectionSquare(QPainter* painter) const;
  void PaintScrubber(QPainter& p) const;
  void RenderVerticalGrid(QPainter* painter, const QRectF& viewportSceneRect, double fRoughGridDensity);
  void RenderSideLinesAndText(QPainter* painter, const QRectF& viewportSceneRect);
  QRectF ComputeViewportSceneRect() const;
  bool PickCpAt(const QPoint& pos, float fMaxPixelDistance, ezSelectedCurveCP& out_Result) const;
  ClickTarget DetectClickTarget(const QPoint& pos);
  void ExecMultiSelection(ezDynamicArray<ezSelectedCurveCP>& out_Selection);
  bool CombineSelection(ezDynamicArray<ezSelectedCurveCP>& inout_Selection, const ezDynamicArray<ezSelectedCurveCP>& change, bool add);
  void ComputeSelectionRect();
  SelectArea WhereIsPoint(QPoint pos) const;
  ezInt32 PickCurveAt(QPoint pos) const;
  void ClampZoomPan();

  ezQGridBarWidget* m_pGridBar = nullptr;

  EditState m_State = EditState::None;
  ezInt32 m_iDraggedCurve;

  ezCurveGroupData* m_pCurveEditData;
  ezHybridArray<ezCurve1D, 4> m_Curves;
  ezHybridArray<ezCurve1D, 4> m_CurvesSorted;
  ezHybridArray<ezVec2d, 4> m_CurveExtents;
  double m_fMaxCurveExtent;
  double m_fMinValue, m_fMaxValue;

  QPointF m_SceneTranslation;
  QPointF m_SceneToPixelScale;
  QPoint m_LastMousePos;

  QBrush m_ControlPointBrush;
  QBrush m_SelectedControlPointBrush;
  QPen m_TangentLinePen;
  QBrush m_TangentHandleBrush;

  ezDynamicArray<ezSelectedCurveCP> m_SelectedCPs;
  ezInt32 m_iSelectedTangentCurve = -1;
  ezInt32 m_iSelectedTangentPoint = -1;
  bool m_bSelectedTangentLeft = false;
  bool m_bBegunChanges = false;
  bool m_bFrameBeforePaint = true;

  QPoint m_multiSelectionStart;
  QRect m_multiSelectRect;
  QRectF m_selectionBRect;
  QPointF m_scaleReferencePoint;
  QPointF m_scaleStartPoint;
  QPointF m_totalPointDrag;
  QRubberBand* m_pRubberband = nullptr;

  bool m_bShowScrubber = false;
  double m_fScrubberPosition = 0;
};
