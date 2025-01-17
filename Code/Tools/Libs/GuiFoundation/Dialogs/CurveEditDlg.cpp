#include <GuiFoundation/GuiFoundationPCH.h>

#include <GuiFoundation/Dialogs/CurveEditDlg.moc.h>
#include <GuiFoundation/Widgets/Curve1DEditorWidget.moc.h>
#include <GuiFoundation/Widgets/CurveEditData.h>
#include <ToolsFoundation/Document/Document.h>
#include <ToolsFoundation/Object/ObjectAccessorBase.h>

QByteArray ezQtCurveEditDlg::s_LastDialogGeometry;

ezQtCurveEditDlg::ezQtCurveEditDlg(ezObjectAccessorBase* pObjectAccessor, const ezDocumentObject* pCurveObject, QWidget* pParent)
  : QDialog(pParent)
{
  m_pObjectAccessor = pObjectAccessor;
  m_pCurveObject = pCurveObject;

  setupUi(this);

  ezQtCurve1DEditorWidget* pEdit = CurveEditor;

  connect(pEdit, &ezQtCurve1DEditorWidget::CpMovedEvent, this, &ezQtCurveEditDlg::OnCpMovedEvent);
  connect(pEdit, &ezQtCurve1DEditorWidget::CpDeletedEvent, this, &ezQtCurveEditDlg::OnCpDeletedEvent);
  connect(pEdit, &ezQtCurve1DEditorWidget::TangentMovedEvent, this, &ezQtCurveEditDlg::OnTangentMovedEvent);
  connect(pEdit, &ezQtCurve1DEditorWidget::InsertCpEvent, this, &ezQtCurveEditDlg::OnInsertCpEvent);
  connect(pEdit, &ezQtCurve1DEditorWidget::TangentLinkEvent, this, &ezQtCurveEditDlg::OnTangentLinkEvent);
  connect(pEdit, &ezQtCurve1DEditorWidget::CpTangentModeEvent, this, &ezQtCurveEditDlg::OnCpTangentModeEvent);
  connect(pEdit, &ezQtCurve1DEditorWidget::BeginCpChangesEvent, this, &ezQtCurveEditDlg::OnBeginCpChangesEvent);
  connect(pEdit, &ezQtCurve1DEditorWidget::EndCpChangesEvent, this, &ezQtCurveEditDlg::OnEndCpChangesEvent);
  connect(pEdit, &ezQtCurve1DEditorWidget::BeginOperationEvent, this, &ezQtCurveEditDlg::OnBeginOperationEvent);
  connect(pEdit, &ezQtCurve1DEditorWidget::EndOperationEvent, this, &ezQtCurveEditDlg::OnEndOperationEvent);

  m_pShortcutUndo = new QShortcut(QKeySequence("Ctrl+Z"), this);
  m_pShortcutRedo = new QShortcut(QKeySequence("Ctrl+Y"), this);

  connect(m_pShortcutUndo, &QShortcut::activated, this, &ezQtCurveEditDlg::on_actionUndo_triggered);
  connect(m_pShortcutRedo, &QShortcut::activated, this, &ezQtCurveEditDlg::on_actionRedo_triggered);

  m_Curves.m_Curves.PushBack(EZ_DEFAULT_NEW(ezSingleCurveData));

  RetrieveCurveState();

  m_uiActionsUndoBaseline = m_pObjectAccessor->GetObjectManager()->GetDocument()->GetCommandHistory()->GetUndoStackSize();
}

void ezQtCurveEditDlg::RetrieveCurveState()
{
  auto& curve = m_Curves.m_Curves.PeekBack();

  ezInt32 iNumPoints = 0;
  m_pObjectAccessor->GetCountByName(m_pCurveObject, "ControlPoints", iNumPoints).AssertSuccess();
  curve->m_ControlPoints.SetCount(iNumPoints);

  ezVariant v;

  // get a local representation of the curve once, so that we can update the preview more efficiently
  for (ezInt32 i = 0; i < iNumPoints; ++i)
  {
    const ezDocumentObject* pPoint = m_pObjectAccessor->GetChildObjectByName(m_pCurveObject, "ControlPoints", i);

    m_pObjectAccessor->GetValueByName(pPoint, "Tick", v).AssertSuccess();
    curve->m_ControlPoints[i].m_iTick = v.ConvertTo<ezInt32>();

    m_pObjectAccessor->GetValueByName(pPoint, "Value", v).AssertSuccess();
    curve->m_ControlPoints[i].m_fValue = v.ConvertTo<double>();

    m_pObjectAccessor->GetValueByName(pPoint, "LeftTangent", v).AssertSuccess();
    curve->m_ControlPoints[i].m_LeftTangent = v.ConvertTo<ezVec2>();

    m_pObjectAccessor->GetValueByName(pPoint, "RightTangent", v).AssertSuccess();
    curve->m_ControlPoints[i].m_RightTangent = v.ConvertTo<ezVec2>();

    m_pObjectAccessor->GetValueByName(pPoint, "Linked", v).AssertSuccess();
    curve->m_ControlPoints[i].m_bTangentsLinked = v.ConvertTo<bool>();

    m_pObjectAccessor->GetValueByName(pPoint, "LeftTangentMode", v).AssertSuccess();
    curve->m_ControlPoints[i].m_LeftTangentMode = (ezCurveTangentMode::Enum)v.ConvertTo<ezInt32>();

    m_pObjectAccessor->GetValueByName(pPoint, "RightTangentMode", v).AssertSuccess();
    curve->m_ControlPoints[i].m_RightTangentMode = (ezCurveTangentMode::Enum)v.ConvertTo<ezInt32>();
  }
}

ezQtCurveEditDlg::~ezQtCurveEditDlg()
{
  s_LastDialogGeometry = saveGeometry();
}

void ezQtCurveEditDlg::SetCurveColor(const ezColor& color)
{
  m_Curves.m_Curves.PeekBack()->m_CurveColor = color;
}

void ezQtCurveEditDlg::SetCurveExtents(double fLower, bool bLowerFixed, double fUpper, bool bUpperFixed)
{
  m_fLowerExtents = fLower;
  m_fUpperExtents = fUpper;
  m_bLowerFixed = bLowerFixed;
  m_bUpperFixed = bUpperFixed;
}

void ezQtCurveEditDlg::SetCurveRanges(double fLower, double fUpper)
{
  m_fLowerRange = fLower;
  m_fUpperRange = fUpper;
}

void ezQtCurveEditDlg::reject()
{
  // ignore
}

void ezQtCurveEditDlg::accept()
{
  // ignore
}

void ezQtCurveEditDlg::cancel()
{
  auto& cmd = *m_pObjectAccessor->GetObjectManager()->GetDocument()->GetCommandHistory();
  cmd.Undo(cmd.GetUndoStackSize() - m_uiActionsUndoBaseline).AssertSuccess();

  QDialog::reject();
}

void ezQtCurveEditDlg::UpdatePreview()
{
  ezQtCurve1DEditorWidget* pEdit = CurveEditor;
  pEdit->SetCurveExtents(m_fLowerExtents, m_fUpperExtents, m_bLowerFixed, m_bUpperFixed);
  pEdit->SetCurveRanges(m_fLowerRange, m_fUpperRange);
  pEdit->SetCurves(m_Curves);
}

void ezQtCurveEditDlg::closeEvent(QCloseEvent*)
{
  cancel();
}

void ezQtCurveEditDlg::showEvent(QShowEvent* e)
{
  QDialog::showEvent(e);

  UpdatePreview();
}

void ezQtCurveEditDlg::OnCpMovedEvent(ezUInt32 curveIdx, ezUInt32 cpIdx, ezInt64 iTickX, double newPosY)
{
  // update the local representation
  {
    auto& cp = m_Curves.m_Curves[curveIdx]->m_ControlPoints[cpIdx];

    if (cp.m_iTick != iTickX || cp.m_fValue != newPosY)
    {
      cp.m_iTick = iTickX;
      cp.m_fValue = newPosY;
    }
  }

  // update the actual object
  {
    const ezDocumentObject* pPoint = m_pObjectAccessor->GetChildObjectByName(m_pCurveObject, "ControlPoints", cpIdx);

    m_pObjectAccessor->SetValueByName(pPoint, "Tick", iTickX).AssertSuccess();
    m_pObjectAccessor->SetValueByName(pPoint, "Value", newPosY).AssertSuccess();
  }
}

void ezQtCurveEditDlg::OnCpDeletedEvent(ezUInt32 curveIdx, ezUInt32 cpIdx)
{
  // update the local representation
  {
    m_Curves.m_Curves[curveIdx]->m_ControlPoints.RemoveAtAndCopy(cpIdx);
  }

  // update the actual object
  {
    const ezDocumentObject* pPoint = m_pObjectAccessor->GetChildObjectByName(m_pCurveObject, "ControlPoints", cpIdx);
    m_pObjectAccessor->RemoveObject(pPoint).AssertSuccess();
  }
}

void ezQtCurveEditDlg::OnTangentMovedEvent(ezUInt32 curveIdx, ezUInt32 cpIdx, float newPosX, float newPosY, bool rightTangent)
{
  // update the local representation
  {
    auto& cp = m_Curves.m_Curves[curveIdx]->m_ControlPoints[cpIdx];

    if (rightTangent)
      cp.m_RightTangent.Set(newPosX, newPosY);
    else
      cp.m_LeftTangent.Set(newPosX, newPosY);
  }

  // update the actual object
  {
    const ezDocumentObject* pPoint = m_pObjectAccessor->GetChildObjectByName(m_pCurveObject, "ControlPoints", cpIdx);

    if (rightTangent)
      m_pObjectAccessor->SetValueByName(pPoint, "RightTangent", ezVec2(newPosX, newPosY)).AssertSuccess();
    else
      m_pObjectAccessor->SetValueByName(pPoint, "LeftTangent", ezVec2(newPosX, newPosY)).AssertSuccess();
  }
}

void ezQtCurveEditDlg::OnInsertCpEvent(ezUInt32 curveIdx, ezInt64 tickX, double value)
{
  // update the local representation
  {
    ezCurveControlPointData cp;
    cp.m_iTick = tickX;
    cp.m_fValue = value;

    m_Curves.m_Curves[curveIdx]->m_ControlPoints.PushBack(cp);
  }

  // update the actual object
  {
    ezUuid guid;
    m_pObjectAccessor->AddObjectByName(m_pCurveObject, "ControlPoints", -1, ezGetStaticRTTI<ezCurveControlPointData>(), guid).AssertSuccess();

    const ezDocumentObject* pPoint = m_pObjectAccessor->GetObject(guid);

    m_pObjectAccessor->SetValueByName(pPoint, "Tick", tickX).AssertSuccess();
    m_pObjectAccessor->SetValueByName(pPoint, "Value", value).AssertSuccess();
  }
}

void ezQtCurveEditDlg::OnTangentLinkEvent(ezUInt32 curveIdx, ezUInt32 cpIdx, bool bLink)
{
  // update the local representation
  {
    auto& cp = m_Curves.m_Curves[curveIdx]->m_ControlPoints[cpIdx];
    cp.m_bTangentsLinked = bLink;
  }

  // update the actual object
  {
    const ezDocumentObject* pPoint = m_pObjectAccessor->GetChildObjectByName(m_pCurveObject, "ControlPoints", cpIdx);

    m_pObjectAccessor->SetValueByName(pPoint, "Linked", bLink).AssertSuccess();
  }
}

void ezQtCurveEditDlg::OnCpTangentModeEvent(ezUInt32 curveIdx, ezUInt32 cpIdx, bool rightTangent, int mode)
{
  // update the local representation
  {
    auto& cp = m_Curves.m_Curves[curveIdx]->m_ControlPoints[cpIdx];

    if (rightTangent)
      cp.m_RightTangentMode = (ezCurveTangentMode::Enum)mode;
    else
      cp.m_LeftTangentMode = (ezCurveTangentMode::Enum)mode;
  }

  // update the actual object
  {
    const ezDocumentObject* pPoint = m_pObjectAccessor->GetChildObjectByName(m_pCurveObject, "ControlPoints", cpIdx);

    if (rightTangent)
      m_pObjectAccessor->SetValueByName(pPoint, "RightTangentMode", mode).AssertSuccess();
    else
      m_pObjectAccessor->SetValueByName(pPoint, "LeftTangentMode", mode).AssertSuccess();
  }
}

void ezQtCurveEditDlg::OnBeginCpChangesEvent(QString name)
{
  m_pObjectAccessor->StartTransaction(name.toUtf8().data());
}

void ezQtCurveEditDlg::OnEndCpChangesEvent()
{
  m_pObjectAccessor->FinishTransaction();

  UpdatePreview();
}

void ezQtCurveEditDlg::OnBeginOperationEvent(QString name)
{
  m_pObjectAccessor->BeginTemporaryCommands(name.toUtf8().data());
}

void ezQtCurveEditDlg::OnEndOperationEvent(bool commit)
{
  if (commit)
    m_pObjectAccessor->FinishTemporaryCommands();
  else
    m_pObjectAccessor->CancelTemporaryCommands();

  UpdatePreview();
}

void ezQtCurveEditDlg::on_actionUndo_triggered()
{
  auto& cmd = *m_pObjectAccessor->GetObjectManager()->GetDocument()->GetCommandHistory();

  if (cmd.CanUndo() && cmd.GetUndoStackSize() > m_uiActionsUndoBaseline)
  {
    cmd.Undo().IgnoreResult();

    RetrieveCurveState();
    UpdatePreview();
  }
}

void ezQtCurveEditDlg::on_actionRedo_triggered()
{
  auto& cmd = *m_pObjectAccessor->GetObjectManager()->GetDocument()->GetCommandHistory();

  if (cmd.CanRedo())
  {
    cmd.Redo().IgnoreResult();

    RetrieveCurveState();
    UpdatePreview();
  }
}

void ezQtCurveEditDlg::on_ButtonOk_clicked()
{
  QDialog::accept();
}

void ezQtCurveEditDlg::on_ButtonCancel_clicked()
{
  cancel();
}
