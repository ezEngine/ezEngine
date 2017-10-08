#include <PCH.h>
#include <EditorPluginScene/Dialogs/DeltaTransformDlg.moc.h>
#include <EditorPluginScene/Scene/SceneDocument.h>
#include <Core/World/GameObject.h>

ezInt32 ezQtDeltaTransformDlg::s_iMode = 0;
ezInt32 ezQtDeltaTransformDlg::s_iSpace = 0;
ezVec3 ezQtDeltaTransformDlg::s_vTranslate(0.0f);
ezVec3 ezQtDeltaTransformDlg::s_vScale(1.0f);
float ezQtDeltaTransformDlg::s_fUniformScale = 1.0f;
ezVec3 ezQtDeltaTransformDlg::s_vRotate(0.0f);

ezQtDeltaTransformDlg::ezQtDeltaTransformDlg(QWidget *parent, ezSceneDocument* pSceneDoc)
  : QDialog(parent)
{
  m_pSceneDocument = pSceneDoc;

  setupUi(this);

  UpdateUI();
  ComboMode->setCurrentIndex(s_iMode);
  ComboSpace->setCurrentIndex(s_iSpace);

  ButtonUndo->setEnabled(false);

}

void ezQtDeltaTransformDlg::on_ComboMode_currentIndexChanged(int index)
{
  QueryUI();

  s_iMode = index;

  UpdateUI();
}

void ezQtDeltaTransformDlg::on_ComboSpace_currentIndexChanged(int index)
{
  s_iSpace = index;
}

void ezQtDeltaTransformDlg::on_ButtonApply_clicked()
{
  auto history = m_pSceneDocument->GetCommandHistory();
  auto selman = m_pSceneDocument->GetSelectionManager();
  const auto& selection = selman->GetTopLevelSelection();

  if (selection.IsEmpty())
    return;

  const ezInt32 iSpace = ComboSpace->currentIndex();

  ezTransform tReference;
  tReference.SetIdentity();

  if (iSpace == 1)
  {
    tReference = m_pSceneDocument->GetGlobalTransform(selection.PeekBack());
  }

  history->StartTransaction("Delta Transform");

  for (const ezDocumentObject* pObject : selection)
  {
    if (!pObject->GetTypeAccessor().GetType()->IsDerivedFrom<ezGameObject>())
      continue;

    if (iSpace == 2)
    {
      tReference = m_pSceneDocument->GetGlobalTransform(pObject);
    }

    ezTransform trans = m_pSceneDocument->GetGlobalTransform(pObject);
    ezTransform localTrans = tReference.GetInverse() * trans;
    ezQuat qRot;

    switch (s_iMode)
    {
    case 0: // translate
      trans.m_vPosition += tReference.m_qRotation * s_vTranslate;
      m_pSceneDocument->SetGlobalTransform(pObject, trans, TransformationChanges::Translation);
      break;

    case 1: // rotate X
      qRot.SetFromAxisAndAngle(ezVec3(1, 0, 0), ezAngle::Degree(s_vRotate.x));
      localTrans.m_qRotation = qRot * localTrans.m_qRotation;
      trans = tReference * localTrans;
      trans.m_qRotation.Normalize();
      m_pSceneDocument->SetGlobalTransform(pObject, trans, TransformationChanges::Rotation);
      break;

    case 2: // rotate Y
      qRot.SetFromAxisAndAngle(ezVec3(0, 1, 0), ezAngle::Degree(s_vRotate.y));
      localTrans.m_qRotation = qRot * localTrans.m_qRotation;
      trans = tReference * localTrans;
      trans.m_qRotation.Normalize();
      m_pSceneDocument->SetGlobalTransform(pObject, trans, TransformationChanges::Rotation);
      break;

    case 3: // rotate Z
      qRot.SetFromAxisAndAngle(ezVec3(0, 0, 1), ezAngle::Degree(s_vRotate.z));
      localTrans.m_qRotation = qRot * localTrans.m_qRotation;
      trans = tReference * localTrans;
      trans.m_qRotation.Normalize();
      m_pSceneDocument->SetGlobalTransform(pObject, trans, TransformationChanges::Rotation);
      break;

    case 4: // Scale
      trans.m_vScale = trans.m_vScale.CompMul(s_vScale);
      m_pSceneDocument->SetGlobalTransform(pObject, trans, TransformationChanges::Scale);
      break;

    case 5: // Scale Uniform
      trans.m_vScale *= s_fUniformScale;

      if (trans.m_vScale.x == trans.m_vScale.y && trans.m_vScale.x == trans.m_vScale.z)
        m_pSceneDocument->SetGlobalTransform(pObject, trans, TransformationChanges::UniformScale);
      else
        m_pSceneDocument->SetGlobalTransform(pObject, trans, TransformationChanges::Scale);

      break;
    }
  }

  history->FinishTransaction();

  ButtonUndo->setEnabled(true);
  ++m_uiActionsApplied;
}

void ezQtDeltaTransformDlg::on_ButtonUndo_clicked()
{
  auto history = m_pSceneDocument->GetCommandHistory();

  if (m_uiActionsApplied > 0)
  {
    --m_uiActionsApplied;
    if (history->CanUndo())
    {
      history->Undo();
    }
  }

  if (m_uiActionsApplied == 0 || !history->CanUndo())
  {
    ButtonUndo->setEnabled(false);
  }
}

void ezQtDeltaTransformDlg::on_ButtonReset_clicked()
{
  s_vTranslate.SetZero();
  s_vScale.Set(1.0f);
  s_fUniformScale = 1.0f;
  s_vRotate.SetZero();

  UpdateUI();
}

void ezQtDeltaTransformDlg::QueryUI()
{
  switch (s_iMode)
  {
  case 0: // translate
    s_vTranslate.x = (float)Value1->value();
    s_vTranslate.y = (float)Value2->value();
    s_vTranslate.z = (float)Value3->value();
    break;

  case 1: // rotate X
    s_vRotate.x = (float)Value1->value();
    break;

  case 2: // rotate Y
    s_vRotate.y = (float)Value1->value();
    break;

  case 3: // rotate Z
    s_vRotate.z = (float)Value1->value();
    break;

  case 4: // Scale
    s_vScale.x = (float)Value1->value();
    s_vScale.y = (float)Value2->value();
    s_vScale.z = (float)Value3->value();
    break;

  case 5: // Scale Uniform
    s_fUniformScale = (float)Value1->value();
    break;
  }
}

void ezQtDeltaTransformDlg::UpdateUI()
{
  ComboSpace->setVisible(true);
  Value1->setVisible(true);
  Value2->setVisible(true);
  Value3->setVisible(true);
  Label1->setVisible(true);
  Label2->setVisible(true);
  Label3->setVisible(true);
  Label1->setText("X");
  Label2->setText("Y");
  Label3->setText("Z");

  switch (s_iMode)
  {
  case 0: // translate
    Value1->setValue(s_vTranslate.x);
    Value2->setValue(s_vTranslate.y);
    Value3->setValue(s_vTranslate.z);
    break;

  case 1: // rotate X
    Value2->setVisible(false);
    Value3->setVisible(false);
    Label2->setVisible(false);
    Label3->setVisible(false);
    Value1->setValue(s_vRotate.x);
    break;

  case 2: // rotate Y
    Value2->setVisible(false);
    Value3->setVisible(false);
    Label2->setVisible(false);
    Label3->setVisible(false);
    Label1->setText("Y");
    Value1->setValue(s_vRotate.y);
    break;

  case 3: // rotate Z
    Value2->setVisible(false);
    Value3->setVisible(false);
    Label2->setVisible(false);
    Label3->setVisible(false);
    Label1->setText("Z");
    Value1->setValue(s_vRotate.z);
    break;

  case 4: // Scale
    ComboSpace->setVisible(false);
    Value1->setValue(s_vScale.x);
    Value2->setValue(s_vScale.y);
    Value3->setValue(s_vScale.z);
    break;

  case 5: // Scale Uniform
    ComboSpace->setVisible(false);
    Value2->setVisible(false);
    Value3->setVisible(false);
    Label2->setVisible(false);
    Label3->setVisible(false);
    Label1->setText("XYZ");
    Value1->setValue(s_fUniformScale);
    break;
  }
}

void ezQtDeltaTransformDlg::on_Value1_valueChanged(double value)
{
  switch (s_iMode)
  {
  case 0: // translate
    s_vTranslate.x = (float)Value1->value();
    break;

  case 1: // rotate X
    s_vRotate.x = (float)Value1->value();
    break;

  case 2: // rotate Y
    s_vRotate.y = (float)Value1->value();
    break;

  case 3: // rotate Z
    s_vRotate.z = (float)Value1->value();
    break;

  case 4: // Scale
    s_vScale.x = (float)Value1->value();
    break;

  case 5: // Scale Uniform
    s_fUniformScale = (float)Value1->value();
    break;
  }
}

void ezQtDeltaTransformDlg::on_Value2_valueChanged(double value)
{
  switch (s_iMode)
  {
  case 0: // translate
    s_vTranslate.y = (float)Value2->value();
    break;

  case 4: // Scale
    s_vScale.y = (float)Value2->value();
    break;
  }
}

void ezQtDeltaTransformDlg::on_Value3_valueChanged(double value)
{
  switch (s_iMode)
  {
  case 0: // translate
    s_vTranslate.z = (float)Value3->value();
    break;

  case 4: // Scale
    s_vScale.z = (float)Value3->value();
    break;
  }
}
