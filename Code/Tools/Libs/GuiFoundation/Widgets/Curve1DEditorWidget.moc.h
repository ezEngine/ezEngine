#pragma once

#include <Foundation/Math/CurveFunctions.h>
#include <Foundation/Tracks/Curve1D.h>
#include <GuiFoundation/GuiFoundationDLL.h>
#include <GuiFoundation/ui_Curve1DEditorWidget.h>

#include <QWidget>

class EZ_GUIFOUNDATION_DLL ezQtCurve1DEditorWidget : public QWidget, public Ui_Curve1DEditorWidget
{
  Q_OBJECT

public:
  explicit ezQtCurve1DEditorWidget(QWidget* pParent);
  ~ezQtCurve1DEditorWidget();

  void SetCurveExtents(double fLowerBound, double fUpperBound, bool bLowerIsFixed, bool bUpperIsFixed);
  void SetCurveRanges(double fLowerRange, double fUpperRange);

  void SetCurves(const ezCurveGroupData& curveData);
  void SetScrubberPosition(ezUInt64 uiTick);
  void ClearSelection();

  void FrameCurve();
  void FrameSelection();
  void MakeRepeatable(bool bAdjustLastPoint);
  void NormalizeCurveX(ezUInt32 uiActiveCurve);
  void NormalizeCurveY(ezUInt32 uiActiveCurve);
  void ClearAllPoints();
  void MirrorHorizontally(ezUInt32 uiActiveCurve);
  void MirrorVertically(ezUInt32 uiActiveCurve);

Q_SIGNALS:
  void CpMovedEvent(ezUInt32 uiCurveIdx, ezUInt32 uiIdx, ezInt64 iTickX, double fNewPosY);
  void CpDeletedEvent(ezUInt32 uiCurveIdx, ezUInt32 uiIdx);
  void TangentMovedEvent(ezUInt32 uiCurveIdx, ezUInt32 uiIdx, float fNewPosX, float fNewPosY, bool bRightTangent);
  void InsertCpEvent(ezUInt32 uiCurveIdx, ezInt64 iTickX, double value);
  void TangentLinkEvent(ezUInt32 uiCurveIdx, ezUInt32 uiIdx, bool bLink);
  void CpTangentModeEvent(ezUInt32 uiCurveIdx, ezUInt32 uiIdx, bool bRightTangent, int iMode); // ezCurveTangentMode

  void BeginCpChangesEvent(QString sName);
  void EndCpChangesEvent();

  void BeginOperationEvent(QString sName);
  void EndOperationEvent(bool bCommit);

private Q_SLOTS:
  void on_LinePosition_editingFinished();
  void on_LineValue_editingFinished();
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
  void onGenerateCurve(ezMath::ezCurveFunction function, bool inverse);
  void onSaveAsPreset();
  void onLoadPreset();

private:
  void InsertCpAt(double posX, double value, ezVec2d epsilon);
  bool PickCurveAt(double x, double y, double fMaxDistanceY, ezInt32& out_iCurveIdx, double& out_ValueY) const;
  bool PickControlPointAt(double x, double y, ezVec2d vMaxDistance, ezInt32& out_iCurveIdx, ezInt32& out_iCpIdx) const;
  void UpdateSpinBoxes();
  void SetTangentMode(ezCurveTangentMode::Enum mode, bool bLeft, bool bRight);
  void ClampPoint(double& x, double& y) const;
  void SaveCurvePreset(const char* szFile) const;
  ezResult LoadCurvePreset(const char* szFile);
  void FindAllPresets();

  double m_fCurveDuration;
  ezVec2 m_vTangentMove;
  ezVec2d m_vControlPointMove;
  ezCurveGroupData m_Curves;
  ezCurveGroupData m_CurvesBackup;
  QPointF m_ContextMenuScenePos;

  static ezDynamicArray<ezString> s_CurvePresets;
};
