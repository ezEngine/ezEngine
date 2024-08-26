#pragma once

#include <GuiFoundation/GuiFoundationDLL.h>
#include <GuiFoundation/ui_EventTrackEditorWidget.h>

#include <QWidget>

class EZ_GUIFOUNDATION_DLL ezQtEventTrackEditorWidget : public QWidget, public Ui_EventTrackEditorWidget
{
  Q_OBJECT

public:
  explicit ezQtEventTrackEditorWidget(QWidget* pParent);
  ~ezQtEventTrackEditorWidget();

  void SetData(const ezEventTrackData& data, double fMinCurveLength);
  void SetScrubberPosition(ezUInt64 uiTick);
  void SetScrubberPosition(ezTime time);
  void ClearSelection();

  void FrameCurve();

Q_SIGNALS:
  void CpMovedEvent(ezUInt32 uiIdx, ezInt64 iTickX);
  void CpDeletedEvent(ezUInt32 uiIdx);
  void InsertCpEvent(ezInt64 iTickX, const char* value);

  void BeginCpChangesEvent(QString sName);
  void EndCpChangesEvent();

  void BeginOperationEvent(QString sName);
  void EndOperationEvent(bool bCommit);

private Q_SLOTS:
  void on_LinePosition_editingFinished();
  void on_AddEventButton_clicked();
  void on_InsertEventButton_clicked();
  void onDeleteControlPoints();
  void onDoubleClick(double scenePosX, double epsilon);
  void onMoveControlPoints(double x);
  void onBeginOperation(QString name);
  void onEndOperation(bool commit);
  // void onScaleControlPoints(QPointF refPt, double scaleX);
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

  double m_fControlPointMove;
  QPointF m_ContextMenuScenePos;
  ezEventSet m_EventSet;
};
