#pragma once

#include <EditorPluginScene/EditorPluginSceneDLL.h>
#include <Tools/EditorPluginScene/ui_DeltaTransformDlg.h>
#include <QDialog>

class ezSceneDocument;

class ezQtDeltaTransformDlg : public QDialog, public Ui_DeltaTransformDlg
{
  Q_OBJECT

public:
  ezQtDeltaTransformDlg(QWidget *parent, ezSceneDocument* pSceneDoc);

  enum Mode
  {
    Translate,
    TranslateDeviation,
    RotateX,
    RotateXRandom,
    RotateXDeviation,
    RotateY,
    RotateYRandom,
    RotateYDeviation,
    RotateZ,
    RotateZRandom,
    RotateZDeviation,
    Scale,
    ScaleDeviation,
    UniformScale,
    UniformScaleDeviation,
    NaturalDeviationZ,
  };

  enum Space
  {
    World,
    LocalSelection,
    LocalEach,
  };

private Q_SLOTS:
  void on_ButtonApply_clicked();
  void on_ButtonUndo_clicked();
  void on_ComboMode_currentIndexChanged(int index);
  void on_ComboSpace_currentIndexChanged(int index);
  void on_Value1_valueChanged(double value);
  void on_Value2_valueChanged(double value);
  void on_Value3_valueChanged(double value);

private:
  void QueryUI();
  void UpdateUI();

  static Mode s_Mode;
  static Space s_Space;
  static ezVec3 s_vTranslate;
  static ezVec3 s_vTranslateDeviation;
  static ezVec3 s_vScale;
  static ezVec3 s_vScaleDeviation;
  static float s_fUniformScale;
  static float s_fUniformScaleDeviation;
  static ezVec3 s_vRotate;
  static ezVec3 s_vRotateRandom;
  static ezVec3 s_vRotateDeviation;
  static float s_fNaturalDeviationZ;

  ezUInt32 m_uiActionsApplied = 0;
  ezSceneDocument* m_pSceneDocument = nullptr;

};
