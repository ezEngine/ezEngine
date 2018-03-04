#pragma once

#include <GuiFoundation/Basics.h>
#include <Code/Tools/GuiFoundation/ui_EventTrackEditorWidget.h>

#include <QWidget>

class EZ_GUIFOUNDATION_DLL ezQtEventTrackEditorWidget : public QWidget, public Ui_EventTrackEditorWidget
{
  Q_OBJECT

public:
  explicit ezQtEventTrackEditorWidget(QWidget* pParent);
  ~ezQtEventTrackEditorWidget();

  //void SetCurves(ezCurveGroupData& curveData, double fMinCurveLength, bool bCurveLengthIsFixed);
  void SetScrubberPosition(ezUInt64 uiTick);
  void ClearSelection();

  void FrameCurve();

signals:
  void CpMovedEvent(ezUInt32 curveIdx, ezUInt32 cpIdx, ezInt64 iTickX, double newPosY);
  void CpDeletedEvent(ezUInt32 curveIdx, ezUInt32 cpIdx);
  void InsertCpEvent(ezUInt32 uiCurveIdx, ezInt64 tickX, double value);

  void BeginCpChangesEvent(QString name);
  void EndCpChangesEvent();

  void BeginOperationEvent(QString name);
  void EndOperationEvent(bool commit);

private slots:
  //void on_LinePosition_editingFinished();
  //void onDeleteControlPoints();
  //void onDoubleClick(const QPointF& scenePos, const QPointF& epsilon);
  //void onMoveControlPoints(double x, double y);
  //void onBeginOperation(QString name);
  //void onEndOperation(bool commit);
  //void onScaleControlPoints(QPointF refPt, double scaleX, double scaleY);
  //void onContextMenu(QPoint pos, QPointF scenePos);
  //void onAddPoint();
  //void onSelectionChanged();

private:
  //void InsertCpAt(double posX, double value, ezVec2d epsilon);
  //bool PickControlPointAt(double x, double y, ezVec2d vMaxDistance, ezInt32& out_iCurveIdx, ezInt32& out_iCpIdx) const;
  //void UpdateSpinBoxes();

  //double m_fCurveDuration;
  //ezVec2d m_ControlPointMove;
  //ezCurveGroupData m_Curves;
  //ezCurveGroupData m_CurvesBackup;
  QPointF m_contextMenuScenePos;
};
