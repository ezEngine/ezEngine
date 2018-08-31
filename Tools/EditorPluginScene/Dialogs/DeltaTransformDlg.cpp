#include <PCH.h>

#include <Core/World/GameObject.h>
#include <EditorPluginScene/Dialogs/DeltaTransformDlg.moc.h>
#include <EditorPluginScene/Scene/SceneDocument.h>
#include <Foundation/Math/Random.h>

ezQtDeltaTransformDlg::Mode ezQtDeltaTransformDlg::s_Mode = ezQtDeltaTransformDlg::Mode::Translate;
ezQtDeltaTransformDlg::Space ezQtDeltaTransformDlg::s_Space = ezQtDeltaTransformDlg::Space::World;
ezVec3 ezQtDeltaTransformDlg::s_vTranslate(0.0f);
ezVec3 ezQtDeltaTransformDlg::s_vScale(1.0f);
float ezQtDeltaTransformDlg::s_fUniformScale = 1.0f;
ezVec3 ezQtDeltaTransformDlg::s_vRotate(0.0f);
bool ezQtDeltaTransformDlg::s_bRandomDeviation = false;

ezQtDeltaTransformDlg::ezQtDeltaTransformDlg(QWidget* parent, ezSceneDocument* pSceneDoc)
    : QDialog(parent)
{
  m_pSceneDocument = pSceneDoc;

  setupUi(this);

  UpdateUI();
  ComboMode->setCurrentIndex(s_Mode);
  ComboSpace->setCurrentIndex(s_Space);

  ButtonUndo->setEnabled(m_uiActionsApplied > 0 && m_pSceneDocument->GetCommandHistory()->CanUndo());
}

void ezQtDeltaTransformDlg::on_ComboMode_currentIndexChanged(int index)
{
  QueryUI();

  s_Mode = (Mode)index;

  UpdateUI();
}

void ezQtDeltaTransformDlg::on_ComboSpace_currentIndexChanged(int index)
{
  s_Space = (Space)index;
}

void ezQtDeltaTransformDlg::on_ButtonApply_clicked()
{
  s_bRandomDeviation = CheckRandom->isChecked();

  ezStringBuilder sAction;

  // early out when nothing is to do
  switch (s_Mode)
  {
    case Mode::Translate:
      if (s_vTranslate == ezVec3(0.0f))
        return;

      sAction.Format("Translate: {0} | {1} | {2}", ezArgF(s_vTranslate.x, 2), ezArgF(s_vTranslate.y, 2), ezArgF(s_vTranslate.z, 2));
      break;

    case Mode::RotateX:
      if (s_vRotate.x == 0.0f)
        return;

      sAction.Format("Rotate X: {0}", ezArgF(s_vRotate.x, 1));
      break;

    case Mode::RotateY:
      if (s_vRotate.y == 0.0f)
        return;

      sAction.Format("Rotate Y: {0}", ezArgF(s_vRotate.y, 1));
      break;

    case Mode::RotateZ:
      if (s_vRotate.z == 0.0f)
        return;

      sAction.Format("Rotate Z: {0}", ezArgF(s_vRotate.z, 1));
      break;

    case Mode::Scale:
      if (s_vScale == ezVec3(1.0f))
        return;

      sAction.Format("Scale: {0} | {1} | {2}", ezArgF(s_vScale.x, 2), ezArgF(s_vScale.y, 2), ezArgF(s_vScale.z, 2));
      break;

    case Mode::UniformScale:
      if (s_fUniformScale == 1.0f)
        return;

      sAction.Format("Scale: {0}", ezArgF(s_fUniformScale, 2));
      break;
  }


  auto history = m_pSceneDocument->GetCommandHistory();
  auto selman = m_pSceneDocument->GetSelectionManager();
  const auto& selection = selman->GetTopLevelSelection();

  if (selection.IsEmpty())
    return;

  const Space space = (Space)ComboSpace->currentIndex();

  ezTransform tReference = m_pSceneDocument->GetGlobalTransform(selection.PeekBack());

  if (space == Space::World)
  {
    tReference = m_pSceneDocument->GetGlobalTransform(selection.PeekBack());
    tReference.m_qRotation.SetIdentity();
  }

  history->StartTransaction(sAction.GetData());

  ezRandom rng;

  if (s_bRandomDeviation)
  {
    rng.InitializeFromCurrentTime();
  }

  for (const ezDocumentObject* pObject : selection)
  {
    if (!pObject->GetTypeAccessor().GetType()->IsDerivedFrom<ezGameObject>())
      continue;

    ezVec3 vTranslate = s_vTranslate;
    ezVec3 vRotate = s_vRotate;
    ezVec3 vScale = s_vScale;
    float fUniformScale = s_fUniformScale;

    if (s_bRandomDeviation)
    {
      vTranslate.x = rng.DoubleMinMax(-s_vTranslate.x, +s_vTranslate.x);
      vTranslate.y = rng.DoubleMinMax(-s_vTranslate.y, +s_vTranslate.y);
      vTranslate.z = rng.DoubleMinMax(-s_vTranslate.z, +s_vTranslate.z);

      vRotate.x = rng.DoubleMinMax(-s_vRotate.x, +s_vRotate.x);
      vRotate.y = rng.DoubleMinMax(-s_vRotate.y, +s_vRotate.y);
      vRotate.z = rng.DoubleMinMax(-s_vRotate.z, +s_vRotate.z);


      vScale.x = rng.DoubleMinMax(ezMath::Min(1.0f / s_vScale.x, s_vScale.x), ezMath::Max(1.0f / s_vScale.x, s_vScale.x));
      vScale.y = rng.DoubleMinMax(ezMath::Min(1.0f / s_vScale.y, s_vScale.y), ezMath::Max(1.0f / s_vScale.y, s_vScale.y));
      vScale.z = rng.DoubleMinMax(ezMath::Min(1.0f / s_vScale.z, s_vScale.z), ezMath::Max(1.0f / s_vScale.z, s_vScale.z));

      fUniformScale = rng.DoubleMinMax(ezMath::Min(1.0f / s_fUniformScale, s_fUniformScale), ezMath::Max(1.0f / s_fUniformScale, s_fUniformScale));
    }

    if (space == Space::LocalEach)
    {
      tReference = m_pSceneDocument->GetGlobalTransform(pObject);
    }

    ezTransform trans = m_pSceneDocument->GetGlobalTransform(pObject);
    ezTransform localTrans = tReference.GetInverse() * trans;
    ezQuat qRot;

    switch (s_Mode)
    {
      case Mode::Translate:
        trans.m_vPosition += tReference.m_qRotation * vTranslate;
        m_pSceneDocument->SetGlobalTransform(pObject, trans, TransformationChanges::Translation);
        break;

      case Mode::RotateX:
        qRot.SetFromAxisAndAngle(ezVec3(1, 0, 0), ezAngle::Degree(vRotate.x));
        localTrans.m_qRotation = qRot * localTrans.m_qRotation;
        localTrans.m_vPosition = qRot * localTrans.m_vPosition;
        trans = tReference * localTrans;
        trans.m_qRotation.Normalize();
        m_pSceneDocument->SetGlobalTransform(pObject, trans, TransformationChanges::Translation | TransformationChanges::Rotation);
        break;

      case Mode::RotateY:
        qRot.SetFromAxisAndAngle(ezVec3(0, 1, 0), ezAngle::Degree(vRotate.y));
        localTrans.m_qRotation = qRot * localTrans.m_qRotation;
        localTrans.m_vPosition = qRot * localTrans.m_vPosition;
        trans = tReference * localTrans;
        trans.m_qRotation.Normalize();
        m_pSceneDocument->SetGlobalTransform(pObject, trans, TransformationChanges::Translation | TransformationChanges::Rotation);
        break;

      case Mode::RotateZ:
        qRot.SetFromAxisAndAngle(ezVec3(0, 0, 1), ezAngle::Degree(vRotate.z));
        localTrans.m_qRotation = qRot * localTrans.m_qRotation;
        localTrans.m_vPosition = qRot * localTrans.m_vPosition;
        trans = tReference * localTrans;
        trans.m_qRotation.Normalize();
        m_pSceneDocument->SetGlobalTransform(pObject, trans, TransformationChanges::Translation | TransformationChanges::Rotation);
        break;

      case Mode::Scale:
        trans.m_vScale = trans.m_vScale.CompMul(vScale);
        m_pSceneDocument->SetGlobalTransform(pObject, trans, TransformationChanges::Scale);
        break;

      case Mode::UniformScale:
        trans.m_vScale *= fUniformScale;

        if (trans.m_vScale.x == trans.m_vScale.y && trans.m_vScale.x == trans.m_vScale.z)
          m_pSceneDocument->SetGlobalTransform(pObject, trans, TransformationChanges::UniformScale);
        else
          m_pSceneDocument->SetGlobalTransform(pObject, trans, TransformationChanges::Scale);

        break;
    }
  }

  history->FinishTransaction();

  ++m_uiActionsApplied;
  ButtonUndo->setEnabled(m_uiActionsApplied > 0 && m_pSceneDocument->GetCommandHistory()->CanUndo());
}

void ezQtDeltaTransformDlg::on_ButtonUndo_clicked()
{
  auto history = m_pSceneDocument->GetCommandHistory();

  if (history->CanUndo())
  {
    --m_uiActionsApplied;
    history->Undo();
  }

  ButtonUndo->setEnabled(m_uiActionsApplied > 0 && m_pSceneDocument->GetCommandHistory()->CanUndo());
}

void ezQtDeltaTransformDlg::on_ButtonReset_clicked()
{
  switch (s_Mode)
  {
    case ezQtDeltaTransformDlg::Translate:
      s_vTranslate.SetZero();
      break;
    case ezQtDeltaTransformDlg::RotateX:
      s_vRotate.x = 0;
      break;
    case ezQtDeltaTransformDlg::RotateY:
      s_vRotate.y = 0;
      break;
    case ezQtDeltaTransformDlg::RotateZ:
      s_vRotate.z = 0;
      break;
    case ezQtDeltaTransformDlg::Scale:
      s_vScale.Set(1.0f);
      break;
    case ezQtDeltaTransformDlg::UniformScale:
      s_fUniformScale = 1.0f;
      break;
  }

  UpdateUI();
}

void ezQtDeltaTransformDlg::QueryUI()
{
  s_bRandomDeviation = CheckRandom->isChecked();

  switch (s_Mode)
  {
    case Mode::Translate:
      s_vTranslate.x = (float)Value1->value();
      s_vTranslate.y = (float)Value2->value();
      s_vTranslate.z = (float)Value3->value();
      break;

    case Mode::RotateX:
      s_vRotate.x = (float)Value1->value();
      break;

    case Mode::RotateY:
      s_vRotate.y = (float)Value1->value();
      break;

    case Mode::RotateZ:
      s_vRotate.z = (float)Value1->value();
      break;

    case Mode::Scale:
      s_vScale.x = (float)Value1->value();
      s_vScale.y = (float)Value2->value();
      s_vScale.z = (float)Value3->value();
      break;

    case Mode::UniformScale:
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
  CheckRandom->setChecked(s_bRandomDeviation);

  switch (s_Mode)
  {
    case Mode::Translate:
      Value1->setValue(s_vTranslate.x);
      Value2->setValue(s_vTranslate.y);
      Value3->setValue(s_vTranslate.z);
      Value1->setSingleStep(1.0f);
      Value2->setSingleStep(1.0f);
      Value3->setSingleStep(1.0f);
      break;

    case Mode::RotateX:
      Value2->setVisible(false);
      Value3->setVisible(false);
      Label2->setVisible(false);
      Label3->setVisible(false);
      Value1->setValue(s_vRotate.x);
      Value1->setSingleStep(5.0f);
      break;

    case Mode::RotateY:
      Value2->setVisible(false);
      Value3->setVisible(false);
      Label2->setVisible(false);
      Label3->setVisible(false);
      Label1->setText("Y");
      Value1->setValue(s_vRotate.y);
      Value1->setSingleStep(5.0f);
      break;

    case Mode::RotateZ:
      Value2->setVisible(false);
      Value3->setVisible(false);
      Label2->setVisible(false);
      Label3->setVisible(false);
      Label1->setText("Z");
      Value1->setValue(s_vRotate.z);
      Value1->setSingleStep(5.0f);
      break;

    case Mode::Scale:
      ComboSpace->setVisible(false);
      Value1->setValue(s_vScale.x);
      Value2->setValue(s_vScale.y);
      Value3->setValue(s_vScale.z);
      Value1->setSingleStep(1.0f);
      break;

    case Mode::UniformScale:
      ComboSpace->setVisible(false);
      Value2->setVisible(false);
      Value3->setVisible(false);
      Label2->setVisible(false);
      Label3->setVisible(false);
      Label1->setText("XYZ");
      Value1->setValue(s_fUniformScale);
      Value1->setSingleStep(1.0f);
      break;
  }
}

void ezQtDeltaTransformDlg::on_Value1_valueChanged(double value)
{
  switch (s_Mode)
  {
    case Mode::Translate:
      s_vTranslate.x = (float)Value1->value();
      break;

    case Mode::RotateX:
      s_vRotate.x = (float)Value1->value();
      break;

    case Mode::RotateY:
      s_vRotate.y = (float)Value1->value();
      break;

    case Mode::RotateZ:
      s_vRotate.z = (float)Value1->value();
      break;

    case Mode::Scale:
      s_vScale.x = (float)Value1->value();
      break;

    case Mode::UniformScale:
      s_fUniformScale = (float)Value1->value();
      break;
  }
}

void ezQtDeltaTransformDlg::on_Value2_valueChanged(double value)
{
  switch (s_Mode)
  {
    case Mode::Translate:
      s_vTranslate.y = (float)Value2->value();
      break;

    case Mode::Scale:
      s_vScale.y = (float)Value2->value();
      break;
  }
}

void ezQtDeltaTransformDlg::on_Value3_valueChanged(double value)
{
  switch (s_Mode)
  {
    case Mode::Translate:
      s_vTranslate.z = (float)Value3->value();
      break;

    case Mode::Scale:
      s_vScale.z = (float)Value3->value();
      break;
  }
}
