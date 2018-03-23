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

  void SetData(const ezEventTrackData& data, double fMinCurveLength);
  void SetScrubberPosition(ezUInt64 uiTick);
  void ClearSelection();

  void FrameCurve();

signals:
  void CpMovedEvent(ezUInt32 cpIdx, ezInt64 iTickX, double newPosY);
  void CpDeletedEvent(ezUInt32 cpIdx);
  void InsertCpEvent(ezInt64 tickX, const char* value);

  void BeginCpChangesEvent(QString name);
  void EndCpChangesEvent();

  void BeginOperationEvent(QString name);
  void EndOperationEvent(bool commit);

private slots:
  //void on_LinePosition_editingFinished();
  //void onDeleteControlPoints();
  void onDoubleClick(double scenePosX, double epsilon);
  //void onMoveControlPoints(double x, double y);
  //void onBeginOperation(QString name);
  //void onEndOperation(bool commit);
  //void onScaleControlPoints(QPointF refPt, double scaleX, double scaleY);
  //void onContextMenu(QPoint pos, QPointF scenePos);
  //void onAddPoint();
  //void onSelectionChanged();

private:
  void InsertCpAt(double posX, double epsilon);
  //bool PickControlPointAt(double x, double y, ezVec2d vMaxDistance, ezInt32& out_iCurveIdx, ezInt32& out_iCpIdx) const;
  //void UpdateSpinBoxes();

  const ezEventTrackData* m_pData = nullptr;
  //double m_fCurveDuration;
  //ezVec2d m_ControlPointMove;
  //ezCurveGroupData m_Curves;
  //ezCurveGroupData m_CurvesBackup;
  QPointF m_contextMenuScenePos;
};
