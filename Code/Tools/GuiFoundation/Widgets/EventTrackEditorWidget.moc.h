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
  void CpMovedEvent(ezUInt32 cpIdx, ezInt64 iTickX);
  void CpDeletedEvent(ezUInt32 cpIdx);
  void InsertCpEvent(ezInt64 tickX, const char* value);

  void BeginCpChangesEvent(QString name);
  void EndCpChangesEvent();

  void BeginOperationEvent(QString name);
  void EndOperationEvent(bool commit);

private slots:
  void on_LinePosition_editingFinished();
  void on_AddEventButton_clicked();
  void onDeleteControlPoints();
  void onDoubleClick(double scenePosX, double epsilon);
  void onMoveControlPoints(double x);
  void onBeginOperation(QString name);
  void onEndOperation(bool commit);
  //void onScaleControlPoints(QPointF refPt, double scaleX);
  void onContextMenu(QPoint pos, QPointF scenePos);
  void onAddPoint();
  void onSelectionChanged();

private:
  void InsertCpAt(double posX, double epsilon);
  void UpdateSpinBoxes();
  void DetermineAvailableEvents();
  void FillEventComboBox(const char* szCurrent = nullptr);

  const ezEventTrackData* m_pData = nullptr;
  ezEventTrackData m_DataCopy;

  double m_ControlPointMove;
  QPointF m_contextMenuScenePos;
  ezEventSet m_EventSet;
};
