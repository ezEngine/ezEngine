#pragma once

#include <EditorPluginScene/Plugin.h>
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
    RotateX,
    RotateY,
    RotateZ,
    Scale,
    UniformScale,
  };

  enum Space
  {
    World,
    LocalSelection,
    LocalEach,
  };

private slots:
  void on_ButtonApply_clicked();
  void on_ButtonUndo_clicked();
  void on_ButtonReset_clicked();
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
  static ezVec3 s_vScale;
  static float s_fUniformScale;
  static ezVec3 s_vRotate;

  ezUInt32 m_uiActionsApplied = 0;
  ezSceneDocument* m_pSceneDocument = nullptr;

};
