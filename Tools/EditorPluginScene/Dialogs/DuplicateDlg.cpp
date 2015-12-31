#include <PCH.h>
#include <EditorPluginScene/Dialogs/DuplicateDlg.moc.h>

//aeEntity* DuplicateSpecial(aeEntity* pOriginal, const ezVec3& vOffset, const ezVec3& vRotation, ezVec3& vRandomTranslation, const ezVec3& vRandomRotation);

ezUInt32 qtDuplicateDlg::s_uiNumberOfCopies = 1;
bool qtDuplicateDlg::s_bStartAtCenter = false;
bool qtDuplicateDlg::s_bGroupCopies = true;
ezVec3 qtDuplicateDlg::s_vTranslation(0, 0, 0);
ezVec3 qtDuplicateDlg::s_vRotation(0, 0, 0);
ezVec3 qtDuplicateDlg::s_vTranslationGauss(0, 0, 0);
ezVec3 qtDuplicateDlg::s_vRotationGauss(0, 0, 0);
int qtDuplicateDlg::s_iStartAngle = 0;
int qtDuplicateDlg::s_iRevolveAngle = 0;
float qtDuplicateDlg::s_fRevolveRadius = 1.0f;
ezVec3 qtDuplicateDlg::vSize;
ezVec3 qtDuplicateDlg::vTranslate;
int qtDuplicateDlg::s_iRevolveAxis = 0;


qtDuplicateDlg::qtDuplicateDlg(QWidget *parent) : QDialog(parent)
{
  setupUi(this);

  vSize.SetZero();
  //ezVec3 vTemp;
  //g_Selection.getBoundingBox(vTemp, vSize);
  vSize *= 2.0f;

  vSize.x = ezMath::Round(vSize.x, 0.01f);
  vSize.y = ezMath::Round(vSize.y, 0.01f);
  vSize.z = ezMath::Round(vSize.z, 0.01f);

  CheckBoxGroupCopies->setChecked(s_bGroupCopies);

  RefPointCenter->setChecked(s_bStartAtCenter);
  RefPointMouse->setChecked(!s_bStartAtCenter);
  spinBoxCopies->setValue(s_uiNumberOfCopies);

  SpinBoxTransX->setValue(s_vTranslation.x);
  SpinBoxTransY->setValue(s_vTranslation.y);
  SpinBoxTransZ->setValue(s_vTranslation.z);

  SpinBoxRotX->setValue(s_vRotation.x);
  SpinBoxRotY->setValue(s_vRotation.y);
  SpinBoxRotZ->setValue(s_vRotation.z);

  SpinBoxTransX_2->setValue(s_vTranslationGauss.x);
  SpinBoxTransY_2->setValue(s_vTranslationGauss.y);
  SpinBoxTransZ_2->setValue(s_vTranslationGauss.z);

  SpinBoxRotX_2->setValue(s_vRotationGauss.x);
  SpinBoxRotY_2->setValue(s_vRotationGauss.y);
  SpinBoxRotZ_2->setValue(s_vRotationGauss.z);

  SpinBoxStartAngle->setValue(s_iStartAngle);
  SpinBoxAngle->setValue(s_iRevolveAngle);
  SpinBoxRadius->setValue(s_fRevolveRadius);

  RevolveNone->setEnabled(!s_bStartAtCenter);
  RevolveX->setEnabled(!s_bStartAtCenter);
  RevolveY->setEnabled(!s_bStartAtCenter);
  RevolveZ->setEnabled(!s_bStartAtCenter);

  SpinBoxStartAngle->setEnabled((!s_bStartAtCenter) && (s_iRevolveAxis != 0));
  SpinBoxAngle->setEnabled((!s_bStartAtCenter) && (s_iRevolveAxis != 0));
  SpinBoxRadius->setEnabled((!s_bStartAtCenter) && (s_iRevolveAxis != 0));

  RevolveNone->setChecked(s_iRevolveAxis == 0);
  RevolveX->setChecked(s_iRevolveAxis == 1);
  RevolveY->setChecked(s_iRevolveAxis == 2);
  RevolveZ->setChecked(s_iRevolveAxis == 3);

  /// \todo If Selection empty, return

  //ezVec3 vWorldPos, temp;
  //int iView = WORKSPACE::getViewCursorIsInside(WORKSPACE::m_iMousePosX, WORKSPACE::m_iMousePosY, vWorldPos, temp);

  //if ((iView < 0) || (iView >= 3))
  //{
  //  close();
  //  return;
  //}

  //aeEntity* pEntity;
  //if (!aeInstanceManager::isInstanceAvailable(g_Selection.getItem(0), (aeInstance**)&pEntity))
  //{
  //  close();
  //  return;
  //}

  //vTranslate = vWorldPos - pEntity->getGlobalOrientation().m_vPosition;
  vTranslate.SetZero();

  //switch (WORKSPACE::m_Views[iView].getViewAxis())
  //{
  //case WORKSPACE_VIEW::X_AXIS:
  //  vTranslate.x = 0;
  //  break;
  //case WORKSPACE_VIEW::Y_AXIS:
  //  vTranslate.y = 0;
  //  break;
  //case WORKSPACE_VIEW::Z_AXIS:
  //  vTranslate.z = 0;
  //  break;
  //}

  //vTranslate.x = ezMath::Round(vTranslate.x, WORKSPACE::m_fGridDensity);
  //vTranslate.y = ezMath::Round(vTranslate.y, WORKSPACE::m_fGridDensity);
  //vTranslate.z = ezMath::Round(vTranslate.z, WORKSPACE::m_fGridDensity);
}

void qtDuplicateDlg::on_PushButtonCancel_clicked()
{
  reject();
}


//aeEntity* DuplicateSpecial(aeEntity* pOriginal, const ezVec3& vOffset, const ezVec3& vRotation, const ezVec3& vRandomTranslation, const ezVec3& vRandomRotation)
//{
//  aeEntity* pNew = pOriginal->Clone(true, false);
//
//  ezMat4 mX, mY, mZ, m, m2, mT;
//  mX.SetRotationMatrixX(vRotation.x);
//  mY.SetRotationMatrixY(vRotation.y);
//  mZ.SetRotationMatrixZ(vRotation.z);
//  m = mX * mY * mZ;
//
//  float rx = rand_gauss_signed(vRandomRotation.x * 10.0f) / 10.0f;
//  float ry = rand_gauss_signed(vRandomRotation.y * 10.0f) / 10.0f;
//  float rz = rand_gauss_signed(vRandomRotation.z * 10.0f) / 10.0f;
//
//  float tx = (vRandomTranslation.x > 0.0f) ? (rand_gauss_signed(vRandomTranslation.x * 100.0f) / 100.0f) : 0.0f;
//  float ty = (vRandomTranslation.y > 0.0f) ? (rand_gauss_signed(vRandomTranslation.y * 100.0f) / 100.0f) : 0.0f;
//  float tz = (vRandomTranslation.z > 0.0f) ? (rand_gauss_signed(vRandomTranslation.z * 100.0f) / 100.0f) : 0.0f;
//
//  mX.SetRotationMatrixX(rx);
//  mY.SetRotationMatrixY(ry);
//  mZ.SetRotationMatrixZ(rz);
//  m2 = mX * mY * mZ;
//
//  m = m * m2;
//
//  ezQuat qb = m.getAsQuaternion() * pOriginal->getGlobalOrientation().m_qRotation;
//
//  pNew->setLocalOrientation(aeOrientation(vOffset + qb * ezVec3(tx, ty, tz), qb));
//
//  return (pNew);
//}

void qtDuplicateDlg::on_toolButtonTransX_clicked()
{
  const float f = ezMath::Round((float)SpinBoxTransX->value(), 0.01f);

  if (f == vSize.x)
    SpinBoxTransX->setValue(-vSize.x);
  else
    if (f == -vSize.x)
      SpinBoxTransX->setValue(0.0);
    else
      SpinBoxTransX->setValue(vSize.x);
}

void qtDuplicateDlg::on_toolButtonTransY_clicked()
{
  const float f = ezMath::Round((float)SpinBoxTransY->value(), 0.01f);

  if (f == vSize.y)
    SpinBoxTransY->setValue(-vSize.y);
  else
    if (f == -vSize.y)
      SpinBoxTransY->setValue(0.0);
    else
      SpinBoxTransY->setValue(vSize.y);
}

void qtDuplicateDlg::on_toolButtonTransZ_clicked()
{
  const float f = ezMath::Round((float)SpinBoxTransZ->value(), 0.01f);

  if (f == vSize.z)
    SpinBoxTransZ->setValue(-vSize.z);
  else
    if (f == -vSize.z)
      SpinBoxTransZ->setValue(0.0);
    else
      SpinBoxTransZ->setValue(vSize.z);
}


void qtDuplicateDlg::on_toolButtonRotX_clicked()
{
  SpinBoxRotX->setValue(0);
}

void qtDuplicateDlg::on_toolButtonRotY_clicked()
{
  SpinBoxRotY->setValue(0);
}

void qtDuplicateDlg::on_toolButtonRotZ_clicked()
{
  SpinBoxRotZ->setValue(0);
}


void qtDuplicateDlg::on_toolButtonRotX_2_clicked()
{
  SpinBoxRotX_2->setValue(0);
}

void qtDuplicateDlg::on_toolButtonRotY_2_clicked()
{
  SpinBoxRotY_2->setValue(0);
}

void qtDuplicateDlg::on_toolButtonRotZ_2_clicked()
{
  SpinBoxRotZ_2->setValue(0);
}


void qtDuplicateDlg::on_toolButtonTransX_2_clicked()
{
  SpinBoxTransX_2->setValue(0);
}

void qtDuplicateDlg::on_toolButtonTransY_2_clicked()
{
  SpinBoxTransY_2->setValue(0);
}

void qtDuplicateDlg::on_toolButtonTransZ_2_clicked()
{
  SpinBoxTransZ_2->setValue(0);
}

void qtDuplicateDlg::on_PushButtonReset_clicked()
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
  SpinBoxRadius->setValue(s_fRevolveRadius);
}

void qtDuplicateDlg::on_RefPointCenter_clicked()
{
  if (RefPointCenter->isChecked())
  {
    s_bStartAtCenter = true;

    RevolveNone->setEnabled(!s_bStartAtCenter);
    RevolveX->setEnabled(!s_bStartAtCenter);
    RevolveY->setEnabled(!s_bStartAtCenter);
    RevolveZ->setEnabled(!s_bStartAtCenter);

    SpinBoxStartAngle->setEnabled((!s_bStartAtCenter) && (s_iRevolveAxis != 0));
    SpinBoxAngle->setEnabled((!s_bStartAtCenter) && (s_iRevolveAxis != 0));
    SpinBoxRadius->setEnabled((!s_bStartAtCenter) && (s_iRevolveAxis != 0));
  }
}

void qtDuplicateDlg::on_RefPointMouse_clicked()
{
  if (RefPointMouse->isChecked())
  {
    s_bStartAtCenter = false;

    RevolveNone->setEnabled(!s_bStartAtCenter);
    RevolveX->setEnabled(!s_bStartAtCenter);
    RevolveY->setEnabled(!s_bStartAtCenter);
    RevolveZ->setEnabled(!s_bStartAtCenter);

    SpinBoxStartAngle->setEnabled((!s_bStartAtCenter) && (s_iRevolveAxis != 0));
    SpinBoxAngle->setEnabled((!s_bStartAtCenter) && (s_iRevolveAxis != 0));
    SpinBoxRadius->setEnabled((!s_bStartAtCenter) && (s_iRevolveAxis != 0));
  }
}

void qtDuplicateDlg::on_RevolveNone_clicked()
{
  if (RevolveNone->isChecked())
  {
    s_iRevolveAxis = 0;

    SpinBoxStartAngle->setEnabled((!s_bStartAtCenter) && (s_iRevolveAxis != 0));
    SpinBoxAngle->setEnabled((!s_bStartAtCenter) && (s_iRevolveAxis != 0));
    SpinBoxRadius->setEnabled((!s_bStartAtCenter) && (s_iRevolveAxis != 0));
  }
}

void qtDuplicateDlg::on_RevolveX_clicked()
{
  if (RevolveX->isChecked())
  {
    s_iRevolveAxis = 1;

    SpinBoxStartAngle->setEnabled((!s_bStartAtCenter) && (s_iRevolveAxis != 0));
    SpinBoxAngle->setEnabled((!s_bStartAtCenter) && (s_iRevolveAxis != 0));
    SpinBoxRadius->setEnabled((!s_bStartAtCenter) && (s_iRevolveAxis != 0));
  }
}

void qtDuplicateDlg::on_RevolveY_clicked()
{
  if (RevolveY->isChecked())
  {
    s_iRevolveAxis = 2;

    SpinBoxStartAngle->setEnabled((!s_bStartAtCenter) && (s_iRevolveAxis != 0));
    SpinBoxAngle->setEnabled((!s_bStartAtCenter) && (s_iRevolveAxis != 0));
    SpinBoxRadius->setEnabled((!s_bStartAtCenter) && (s_iRevolveAxis != 0));
  }
}

void qtDuplicateDlg::on_RevolveZ_clicked()
{
  if (RevolveZ->isChecked())
  {
    s_iRevolveAxis = 3;

    SpinBoxStartAngle->setEnabled((!s_bStartAtCenter) && (s_iRevolveAxis != 0));
    SpinBoxAngle->setEnabled((!s_bStartAtCenter) && (s_iRevolveAxis != 0));
    SpinBoxRadius->setEnabled((!s_bStartAtCenter) && (s_iRevolveAxis != 0));
  }
}



void qtDuplicateDlg::on_PushButtonOk_clicked()
{
  s_bGroupCopies = CheckBoxGroupCopies->isChecked();

  s_uiNumberOfCopies = spinBoxCopies->value();

  s_vTranslation.x = SpinBoxTransX->value();
  s_vTranslation.y = SpinBoxTransY->value();
  s_vTranslation.z = SpinBoxTransZ->value();

  s_vRotation.x = SpinBoxRotX->value();
  s_vRotation.y = SpinBoxRotY->value();
  s_vRotation.z = SpinBoxRotZ->value();

  s_vTranslationGauss.x = SpinBoxTransX_2->value();
  s_vTranslationGauss.y = SpinBoxTransY_2->value();
  s_vTranslationGauss.z = SpinBoxTransZ_2->value();

  s_vRotationGauss.x = SpinBoxRotX_2->value();
  s_vRotationGauss.y = SpinBoxRotY_2->value();
  s_vRotationGauss.z = SpinBoxRotZ_2->value();

  s_iStartAngle = SpinBoxStartAngle->value();
  s_iRevolveAngle = SpinBoxAngle->value();
  s_fRevolveRadius = ezMath::Clamp((float)SpinBoxRadius->value(), 0.0f, 100.0f);


  //list<aeEntity*> NewSelection;

  //UNDO_SYSTEM::BeginUndoableOperation();

  //int iAdd = s_bStartAtCenter ? 1 : 0;

  //for (int s = 0; s < g_Selection.getNumberOfItems(); ++s)
  //{
  //  aeEntity* pEntity;
  //  if (aeInstanceManager::isInstanceAvailable(g_Selection.getItem(s), (aeInstance**)&pEntity))
  //  {
  //    aeGroup *pGroup;
  //    if (s_bGroupCopies)
  //    {
  //      g_Scenegraph->AddEntity<aeGroup>((aeEntity**)&pGroup);

  //      pGroup->setName("Duplicate");

  //      if ((!s_bStartAtCenter) && (s_iRevolveAxis > 0))
  //        pGroup->setGlobalOrientation(aeOrientation(pEntity->getGlobalOrientation().m_vPosition + vTranslate, ezQuat::getIdentityQuaternion()));
  //      else
  //        pGroup->setGlobalOrientation(pEntity->getGlobalOrientation());

  //      UNDO_SYSTEM::StoreOperation(UNDO_SYSTEM::UNDOOP_NODE_CREATED, pGroup->getInstanceHandle());

  //      if (s_bStartAtCenter)
  //      {
  //        UNDO_SYSTEM::StoreOperation(UNDO_SYSTEM::UNDOOP_CHANGE_PARENT, pEntity->getInstanceHandle());

  //        pGroup->AddChild(pEntity->getInstanceHandle());
  //        pEntity->setGlobalOrientation(pEntity->getGlobalOrientation());
  //      }

  //      NewSelection.push_back(pGroup);
  //    }
  //    else
  //      if (s_bStartAtCenter)
  //        NewSelection.push_back(pEntity);

  //    for (int i = 0; i < s_uiNumberOfCopies - iAdd; ++i)
  //    {
  //      ezVec3 vCenter = pEntity->getGlobalOrientation().m_vPosition;

  //      if (!s_bStartAtCenter)
  //      {
  //        vCenter += vTranslate;

  //        if (s_iRevolveAxis > 0)
  //        {
  //          ezMat4 m;

  //          switch (s_iRevolveAxis)
  //          {
  //          case 1: //X-axis
  //            m.setRotationMatrixX((float)(s_iStartAngle + i * s_iRevolveAngle));
  //            vCenter += m * ezVec3(0, 0, s_fRevolveRadius);
  //            break;
  //          case 2: //Y-axis
  //            m.setRotationMatrixY((float)(s_iStartAngle + i * s_iRevolveAngle));
  //            vCenter += m * ezVec3(s_fRevolveRadius, 0, 0);
  //            break;
  //          case 3: //Z-axis
  //            m.setRotationMatrixZ((float)(s_iStartAngle + i * s_iRevolveAngle));
  //            vCenter += m * ezVec3(s_fRevolveRadius, 0, 0);
  //            break;
  //          }
  //        }
  //      }

  //      aeEntity* pRes = DuplicateSpecial(pEntity, vCenter + (float)(i + iAdd) * s_vTranslation, (float)(i + iAdd) * s_vRotation, s_vTranslationGauss, s_vRotationGauss);

  //      if (s_bGroupCopies)
  //      {
  //        if ((!s_bStartAtCenter) && (i == 0) && (s_iRevolveAxis == 0))
  //          pGroup->setGlobalOrientation(pRes->getLocalOrientation());

  //        pGroup->AddChild(pRes->getInstanceHandle());

  //        pRes->setGlobalOrientation(pRes->getLocalOrientation());
  //      }
  //      else
  //        NewSelection.push_back(pRes);

  //      UNDO_SYSTEM::StoreOperation(UNDO_SYSTEM::UNDOOP_NODE_CREATED, pRes->getInstanceHandle());
  //    }
  //  }
  //}

  //UNDO_SYSTEM::EndUndoableOperation(true);

  //g_Selection.ClearSelection();

  //for (list<aeEntity*>::iterator it = NewSelection.begin(); it != NewSelection.end(); ++it)
  //  g_Selection.AddItem((*it)->getInstanceHandle());

  accept();
}


