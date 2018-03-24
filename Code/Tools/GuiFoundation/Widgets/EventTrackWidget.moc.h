#pragma once

#include <GuiFoundation/Basics.h>
#include <Foundation/Containers/DynamicArray.h>
#include <GuiFoundation/Widgets/EventTrackEditData.h>
#include <Foundation/Math/Vec2.h>

#include <QWidget>
#include <QPen>
#include <QBrush>

class ezQGridBarWidget;
class QRubberBand;

class EZ_GUIFOUNDATION_DLL ezQtEventTrackWidget : public QWidget
{
  Q_OBJECT

public:
  ezQtEventTrackWidget(QWidget* parent);

  void SetData(const ezEventTrackData* pData, double fMinCurveLength);
  void SetGridBarWidget(ezQGridBarWidget* pGridBar) { m_pGridBar = pGridBar; }

  void SetScrubberPosition(double fPosition);

  void FrameCurve();

  QPoint MapFromScene(const QPointF& pos) const;
  //QPoint MapFromScene(const ezVec2d& pos) const { return MapFromScene(QPointF(pos.x, pos.y)); }
  QPointF MapToScene(const QPoint& pos) const;

  void ClearSelection();

signals:
  void DoubleClickEvent(double scenePosX, double epsilon);
  void DeleteControlPointsEvent();
  //void MoveControlPointsEvent(double moveX, double moveY);
  void BeginOperationEvent(QString name);
  void EndOperationEvent(bool bCommit);
  //void ScaleControlPointsEvent(const QPointF& centerPos, double scaleX, double scaleY);
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
  enum class ClickTarget { Nothing, SelectedPoint };
  enum class EditState { None, DraggingPoints, MultiSelect, RightClick, Panning, ScaleLeftRight };

  struct Point
  {
    EZ_DECLARE_POD_TYPE();

    ezUInt32 m_uiOrgIndex;
    bool m_bSelected;
    double m_fPosX;
  };

  struct SelectedPoint
  {
    EZ_DECLARE_POD_TYPE();

    ezUInt32 m_uiCategory;
    ezUInt32 m_uiSortedIdx;
  };

  struct PointCategory
  {
    ezHashedString m_sName;
    ezHybridArray<Point, 32> m_Points;
  };

  //const ezDynamicArray<ezUInt32>& GetSelection() const { return m_SelectedCPs; }
  //bool IsSelected(ezUInt32 cp) const;
  void SetSelection(SelectedPoint cp);
  //void ToggleSelected(ezUInt32 cp);
  //void SetSelected(ezUInt32 cp, bool set);

  void PaintOutsideAreaOverlay(QPainter* painter) const;
  void PaintControlPoints(QPainter* painter) const;
  void PaintMultiSelectionSquare(QPainter* painter) const;
  void PaintScrubber(QPainter& p) const;
  void RenderVerticalGrid(QPainter* painter, const QRectF& viewportSceneRect, double fRoughGridDensity);
  QRectF ComputeViewportSceneRect() const;
  bool PickCpAt(const QPoint& pos, float fMaxPixelDistance, SelectedPoint& out_Result) const;
  ClickTarget DetectClickTarget(const QPoint& pos);
  //void ExecMultiSelection(ezDynamicArray<ezSelectedCurveCP>& out_Selection);
  //bool CombineSelection(ezDynamicArray<ezSelectedCurveCP>& inout_Selection, const ezDynamicArray<ezSelectedCurveCP>& change, bool add);
  //void ComputeSelectionRect();
  //SelectArea WhereIsPoint(QPoint pos) const;
  void ClampZoomPan();
  void RecreateSortedData();

  ezQGridBarWidget* m_pGridBar = nullptr;

  EditState m_State = EditState::None;

  const ezEventTrackData* m_pEditData = nullptr;

  double m_fMaxCurveExtent = 0;
  double m_fSceneTranslationX = 0;
  QPointF m_SceneToPixelScale;
  QPoint m_LastMousePos;

  QBrush m_ControlPointBrush;
  QBrush m_SelectedControlPointBrush;
  QPen m_ControlPointPen;

  //ezDynamicArray<ezUInt32> m_SelectedCPs;
  bool m_bBegunChanges = false;
  bool m_bFrameBeforePaint = true;

  QPoint m_multiSelectionStart;
  QRect m_multiSelectRect;
  //QRectF m_selectionBRect;
  //QPointF m_scaleReferencePoint;
  //QPointF m_scaleStartPoint;
  //QPointF m_totalPointDrag;
  QRubberBand* m_pRubberband = nullptr;

  bool m_bShowScrubber = false;
  double m_fScrubberPosition = 0;

  ezHashTable<ezHashedString, ezUInt32> m_NameToCategory;
  ezHybridArray<PointCategory, 8> m_Categories;
  ezHybridArray<SelectedPoint, 32> m_SelectedPoints;
};
