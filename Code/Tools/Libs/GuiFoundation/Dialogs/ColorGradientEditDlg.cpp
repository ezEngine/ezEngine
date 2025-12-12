#include <GuiFoundation/GuiFoundationPCH.h>

#include <Foundation/Tracks/ColorGradient.h>
#include <GuiFoundation/Dialogs/ColorGradientEditDlg.moc.h>
#include <GuiFoundation/Widgets/ColorGradientEditorWidget.moc.h>
#include <QShortcut>
#include <ToolsFoundation/Document/Document.h>
#include <ToolsFoundation/Object/ObjectAccessorBase.h>

QByteArray ezQtColorGradientEditDlg::s_LastDialogGeometry;

ezQtColorGradientEditDlg::ezQtColorGradientEditDlg(ezObjectAccessorBase* pObjectAccessor, const ezDocumentObject* pGradientObject, QWidget* pParent, ezStringView sTitle)
  : QDialog(pParent)
{
  m_pObjectAccessor = pObjectAccessor;
  m_pGradientObject = pGradientObject;

  setupUi(this);

  if (!sTitle.IsEmpty())
  {
    ezStringBuilder tmp;
    setWindowTitle(sTitle.GetData(tmp));
  }

  ezQtColorGradientEditorWidget* pEdit = GradientEditor;

  // Connect color CP signals
  connect(pEdit, &ezQtColorGradientEditorWidget::ColorCpAdded, this, &ezQtColorGradientEditDlg::OnColorCpAdded);
  connect(pEdit, &ezQtColorGradientEditorWidget::ColorCpMoved, this, &ezQtColorGradientEditDlg::OnColorCpMoved);
  connect(pEdit, &ezQtColorGradientEditorWidget::ColorCpDeleted, this, &ezQtColorGradientEditDlg::OnColorCpDeleted);
  connect(pEdit, &ezQtColorGradientEditorWidget::ColorCpChanged, this, &ezQtColorGradientEditDlg::OnColorCpChanged);

  // Connect alpha CP signals
  connect(pEdit, &ezQtColorGradientEditorWidget::AlphaCpAdded, this, &ezQtColorGradientEditDlg::OnAlphaCpAdded);
  connect(pEdit, &ezQtColorGradientEditorWidget::AlphaCpMoved, this, &ezQtColorGradientEditDlg::OnAlphaCpMoved);
  connect(pEdit, &ezQtColorGradientEditorWidget::AlphaCpDeleted, this, &ezQtColorGradientEditDlg::OnAlphaCpDeleted);
  connect(pEdit, &ezQtColorGradientEditorWidget::AlphaCpChanged, this, &ezQtColorGradientEditDlg::OnAlphaCpChanged);

  // Connect intensity CP signals
  connect(pEdit, &ezQtColorGradientEditorWidget::IntensityCpAdded, this, &ezQtColorGradientEditDlg::OnIntensityCpAdded);
  connect(pEdit, &ezQtColorGradientEditorWidget::IntensityCpMoved, this, &ezQtColorGradientEditDlg::OnIntensityCpMoved);
  connect(pEdit, &ezQtColorGradientEditorWidget::IntensityCpDeleted, this, &ezQtColorGradientEditDlg::OnIntensityCpDeleted);
  connect(pEdit, &ezQtColorGradientEditorWidget::IntensityCpChanged, this, &ezQtColorGradientEditDlg::OnIntensityCpChanged);

  // Connect operation boundaries
  connect(pEdit, &ezQtColorGradientEditorWidget::BeginOperation, this, &ezQtColorGradientEditDlg::OnBeginOperation);
  connect(pEdit, &ezQtColorGradientEditorWidget::EndOperation, this, &ezQtColorGradientEditDlg::OnEndOperation);

  // Connect utility functions
  connect(pEdit, &ezQtColorGradientEditorWidget::NormalizeRange, this, &ezQtColorGradientEditDlg::OnNormalizeRange);

  // Setup keyboard shortcuts
  m_pShortcutUndo = new QShortcut(QKeySequence("Ctrl+Z"), this);
  m_pShortcutRedo = new QShortcut(QKeySequence("Ctrl+Y"), this);

  connect(m_pShortcutUndo, &QShortcut::activated, this, &ezQtColorGradientEditDlg::on_actionUndo_triggered);
  connect(m_pShortcutRedo, &QShortcut::activated, this, &ezQtColorGradientEditDlg::on_actionRedo_triggered);

  RetrieveGradientState();

  m_uiActionsUndoBaseline = m_pObjectAccessor->GetObjectManager()->GetDocument()->GetCommandHistory()->GetUndoStackSize();

  UpdateUndoRedoState();
}

ezQtColorGradientEditDlg::~ezQtColorGradientEditDlg()
{
  s_LastDialogGeometry = saveGeometry();
}

void ezQtColorGradientEditDlg::RetrieveGradientState()
{
  m_Gradient.Clear();
  ezVariant v;

  // Retrieve ColorCPs
  ezInt32 iNumColorCPs = 0;
  m_pObjectAccessor->GetCountByName(m_pGradientObject, "ColorCPs", iNumColorCPs).AssertSuccess();

  for (ezInt32 i = 0; i < iNumColorCPs; ++i)
  {
    const ezDocumentObject* pCP = m_pObjectAccessor->GetChildObjectByName(m_pGradientObject, "ColorCPs", i);

    m_pObjectAccessor->GetValueByName(pCP, "Tick", v).AssertSuccess();
    ezInt64 tick = v.ConvertTo<ezInt64>();

    m_pObjectAccessor->GetValueByName(pCP, "Red", v).AssertSuccess();
    ezUInt8 r = v.ConvertTo<ezUInt8>();
    m_pObjectAccessor->GetValueByName(pCP, "Green", v).AssertSuccess();
    ezUInt8 g = v.ConvertTo<ezUInt8>();
    m_pObjectAccessor->GetValueByName(pCP, "Blue", v).AssertSuccess();
    ezUInt8 b = v.ConvertTo<ezUInt8>();

    m_Gradient.AddColorControlPoint(ezColorGradient::TickToTime(tick), ezColorGammaUB(r, g, b));
  }

  // Retrieve AlphaCPs
  ezInt32 iNumAlphaCPs = 0;
  m_pObjectAccessor->GetCountByName(m_pGradientObject, "AlphaCPs", iNumAlphaCPs).AssertSuccess();

  for (ezInt32 i = 0; i < iNumAlphaCPs; ++i)
  {
    const ezDocumentObject* pCP = m_pObjectAccessor->GetChildObjectByName(m_pGradientObject, "AlphaCPs", i);

    m_pObjectAccessor->GetValueByName(pCP, "Tick", v).AssertSuccess();
    ezInt64 tick = v.ConvertTo<ezInt64>();
    m_pObjectAccessor->GetValueByName(pCP, "Alpha", v).AssertSuccess();
    ezUInt8 alpha = v.ConvertTo<ezUInt8>();

    m_Gradient.AddAlphaControlPoint(ezColorGradient::TickToTime(tick), alpha);
  }

  // Retrieve IntensityCPs
  ezInt32 iNumIntensityCPs = 0;
  m_pObjectAccessor->GetCountByName(m_pGradientObject, "IntensityCPs", iNumIntensityCPs).AssertSuccess();

  for (ezInt32 i = 0; i < iNumIntensityCPs; ++i)
  {
    const ezDocumentObject* pCP = m_pObjectAccessor->GetChildObjectByName(m_pGradientObject, "IntensityCPs", i);

    m_pObjectAccessor->GetValueByName(pCP, "Tick", v).AssertSuccess();
    ezInt64 tick = v.ConvertTo<ezInt64>();
    m_pObjectAccessor->GetValueByName(pCP, "Intensity", v).AssertSuccess();
    float intensity = v.ConvertTo<float>();

    m_Gradient.AddIntensityControlPoint(ezColorGradient::TickToTime(tick), intensity);
  }
}

void ezQtColorGradientEditDlg::reject()
{
  // Ignore - use cancel() instead
}

void ezQtColorGradientEditDlg::accept()
{
  // Ignore - handled by OK button
}

void ezQtColorGradientEditDlg::cancel()
{
  auto& cmd = *m_pObjectAccessor->GetObjectManager()->GetDocument()->GetCommandHistory();
  cmd.Undo(cmd.GetUndoStackSize() - m_uiActionsUndoBaseline).AssertSuccess();

  QDialog::reject();
}

void ezQtColorGradientEditDlg::UpdatePreview()
{
  ezQtColorGradientEditorWidget* pEdit = GradientEditor;
  pEdit->SetColorGradient(m_Gradient);
}

void ezQtColorGradientEditDlg::closeEvent(QCloseEvent*)
{
  cancel();
}

void ezQtColorGradientEditDlg::showEvent(QShowEvent* e)
{
  QDialog::showEvent(e);
  UpdatePreview();
}

// Color CP handlers
void ezQtColorGradientEditDlg::OnColorCpAdded(double fPosX, const ezColorGammaUB& color)
{
  fPosX = ezColorGradient::SnapTimeTo(fPosX);

  // Update local representation
  m_Gradient.AddColorControlPoint(fPosX, color);

  // Update document object
  auto* history = m_pObjectAccessor->GetObjectManager()->GetDocument()->GetCommandHistory();
  history->StartTransaction("Add Color Control Point");

  ezUuid guid;
  m_pObjectAccessor->AddObjectByName(m_pGradientObject, "ColorCPs", -1, ezGetStaticRTTI<ezColorGradientColorCP>(), guid).AssertSuccess();

  const ezDocumentObject* pCP = m_pObjectAccessor->GetObject(guid);
  ezInt64 tick = ezColorGradient::TimeToTick(fPosX);

  m_pObjectAccessor->SetValueByName(pCP, "Tick", tick).AssertSuccess();
  m_pObjectAccessor->SetValueByName(pCP, "Red", color.r).AssertSuccess();
  m_pObjectAccessor->SetValueByName(pCP, "Green", color.g).AssertSuccess();
  m_pObjectAccessor->SetValueByName(pCP, "Blue", color.b).AssertSuccess();

  history->FinishTransaction();

  RetrieveGradientState();
  UpdatePreview();
  UpdateUndoRedoState();
}

void ezQtColorGradientEditDlg::OnColorCpMoved(ezInt32 iIndex, double fNewPosX)
{
  // Update local representation
  auto& cp = m_Gradient.ModifyColorControlPoint(iIndex);
  cp.m_iTick = ezColorGradient::SnapTimeToTick(fNewPosX);

  // Update document object
  const ezDocumentObject* pCP = m_pObjectAccessor->GetChildObjectByName(m_pGradientObject, "ColorCPs", iIndex);
  m_pObjectAccessor->SetValueByName(pCP, "Tick", cp.m_iTick).AssertSuccess();

  // Update the widget during drag
  UpdatePreview();
}

void ezQtColorGradientEditDlg::OnColorCpDeleted(ezInt32 iIndex)
{
  // Update document object
  auto* history = m_pObjectAccessor->GetObjectManager()->GetDocument()->GetCommandHistory();
  history->StartTransaction("Remove Control Point");

  const ezDocumentObject* pCP = m_pObjectAccessor->GetChildObjectByName(m_pGradientObject, "ColorCPs", iIndex);
  m_pObjectAccessor->RemoveObject(pCP).AssertSuccess();

  history->FinishTransaction();

  // Retrieve updated state
  RetrieveGradientState();
  UpdatePreview();
  UpdateUndoRedoState();
}

void ezQtColorGradientEditDlg::OnColorCpChanged(ezInt32 iIndex, const ezColorGammaUB& color)
{
  // Update local representation
  auto& cp = m_Gradient.ModifyColorControlPoint(iIndex);
  cp.m_GammaRed = color.r;
  cp.m_GammaGreen = color.g;
  cp.m_GammaBlue = color.b;

  // Update document object
  auto* history = m_pObjectAccessor->GetObjectManager()->GetDocument()->GetCommandHistory();
  history->StartTransaction("Change Color");

  const ezDocumentObject* pCP = m_pObjectAccessor->GetChildObjectByName(m_pGradientObject, "ColorCPs", iIndex);
  m_pObjectAccessor->SetValueByName(pCP, "Red", color.r).AssertSuccess();
  m_pObjectAccessor->SetValueByName(pCP, "Green", color.g).AssertSuccess();
  m_pObjectAccessor->SetValueByName(pCP, "Blue", color.b).AssertSuccess();

  history->FinishTransaction();

  RetrieveGradientState();
  UpdatePreview();
  UpdateUndoRedoState();
}

// Alpha CP handlers
void ezQtColorGradientEditDlg::OnAlphaCpAdded(double fPosX, ezUInt8 uiAlpha)
{
  fPosX = ezColorGradient::SnapTimeTo(fPosX);

  m_Gradient.AddAlphaControlPoint(fPosX, uiAlpha);

  auto* history = m_pObjectAccessor->GetObjectManager()->GetDocument()->GetCommandHistory();
  history->StartTransaction("Add Alpha Control Point");

  ezUuid guid;
  m_pObjectAccessor->AddObjectByName(m_pGradientObject, "AlphaCPs", -1, ezGetStaticRTTI<ezColorGradientAlphaCP>(), guid).AssertSuccess();

  const ezDocumentObject* pCP = m_pObjectAccessor->GetObject(guid);
  ezInt64 tick = ezColorGradient::SnapTimeToTick(fPosX);

  m_pObjectAccessor->SetValueByName(pCP, "Tick", tick).AssertSuccess();
  m_pObjectAccessor->SetValueByName(pCP, "Alpha", uiAlpha).AssertSuccess();

  history->FinishTransaction();

  RetrieveGradientState();
  UpdatePreview();
  UpdateUndoRedoState();
}

void ezQtColorGradientEditDlg::OnAlphaCpMoved(ezInt32 iIndex, double fNewPosX)
{
  auto& cp = m_Gradient.ModifyAlphaControlPoint(iIndex);
  cp.m_iTick = ezColorGradient::SnapTimeToTick(fNewPosX);

  const ezDocumentObject* pCP = m_pObjectAccessor->GetChildObjectByName(m_pGradientObject, "AlphaCPs", iIndex);
  m_pObjectAccessor->SetValueByName(pCP, "Tick", cp.m_iTick).AssertSuccess();

  // Update the widget during drag
  UpdatePreview();
}

void ezQtColorGradientEditDlg::OnAlphaCpDeleted(ezInt32 iIndex)
{
  auto* history = m_pObjectAccessor->GetObjectManager()->GetDocument()->GetCommandHistory();
  history->StartTransaction("Remove Control Point");

  const ezDocumentObject* pCP = m_pObjectAccessor->GetChildObjectByName(m_pGradientObject, "AlphaCPs", iIndex);
  m_pObjectAccessor->RemoveObject(pCP).AssertSuccess();

  history->FinishTransaction();

  RetrieveGradientState();
  UpdatePreview();
  UpdateUndoRedoState();
}

void ezQtColorGradientEditDlg::OnAlphaCpChanged(ezInt32 iIndex, ezUInt8 uiAlpha)
{
  auto& cp = m_Gradient.ModifyAlphaControlPoint(iIndex);
  cp.m_Alpha = uiAlpha;

  auto* history = m_pObjectAccessor->GetObjectManager()->GetDocument()->GetCommandHistory();
  history->StartTransaction("Change Alpha");

  const ezDocumentObject* pCP = m_pObjectAccessor->GetChildObjectByName(m_pGradientObject, "AlphaCPs", iIndex);
  m_pObjectAccessor->SetValueByName(pCP, "Alpha", uiAlpha).AssertSuccess();

  history->FinishTransaction();

  RetrieveGradientState();
  UpdatePreview();
  UpdateUndoRedoState();
}

// Intensity CP handlers
void ezQtColorGradientEditDlg::OnIntensityCpAdded(double fPosX, float fIntensity)
{
  fPosX = ezColorGradient::SnapTimeTo(fPosX);

  m_Gradient.AddIntensityControlPoint(fPosX, fIntensity);

  auto* history = m_pObjectAccessor->GetObjectManager()->GetDocument()->GetCommandHistory();
  history->StartTransaction("Add Intensity Control Point");

  ezUuid guid;
  m_pObjectAccessor->AddObjectByName(m_pGradientObject, "IntensityCPs", -1, ezGetStaticRTTI<ezColorGradientIntensityCP>(), guid).AssertSuccess();

  const ezDocumentObject* pCP = m_pObjectAccessor->GetObject(guid);
  ezInt64 tick = ezColorGradient::SnapTimeToTick(fPosX);

  m_pObjectAccessor->SetValueByName(pCP, "Tick", tick).AssertSuccess();
  m_pObjectAccessor->SetValueByName(pCP, "Intensity", fIntensity).AssertSuccess();

  history->FinishTransaction();

  RetrieveGradientState();
  UpdatePreview();
  UpdateUndoRedoState();
}

void ezQtColorGradientEditDlg::OnIntensityCpMoved(ezInt32 iIndex, double fNewPosX)
{
  auto& cp = m_Gradient.ModifyIntensityControlPoint(iIndex);
  cp.m_iTick = ezColorGradient::SnapTimeToTick(fNewPosX);

  const ezDocumentObject* pCP = m_pObjectAccessor->GetChildObjectByName(m_pGradientObject, "IntensityCPs", iIndex);
  m_pObjectAccessor->SetValueByName(pCP, "Tick", cp.m_iTick).AssertSuccess();

  // Update the widget during drag
  UpdatePreview();
}

void ezQtColorGradientEditDlg::OnIntensityCpDeleted(ezInt32 iIndex)
{
  auto* history = m_pObjectAccessor->GetObjectManager()->GetDocument()->GetCommandHistory();
  history->StartTransaction("Remove Control Point");

  const ezDocumentObject* pCP = m_pObjectAccessor->GetChildObjectByName(m_pGradientObject, "IntensityCPs", iIndex);
  m_pObjectAccessor->RemoveObject(pCP).AssertSuccess();

  history->FinishTransaction();

  RetrieveGradientState();
  UpdatePreview();
  UpdateUndoRedoState();
}

void ezQtColorGradientEditDlg::OnIntensityCpChanged(ezInt32 iIndex, float fIntensity)
{
  auto& cp = m_Gradient.ModifyIntensityControlPoint(iIndex);
  cp.m_Intensity = fIntensity;

  auto* history = m_pObjectAccessor->GetObjectManager()->GetDocument()->GetCommandHistory();
  history->StartTransaction("Change Intensity");

  const ezDocumentObject* pCP = m_pObjectAccessor->GetChildObjectByName(m_pGradientObject, "IntensityCPs", iIndex);
  m_pObjectAccessor->SetValueByName(pCP, "Intensity", fIntensity).AssertSuccess();

  history->FinishTransaction();

  RetrieveGradientState();
  UpdatePreview();
  UpdateUndoRedoState();
}

// Operation boundaries
void ezQtColorGradientEditDlg::OnBeginOperation()
{
  m_pObjectAccessor->BeginTemporaryCommands("Modify Gradient");
}

void ezQtColorGradientEditDlg::OnEndOperation(bool bCommit)
{
  if (bCommit)
  {
    m_pObjectAccessor->FinishTemporaryCommands();
    RetrieveGradientState();
    UpdatePreview();
  }
  else
  {
    m_pObjectAccessor->CancelTemporaryCommands();
    RetrieveGradientState();
    UpdatePreview();
  }

  UpdateUndoRedoState();
}

void ezQtColorGradientEditDlg::OnNormalizeRange()
{
  auto* history = m_pObjectAccessor->GetObjectManager()->GetDocument()->GetCommandHistory();
  history->StartTransaction("Normalize Gradient Range");

  // Find min/max tick values across all control points
  double fMinTime = ezMath::MaxValue<double>();
  double fMaxTime = -ezMath::MaxValue<double>();

  m_Gradient.GetExtents(fMinTime, fMaxTime);

  if (fMinTime >= fMaxTime || (fMinTime == 0.0 && fMaxTime == 1.0))
  {
    history->CancelTransaction();
    return; // Already normalized or invalid
  }

  const double fRange = fMaxTime - fMinTime;

  // Normalize all control points
  ezInt32 iNumCPs = 0;

  // Color CPs
  m_pObjectAccessor->GetCountByName(m_pGradientObject, "ColorCPs", iNumCPs).AssertSuccess();
  for (ezInt32 i = 0; i < iNumCPs; ++i)
  {
    const ezDocumentObject* pCP = m_pObjectAccessor->GetChildObjectByName(m_pGradientObject, "ColorCPs", i);
    ezVariant v;
    m_pObjectAccessor->GetValueByName(pCP, "Tick", v).AssertSuccess();
    ezInt64 oldTick = v.ConvertTo<ezInt64>();
    double oldTime = ezColorGradient::TickToTime(oldTick);
    double newTime = (oldTime - fMinTime) / fRange;
    ezInt64 newTick = ezColorGradient::TimeToTick(newTime);
    m_pObjectAccessor->SetValueByName(pCP, "Tick", newTick).AssertSuccess();
  }

  // Alpha CPs
  m_pObjectAccessor->GetCountByName(m_pGradientObject, "AlphaCPs", iNumCPs).AssertSuccess();
  for (ezInt32 i = 0; i < iNumCPs; ++i)
  {
    const ezDocumentObject* pCP = m_pObjectAccessor->GetChildObjectByName(m_pGradientObject, "AlphaCPs", i);
    ezVariant v;
    m_pObjectAccessor->GetValueByName(pCP, "Tick", v).AssertSuccess();
    ezInt64 oldTick = v.ConvertTo<ezInt64>();
    double oldTime = ezColorGradient::TickToTime(oldTick);
    double newTime = (oldTime - fMinTime) / fRange;
    ezInt64 newTick = ezColorGradient::TimeToTick(newTime);
    m_pObjectAccessor->SetValueByName(pCP, "Tick", newTick).AssertSuccess();
  }

  // Intensity CPs
  m_pObjectAccessor->GetCountByName(m_pGradientObject, "IntensityCPs", iNumCPs).AssertSuccess();
  for (ezInt32 i = 0; i < iNumCPs; ++i)
  {
    const ezDocumentObject* pCP = m_pObjectAccessor->GetChildObjectByName(m_pGradientObject, "IntensityCPs", i);
    ezVariant v;
    m_pObjectAccessor->GetValueByName(pCP, "Tick", v).AssertSuccess();
    ezInt64 oldTick = v.ConvertTo<ezInt64>();
    double oldTime = ezColorGradient::TickToTime(oldTick);
    double newTime = (oldTime - fMinTime) / fRange;
    ezInt64 newTick = ezColorGradient::TimeToTick(newTime);
    m_pObjectAccessor->SetValueByName(pCP, "Tick", newTick).AssertSuccess();
  }

  history->FinishTransaction();

  RetrieveGradientState();
  UpdatePreview();
  UpdateUndoRedoState();
}

void ezQtColorGradientEditDlg::UpdateUndoRedoState()
{
  auto& cmd = *m_pObjectAccessor->GetObjectManager()->GetDocument()->GetCommandHistory();

  ButtonUndo->setEnabled(cmd.CanUndo());
  ButtonRedo->setEnabled(cmd.CanRedo());
}

void ezQtColorGradientEditDlg::on_actionUndo_triggered()
{
  auto& cmd = *m_pObjectAccessor->GetObjectManager()->GetDocument()->GetCommandHistory();
  cmd.Undo().AssertSuccess();

  RetrieveGradientState();
  UpdatePreview();
  UpdateUndoRedoState();
}

void ezQtColorGradientEditDlg::on_actionRedo_triggered()
{
  auto& cmd = *m_pObjectAccessor->GetObjectManager()->GetDocument()->GetCommandHistory();
  cmd.Redo().AssertSuccess();

  RetrieveGradientState();
  UpdatePreview();
  UpdateUndoRedoState();
}

void ezQtColorGradientEditDlg::on_ButtonOk_clicked()
{
  QDialog::accept();
}

void ezQtColorGradientEditDlg::on_ButtonCancel_clicked()
{
  cancel();
}

void ezQtColorGradientEditDlg::on_ButtonUndo_clicked()
{
  on_actionUndo_triggered();
}

void ezQtColorGradientEditDlg::on_ButtonRedo_clicked()
{
  on_actionRedo_triggered();
}
