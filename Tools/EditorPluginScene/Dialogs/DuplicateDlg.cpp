#include <PCH.h>

#include <EditorPluginScene/Dialogs/DuplicateDlg.moc.h>
#include <QDialogButtonBox>
#include <QPushButton>

ezUInt32 ezQtDuplicateDlg::s_uiNumberOfCopies = 1;
bool ezQtDuplicateDlg::s_bGroupCopies = false;
ezVec3 ezQtDuplicateDlg::s_vTranslationStep(0, 0, 0);
ezVec3 ezQtDuplicateDlg::s_vRotationStep(0, 0, 0);
ezVec3 ezQtDuplicateDlg::s_vRandomTranslation(0, 0, 0);
ezVec3 ezQtDuplicateDlg::s_vRandomRotation(0, 0, 0);
int ezQtDuplicateDlg::s_iRevolveAxis = 0;
int ezQtDuplicateDlg::s_iRevolveStartAngle = 0;
int ezQtDuplicateDlg::s_iRevolveAngleStep = 0;
float ezQtDuplicateDlg::s_fRevolveRadius = 1.0f;


ezQtDuplicateDlg::ezQtDuplicateDlg(QWidget* parent)
    : QDialog(parent)
{
  setupUi(this);

  /// \todo Boundingbox helper not implemented yet (get bbox size from selection)
  {
    m_vBoundingBoxSize.SetZero();

    toolButtonTransX->setVisible(false);
    toolButtonTransY->setVisible(false);
    toolButtonTransZ->setVisible(false);
  }

  m_vBoundingBoxSize.x = ezMath::RoundToMultiple(m_vBoundingBoxSize.x, 0.01f);
  m_vBoundingBoxSize.y = ezMath::RoundToMultiple(m_vBoundingBoxSize.y, 0.01f);
  m_vBoundingBoxSize.z = ezMath::RoundToMultiple(m_vBoundingBoxSize.z, 0.01f);

  CheckBoxGroupCopies->setChecked(s_bGroupCopies);

  /// \todo Grouping of duplicates not implemented yet
  {
    CheckBoxGroupCopies->setVisible(false);
  }

  spinBoxCopies->setValue(s_uiNumberOfCopies);

  SpinBoxTransX->setValue(s_vTranslationStep.x);
  SpinBoxTransY->setValue(s_vTranslationStep.y);
  SpinBoxTransZ->setValue(s_vTranslationStep.z);

  SpinBoxRotX->setValue(s_vRotationStep.x);
  SpinBoxRotY->setValue(s_vRotationStep.y);
  SpinBoxRotZ->setValue(s_vRotationStep.z);

  SpinBoxTransX_2->setValue(s_vRandomTranslation.x);
  SpinBoxTransY_2->setValue(s_vRandomTranslation.y);
  SpinBoxTransZ_2->setValue(s_vRandomTranslation.z);

  SpinBoxRotX_2->setValue(s_vRandomRotation.x);
  SpinBoxRotY_2->setValue(s_vRandomRotation.y);
  SpinBoxRotZ_2->setValue(s_vRandomRotation.z);

  SpinBoxStartAngle->setValue(s_iRevolveStartAngle);
  SpinBoxAngle->setValue(s_iRevolveAngleStep);
  SpinBoxRadius->setValue(s_fRevolveRadius);

  SpinBoxStartAngle->setEnabled(s_iRevolveAxis != 0);
  SpinBoxAngle->setEnabled(s_iRevolveAxis != 0);
  SpinBoxRadius->setEnabled(s_iRevolveAxis != 0);

  RevolveNone->setChecked(s_iRevolveAxis == 0);
  RevolveX->setChecked(s_iRevolveAxis == 1);
  RevolveY->setChecked(s_iRevolveAxis == 2);
  RevolveZ->setChecked(s_iRevolveAxis == 3);
}

void ezQtDuplicateDlg::on_DefaultButtons_clicked(QAbstractButton* button)
{
  if (button == DefaultButtons->button(QDialogButtonBox::Cancel))
  {
    reject();
  }

  if (button == DefaultButtons->button(QDialogButtonBox::Reset))
  {
    spinBoxCopies->setValue(1);

    SpinBoxRotX->setValue(0);
    SpinBoxRotY->setValue(0);
    SpinBoxRotZ->setValue(0);

    SpinBoxTransX->setValue(0.0);
    SpinBoxTransY->setValue(0.0);
    SpinBoxTransZ->setValue(0.0);

    SpinBoxStartAngle->setValue(0);
    SpinBoxAngle->setValue(0);
  }

  if (button == DefaultButtons->button(QDialogButtonBox::Ok))
  {
    s_bGroupCopies = CheckBoxGroupCopies->isChecked();

    s_uiNumberOfCopies = spinBoxCopies->value();

    s_vTranslationStep.x = SpinBoxTransX->value();
    s_vTranslationStep.y = SpinBoxTransY->value();
    s_vTranslationStep.z = SpinBoxTransZ->value();

    s_vRotationStep.x = SpinBoxRotX->value();
    s_vRotationStep.y = SpinBoxRotY->value();
    s_vRotationStep.z = SpinBoxRotZ->value();

    s_vRandomTranslation.x = SpinBoxTransX_2->value();
    s_vRandomTranslation.y = SpinBoxTransY_2->value();
    s_vRandomTranslation.z = SpinBoxTransZ_2->value();

    s_vRandomRotation.x = SpinBoxRotX_2->value();
    s_vRandomRotation.y = SpinBoxRotY_2->value();
    s_vRandomRotation.z = SpinBoxRotZ_2->value();

    s_iRevolveStartAngle = SpinBoxStartAngle->value();
    s_iRevolveAngleStep = SpinBoxAngle->value();
    s_fRevolveRadius = ezMath::Clamp((float)SpinBoxRadius->value(), 0.0f, 1000.0f);

    accept();
  }
}

void ezQtDuplicateDlg::on_toolButtonTransX_clicked()
{
  const float f = ezMath::RoundToMultiple((float)SpinBoxTransX->value(), 0.01f);

  if (f == m_vBoundingBoxSize.x)
    SpinBoxTransX->setValue(-m_vBoundingBoxSize.x);
  else if (f == -m_vBoundingBoxSize.x)
    SpinBoxTransX->setValue(0.0);
  else
    SpinBoxTransX->setValue(m_vBoundingBoxSize.x);
}

void ezQtDuplicateDlg::on_toolButtonTransY_clicked()
{
  const float f = ezMath::RoundToMultiple((float)SpinBoxTransY->value(), 0.01f);

  if (f == m_vBoundingBoxSize.y)
    SpinBoxTransY->setValue(-m_vBoundingBoxSize.y);
  else if (f == -m_vBoundingBoxSize.y)
    SpinBoxTransY->setValue(0.0);
  else
    SpinBoxTransY->setValue(m_vBoundingBoxSize.y);
}

void ezQtDuplicateDlg::on_toolButtonTransZ_clicked()
{
  const float f = ezMath::RoundToMultiple((float)SpinBoxTransZ->value(), 0.01f);

  if (f == m_vBoundingBoxSize.z)
    SpinBoxTransZ->setValue(-m_vBoundingBoxSize.z);
  else if (f == -m_vBoundingBoxSize.z)
    SpinBoxTransZ->setValue(0.0);
  else
    SpinBoxTransZ->setValue(m_vBoundingBoxSize.z);
}

void ezQtDuplicateDlg::on_RevolveNone_clicked()
{
  if (RevolveNone->isChecked())
  {
    s_iRevolveAxis = 0;

    SpinBoxStartAngle->setEnabled(s_iRevolveAxis != 0);
    SpinBoxAngle->setEnabled(s_iRevolveAxis != 0);
    SpinBoxRadius->setEnabled(s_iRevolveAxis != 0);
  }
}

void ezQtDuplicateDlg::on_RevolveX_clicked()
{
  if (RevolveX->isChecked())
  {
    s_iRevolveAxis = 1;

    SpinBoxStartAngle->setEnabled(s_iRevolveAxis != 0);
    SpinBoxAngle->setEnabled(s_iRevolveAxis != 0);
    SpinBoxRadius->setEnabled(s_iRevolveAxis != 0);
  }
}

void ezQtDuplicateDlg::on_RevolveY_clicked()
{
  if (RevolveY->isChecked())
  {
    s_iRevolveAxis = 2;

    SpinBoxStartAngle->setEnabled(s_iRevolveAxis != 0);
    SpinBoxAngle->setEnabled(s_iRevolveAxis != 0);
    SpinBoxRadius->setEnabled(s_iRevolveAxis != 0);
  }
}

void ezQtDuplicateDlg::on_RevolveZ_clicked()
{
  if (RevolveZ->isChecked())
  {
    s_iRevolveAxis = 3;

    SpinBoxStartAngle->setEnabled(s_iRevolveAxis != 0);
    SpinBoxAngle->setEnabled(s_iRevolveAxis != 0);
    SpinBoxRadius->setEnabled(s_iRevolveAxis != 0);
  }
}
