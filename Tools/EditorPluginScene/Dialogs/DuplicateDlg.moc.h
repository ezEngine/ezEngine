#pragma once

#include <EditorPluginScene/Plugin.h>
#include <Tools/EditorPluginScene/ui_DuplicateDlg.h>

#include <QDialog>

class ezQtDuplicateDlg : public QDialog, public Ui_DuplicateDlg
{
  Q_OBJECT

public:
  ezQtDuplicateDlg(QWidget* parent = nullptr);

  static ezUInt32 s_uiNumberOfCopies;
  static bool s_bGroupCopies;
  static ezVec3 s_vTranslationStep;
  static ezVec3 s_vRotationStep;
  static ezVec3 s_vRandomTranslation;
  static ezVec3 s_vRandomRotation;
  static int s_iRevolveStartAngle;
  static int s_iRevolveAngleStep;
  static float s_fRevolveRadius;
  static int s_iRevolveAxis;

public Q_SLOTS:
  virtual void on_DefaultButtons_clicked(QAbstractButton* button);

  virtual void on_toolButtonTransX_clicked();
  virtual void on_toolButtonTransY_clicked();
  virtual void on_toolButtonTransZ_clicked();

  virtual void on_RevolveNone_clicked();
  virtual void on_RevolveX_clicked();
  virtual void on_RevolveY_clicked();
  virtual void on_RevolveZ_clicked();

private:
  ezVec3 m_vBoundingBoxSize;
};
