#include <EditorPluginScene/EditorPluginScenePCH.h>

#include <Core/World/GameObject.h>
#include <EditorPluginScene/Commands/SceneCommands.h>
#include <EditorPluginScene/Dialogs/DuplicateDlg.moc.h>
#include <EditorPluginScene/Scene/SceneDocument.h>
#include <Foundation/IO/MemoryStream.h>
#include <Foundation/Math/Random.h>
#include <Foundation/Serialization/DdlSerializer.h>
#include <GuiFoundation/UIServices/UIServices.moc.h>
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


ezQtDuplicateDlg::ezQtDuplicateDlg(QWidget* pParent, ezSceneDocument* pSceneDoc)
  : ezQtDialog(pParent)
{
  m_pSceneDocument = pSceneDoc;

  setupUi(this);

  m_pButtonUndo = DefaultButtons->addButton("Undo", QDialogButtonBox::ActionRole);
  m_pButtonUndo->setAutoDefault(false);
  DefaultButtons->button(QDialogButtonBox::Apply)->setDefault(true);

  UpdateSourceObjects();

  // right-click resets a spin box to its default value, which is 0 unless set otherwise
  spinBoxCopies->SetIntMode(true);
  spinBoxCopies->setDefaultValue(1.0);
  SpinBoxRadius->setDefaultValue(1.0);

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

  // connect after the values are set, so that the preview is only updated once
  for (QDoubleSpinBox* pSpinBox : findChildren<QDoubleSpinBox*>())
  {
    connect(pSpinBox, &QDoubleSpinBox::valueChanged, this, [this]()
      { UpdatePreview(); });
  }

  UpdateUndoButton();
  PickNewRandomSeed();
  UpdatePreview();
}

ezQtDuplicateDlg::~ezQtDuplicateDlg()
{
  m_pSceneDocument->ShowTransformPreview({}, {});
}

void ezQtDuplicateDlg::on_DefaultButtons_clicked(QAbstractButton* pButton)
{
  if (pButton == DefaultButtons->button(QDialogButtonBox::Close))
  {
    reject();
  }

  if (pButton == DefaultButtons->button(QDialogButtonBox::Reset))
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

  if (pButton == DefaultButtons->button(QDialogButtonBox::Apply))
  {
    Apply();
  }

  if (pButton == m_pButtonUndo)
  {
    Undo();
  }
}

void ezQtDuplicateDlg::QueryUI()
{
  s_bGroupCopies = CheckBoxGroupCopies->isChecked();

  s_uiNumberOfCopies = static_cast<ezUInt32>(spinBoxCopies->value());

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
}

void ezQtDuplicateDlg::FillCommandSettings(ezDuplicateObjectsCommand& ref_cmd) const
{
  ref_cmd.m_uiNumberOfCopies = s_uiNumberOfCopies;
  ref_cmd.m_vAccumulativeTranslation = s_vTranslationStep;
  ref_cmd.m_vAccumulativeRotation = s_vRotationStep;
  ref_cmd.m_vRandomRotation = s_vRandomRotation;
  ref_cmd.m_vRandomTranslation = s_vRandomTranslation;
  ref_cmd.m_bGroupDuplicates = s_bGroupCopies;
  ref_cmd.m_iRevolveAxis = static_cast<ezInt8>(s_iRevolveAxis);
  ref_cmd.m_fRevolveRadius = s_fRevolveRadius;
  ref_cmd.m_RevolveStartAngle = ezAngle::MakeFromDegree((float)s_iRevolveStartAngle);
  ref_cmd.m_RevolveAngleStep = ezAngle::MakeFromDegree((float)s_iRevolveAngleStep);
  ref_cmd.m_uiRandomSeed = m_uiRandomSeed;
}

static ezUInt32 CountGameObjects(const ezDocumentObject* pObject)
{
  if (!pObject->GetTypeAccessor().GetType()->IsDerivedFrom<ezGameObject>())
    return 0;

  ezUInt32 uiCount = 1;
  for (const ezDocumentObject* pChild : pObject->GetChildren())
  {
    uiCount += CountGameObjects(pChild);
  }

  return uiCount;
}

void ezQtDuplicateDlg::Apply()
{
  QueryUI();

  {
    // safety measure against accidentally creating a huge amount of objects
    constexpr ezUInt32 uiConfirmThreshold = 1000;

    ezUInt32 uiNumObjects = 0;
    for (const ezDocumentObject* pObject : m_TopLevelObjects)
    {
      uiNumObjects += CountGameObjects(pObject);
    }

    const ezUInt64 uiNumNewObjects = (ezUInt64)uiNumObjects * s_uiNumberOfCopies;

    if (uiNumNewObjects > uiConfirmThreshold)
    {
      if (ezQtUiServices::MessageBoxQuestion(ezFmt("This creates {} new objects ({} objects x {} copies).\n\nContinue?", uiNumNewObjects, uiNumObjects, s_uiNumberOfCopies), QMessageBox::StandardButton::Yes | QMessageBox::StandardButton::No, QMessageBox::StandardButton::No, QMessageBox::StandardButton::No) != QMessageBox::StandardButton::Yes)
      {
        return;
      }
    }
  }

  ezMap<ezUuid, ezUuid> parents;

  ezAbstractObjectGraph graph;
  m_pSceneDocument->CopySelectedObjects(graph, &parents);

  ezStringBuilder temp, tmp1, tmp2;
  for (auto it = parents.GetIterator(); it.IsValid(); ++it)
  {
    temp.AppendFormat("{0}={1};", ezConversionUtils::ToString(it.Key(), tmp1), ezConversionUtils::ToString(it.Value(), tmp2));
  }

  // Serialize to string
  ezContiguousMemoryStreamStorage streamStorage;
  ezMemoryStreamWriter memoryWriter(&streamStorage);

  ezAbstractGraphDdlSerializer::Write(memoryWriter, &graph);
  memoryWriter.WriteBytes("\0", 1).IgnoreResult(); // null terminate

  ezDuplicateObjectsCommand cmd;
  cmd.m_sGraphTextFormat = (const char*)streamStorage.GetData();
  cmd.m_sParentNodes = temp;
  FillCommandSettings(cmd);

  auto history = m_pSceneDocument->GetCommandHistory();

  history->StartTransaction("Duplicate Special");

  if (history->AddCommand(cmd).Failed())
  {
    history->CancelTransaction();
  }
  else
  {
    history->FinishTransaction();
    ++m_uiActionsApplied;
  }

  UpdateUndoButton();

  // the command selects the duplicates in addition to the source objects, the next application duplicates all of them
  UpdateSourceObjects();

  // the random offsets should be different for the next application
  PickNewRandomSeed();
  UpdatePreview();
}

void ezQtDuplicateDlg::Undo()
{
  auto history = m_pSceneDocument->GetCommandHistory();

  if (m_uiActionsApplied > 0 && history->CanUndo())
  {
    --m_uiActionsApplied;
    history->Undo().IgnoreResult();
  }

  UpdateUndoButton();

  // undoing deselects the removed duplicates, which leaves the source objects of that application selected
  UpdateSourceObjects();
  UpdatePreview();
}

void ezQtDuplicateDlg::UpdateSourceObjects()
{
  m_TopLevelObjects.Clear();

  ezTempHybridArray<ezSelectionEntry, 64> selection;
  m_pSceneDocument->GetSelectionManager()->GetTopLevelSelection(selection);

  for (const ezSelectionEntry& entry : selection)
  {
    m_TopLevelObjects.PushBack(entry.m_pObject);
  }

  m_pSceneDocument->QueryObjectBounds(m_TopLevelObjects, m_ObjectBounds);

  SetupBoundingBoxHelper();
}

void ezQtDuplicateDlg::UpdateUndoButton()
{
  m_pButtonUndo->setEnabled(m_uiActionsApplied > 0 && m_pSceneDocument->GetCommandHistory()->CanUndo());
}

void ezQtDuplicateDlg::UpdatePreview()
{
  QueryUI();

  ezDuplicateObjectsCommand cmd;
  FillCommandSettings(cmd);

  ezTempHybridArray<ezDuplicateObjectsCommand::CopyOffset, 16> offsets;
  cmd.ComputeCopyOffsets(offsets);

  ezTempHybridArray<ezTransform, 64> sourceTransforms;
  for (const ezDocumentObject* pObject : m_TopLevelObjects)
  {
    sourceTransforms.PushBack(m_pSceneDocument->GetGlobalTransform(pObject));
  }

  // the indicators are drawn every frame, so very large previews are cut off
  constexpr ezUInt32 uiMaxIndicators = 10000;

  ezTempHybridArray<ezTransform, 64> transforms;
  ezTempHybridArray<ezBoundingBox, 64> bounds;

  for (const auto& offset : offsets)
  {
    for (ezUInt32 i = 0; i < m_TopLevelObjects.GetCount() && transforms.GetCount() < uiMaxIndicators; ++i)
    {
      ezTransform tGlobal = sourceTransforms[i];
      tGlobal.m_vPosition += offset.m_vPositionOffset;
      tGlobal.m_qRotation = offset.m_qRotation * tGlobal.m_qRotation;

      transforms.PushBack(tGlobal);
      bounds.PushBack(m_ObjectBounds[i]);
    }
  }

  m_pSceneDocument->ShowTransformPreview(transforms, bounds);
}

void ezQtDuplicateDlg::PickNewRandomSeed()
{
  ezRandom rng;
  rng.InitializeFromCurrentTime();

  // zero would make the command use the current time
  m_uiRandomSeed = ezMath::Max<ezUInt64>(rng.UInt(), 1);
}

void ezQtDuplicateDlg::SetupBoundingBoxHelper()
{
  ezBoundingBox worldBounds = ezBoundingBox::MakeInvalid();

  for (ezUInt32 i = 0; i < m_TopLevelObjects.GetCount(); ++i)
  {
    ezBoundingBox box = m_ObjectBounds[i];
    if (!box.IsValid())
      continue;

    box.TransformFromOrigin(m_pSceneDocument->GetGlobalTransform(m_TopLevelObjects[i]).GetAsMat4());
    worldBounds.ExpandToInclude(box);
  }

  const bool bHasBounds = worldBounds.IsValid();

  m_vBoundingBoxSize = bHasBounds ? worldBounds.GetExtents() : ezVec3::MakeZero();
  m_vBoundingBoxSize.x = ezMath::RoundToMultiple(m_vBoundingBoxSize.x, 0.01f);
  m_vBoundingBoxSize.y = ezMath::RoundToMultiple(m_vBoundingBoxSize.y, 0.01f);
  m_vBoundingBoxSize.z = ezMath::RoundToMultiple(m_vBoundingBoxSize.z, 0.01f);

  toolButtonTransX->setVisible(bHasBounds);
  toolButtonTransY->setVisible(bHasBounds);
  toolButtonTransZ->setVisible(bHasBounds);

  if (bHasBounds)
  {
    toolButtonTransX->setToolTip(QString("Cycle the X step through +%1, -%1 and 0 (size of the selection)").arg(m_vBoundingBoxSize.x));
    toolButtonTransY->setToolTip(QString("Cycle the Y step through +%1, -%1 and 0 (size of the selection)").arg(m_vBoundingBoxSize.y));
    toolButtonTransZ->setToolTip(QString("Cycle the Z step through +%1, -%1 and 0 (size of the selection)").arg(m_vBoundingBoxSize.z));
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
    UpdatePreview();
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
    UpdatePreview();
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
    UpdatePreview();
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
    UpdatePreview();
  }
}
