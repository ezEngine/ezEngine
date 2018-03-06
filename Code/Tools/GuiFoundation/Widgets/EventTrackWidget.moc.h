#pragma once

#include <GuiFoundation/Basics.h>
#include <Foundation/Containers/DynamicArray.h>
//#include <GuiFoundation/Widgets/CurveEditData.h>
#include <Foundation/Math/Vec2.h>

#include <QWidget>
#include <QPen>
#include <QBrush>

class ezQGridBarWidget;
class QRubberBand;

class EZ_GUIFOUNDATION_DLL ezQEventTrackWidget : public QWidget
{
  Q_OBJECT

public:
  ezQEventTrackWidget(QWidget* parent);

  //void SetCurves(ezCurveGroupData* pCurveEditData, double fMinCurveLength, bool bCurveLengthIsFixed);
  void SetGridBarWidget(ezQGridBarWidget* pGridBar) { m_pGridBar = pGridBar; }

  void SetScrubberPosition(double fPosition);
  //double GetMaxCurveExtent() const { return m_fMaxCurveExtent; }

  void FrameCurve();

  QPoint MapFromScene(const QPointF& pos) const;
  //QPoint MapFromScene(const ezVec2d& pos) const { return MapFromScene(QPointF(pos.x, pos.y)); }
  QPointF MapToScene(const QPoint& pos) const;
  ezVec2 MapDirFromScene(const ezVec2& pos) const;

  void ClearSelection();
  //const ezDynamicArray<ezSelectedCurveCP>& GetSelection() const { return m_SelectedCPs; }
  //bool IsSelected(const ezSelectedCurveCP& cp) const;
  //void SetSelection(const ezSelectedCurveCP& cp);
  //void ToggleSelected(const ezSelectedCurveCP& cp);
  //void SetSelected(const ezSelectedCurveCP& cp, bool set);

signals:
  void DoubleClickEvent(const QPointF& scenePos, const QPointF& epsilon);
  void DeleteControlPointsEvent();
  void MoveControlPointsEvent(double moveX, double moveY);
  void BeginOperationEvent(QString name);
  void EndOperationEvent(bool bCommit);
  void ScaleControlPointsEvent(const QPointF& centerPos, double scaleX, double scaleY);
  void ContextMenuEvent(QPoint pos, QPointF scenePos);
  void SelectionChangedEvent();

protected:
  virtual void paintEvent(QPaintEvent* e) override;
  virtual void mousePressEvent(QMouseEvent* e) override;
  virtual void mouseReleaseEvent(QMouseEvent* e) override;
  virtual void mouseMoveEvent(QMouseEvent* e) override;
  virtual void mouseDoubleClickEvent(QMouseEvent* e) override;
  virtual void wheelEvent(QWheelEvent* e) override;
  virtual void keyPressEvent(QKeyEvent* e) override;

private:
  //enum class ClickTarget { Nothing, SelectedPoint, TangentHandle };
  enum class EditState { None, DraggingPoints, DraggingPointsHorz, DraggingPointsVert, DraggingTangents, MultiSelect, RightClick, Panning, ScaleLeftRight, ScaleUpDown, DraggingCurve };

  void PaintOutsideAreaOverlay(QPainter* painter) const;
  void PaintControlPoints(QPainter* painter) const;
  void PaintSelectedControlPoints(QPainter* painter) const;
  void PaintMultiSelectionSquare(QPainter* painter) const;
  void PaintScrubber(QPainter& p) const;
  void RenderVerticalGrid(QPainter* painter, const QRectF& viewportSceneRect, double fRoughGridDensity);
  void RenderSideLinesAndText(QPainter* painter, const QRectF& viewportSceneRect);
  QRectF ComputeViewportSceneRect() const;
  //bool PickCpAt(const QPoint& pos, float fMaxPixelDistance, ezSelectedCurveCP& out_Result) const;
  //ClickTarget DetectClickTarget(const QPoint& pos);
  //void ExecMultiSelection(ezDynamicArray<ezSelectedCurveCP>& out_Selection);
  //bool CombineSelection(ezDynamicArray<ezSelectedCurveCP>& inout_Selection, const ezDynamicArray<ezSelectedCurveCP>& change, bool add);
  //void ComputeSelectionRect();
  //SelectArea WhereIsPoint(QPoint pos) const;
  void ClampZoomPan();

  ezQGridBarWidget* m_pGridBar = nullptr;

  EditState m_State = EditState::None;

  //ezCurveGroupData* m_pCurveEditData;
  //ezHybridArray<ezVec2d, 4> m_CurveExtents;
  //double m_fMaxCurveExtent;

  QPointF m_SceneTranslation;
  QPointF m_SceneToPixelScale;
  QPoint m_LastMousePos;

  QBrush m_ControlPointBrush;
  QBrush m_SelectedControlPointBrush;

  //ezDynamicArray<ezSelectedCurveCP> m_SelectedCPs;
  bool m_bBegunChanges = false;
  bool m_bFrameBeforePaint = true;

  //QPoint m_multiSelectionStart;
  //QRect m_multiSelectRect;
  //QRectF m_selectionBRect;
  //QPointF m_scaleReferencePoint;
  //QPointF m_scaleStartPoint;
  //QPointF m_totalPointDrag;
  //QRubberBand* m_pRubberband = nullptr;

  bool m_bShowScrubber = false;
  double m_fScrubberPosition = 0;
};
