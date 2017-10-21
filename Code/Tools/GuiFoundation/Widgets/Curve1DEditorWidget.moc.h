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

  void SetCurves(ezCurve1DAssetData& curveData);

  void FrameCurve();

signals:
  void CpMovedEvent(ezUInt32 curveIdx, ezUInt32 cpIdx, float newPosX, float newPosY);
  void CpDeletedEvent(ezUInt32 curveIdx, ezUInt32 cpIdx);
  void TangentMovedEvent(ezUInt32 curveIdx, ezUInt32 cpIdx, float newPosX, float newPosY, bool rightTangent);
  void InsertCpEvent(ezUInt32 uiCurveIdx, float posX, float value);
  void TangentLinkEvent(ezUInt32 curveIdx, ezUInt32 cpIdx, bool bLink);

  void BeginCpChangesEvent(QString name);
  void EndCpChangesEvent();

  void BeginOperationEvent(QString name);
  void EndOperationEvent(bool commit);

private slots:
  void on_ButtonFrame_clicked();
  void on_SpinPosition_valueChanged(double value);
  void on_ButtonNormalizeX_clicked();
  void on_ButtonNormalizeY_clicked();
  void onDeleteControlPoints();
  void onDoubleClick(const QPointF& scenePos, const QPointF& epsilon);
  void onMoveControlPoints(double x, double y);
  void onMoveTangents(double x, double y);
  void onBeginOperation(QString name);
  void onScaleControlPoints(QPointF refPt, double scaleX, double scaleY);
  void onContextMenu(QPoint pos, QPointF scenePos);
  void onAddPoint();
  void onLinkTangents();
  void onBreakTangents();
  void onFlattenTangents();

private:
  void NormalizeCurveX(ezUInt32 uiActiveCurve);
  void NormalizeCurveY(ezUInt32 uiActiveCurve);
  void InsertCpAt(float posX, float value, float epsilon);
  bool PickCurveAt(float x, float y, float fMaxYDistance, ezInt32& out_iCurveIdx, float& out_ValueY) const;
  bool PickControlPointAt(float x, float y, float fMaxDistance, ezInt32& out_iCurveIdx, ezInt32& out_iCpIdx) const;

  ezVec2 m_TangentMove;
  ezVec2 m_ControlPointMove;
  ezCurve1DAssetData m_Curves;
  ezCurve1DAssetData m_CurvesBackup;
  QPointF m_contextMenuScenePos;
};
