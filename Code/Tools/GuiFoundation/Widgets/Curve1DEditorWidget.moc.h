#pragma once

#include <GuiFoundation/Basics.h>
#include <Foundation/Math/Curve1D.h>
#include <Code/Tools/GuiFoundation/ui_Curve1DEditorWidget.h>

#include <QWidget>

class EZ_GUIFOUNDATION_DLL ezQtCurve1DEditorWidget : public QWidget, public Ui_Curve1DEditorWidget
{
  Q_OBJECT

public:
  explicit ezQtCurve1DEditorWidget(QWidget* pParent);
  ~ezQtCurve1DEditorWidget();

  void SetCurves(ezCurveGroupData& curveData);

  void FrameCurve();
  void MakeRepeatable(bool bAdjustLastPoint);
  void NormalizeCurveX(ezUInt32 uiActiveCurve);
  void NormalizeCurveY(ezUInt32 uiActiveCurve);

signals:
  void CpMovedEvent(ezUInt32 curveIdx, ezUInt32 cpIdx, ezInt64 iTickX, double newPosY);
  void CpDeletedEvent(ezUInt32 curveIdx, ezUInt32 cpIdx);
  void TangentMovedEvent(ezUInt32 curveIdx, ezUInt32 cpIdx, float newPosX, float newPosY, bool rightTangent);
  void InsertCpEvent(ezUInt32 uiCurveIdx, ezInt64 tickX, double value);
  void TangentLinkEvent(ezUInt32 curveIdx, ezUInt32 cpIdx, bool bLink);
  void CpTangentModeEvent(ezUInt32 curveIdx, ezUInt32 cpIdx, bool rightTangent, int mode); // ezCurveTangentMode

  void BeginCpChangesEvent(QString name);
  void EndCpChangesEvent();

  void BeginOperationEvent(QString name);
  void EndOperationEvent(bool commit);

private slots:
  void on_SpinPosition_valueChanged(double value);
  void on_SpinValue_valueChanged(double value);
  void onDeleteControlPoints();
  void onDoubleClick(const QPointF& scenePos, const QPointF& epsilon);
  void onMoveControlPoints(double x, double y);
  void onMoveTangents(float x, float y);
  void onBeginOperation(QString name);
  void onEndOperation(bool commit);
  void onScaleControlPoints(QPointF refPt, double scaleX, double scaleY);
  void onContextMenu(QPoint pos, QPointF scenePos);
  void onAddPoint();
  void onLinkTangents();
  void onBreakTangents();
  void onFlattenTangents();
  void onSelectionChanged();
  void onMoveCurve(ezInt32 iCurve, double moveY);

private:
  void InsertCpAt(double posX, double value, ezVec2d epsilon);
  bool PickCurveAt(double x, double y, double fMaxDistanceY, ezInt32& out_iCurveIdx, double& out_ValueY) const;
  bool PickControlPointAt(double x, double y, ezVec2d vMaxDistance, ezInt32& out_iCurveIdx, ezInt32& out_iCpIdx) const;
  void UpdateSpinBoxes();
  void SetTangentMode(ezCurveTangentMode::Enum mode, bool bLeft, bool bRight);

  ezVec2 m_TangentMove;
  ezVec2d m_ControlPointMove;
  ezCurveGroupData m_Curves;
  ezCurveGroupData m_CurvesBackup;
  QPointF m_contextMenuScenePos;
};
