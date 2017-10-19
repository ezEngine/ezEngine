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

  void SetCurves(const ezArrayPtr<ezCurve1D>& curves);

  void FrameCurve();

signals:
  void InsertCpAt(float posX, float value, float epsilon);
  void CpMoved(ezUInt32 curveIdx, ezUInt32 cpIdx, float newPosX, float newPosY);
  void CpDeleted(ezUInt32 curveIdx, ezUInt32 cpIdx);
  void TangentMoved(ezUInt32 curveIdx, ezUInt32 cpIdx, float newPosX, float newPosY, bool rightTangent);

  void NormalizeRangeX();
  void NormalizeRangeY();

  void BeginCpChanges(QString name);
  void EndCpChanges();

  void BeginOperation(QString name);
  void EndOperation(bool commit);


private slots:
  void on_ButtonFrame_clicked();
  void on_SpinPosition_valueChanged(double value);
  void on_ButtonNormalizeX_clicked();
  void on_ButtonNormalizeY_clicked();
  void onDeleteControlPoints();
  void onDoubleClick(const QPointF& scenePos, const QPointF& epsilon);
  void onMoveControlPoints(double x, double y);
  void onMoveTangents(double x, double y);

private:

  ezHybridArray<ezCurve1D, 4> m_Curves;
};
