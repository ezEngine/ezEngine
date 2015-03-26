#include <GuiFoundation/PCH.h>
#include <GuiFoundation/Dialogs/ModifiedDocumentsDlg.moc.h>
#include <ToolsFoundation/Project/ToolsProject.h>

ezModifiedDocumentsDlg::ezModifiedDocumentsDlg(QWidget* parent, const ezHybridArray<ezDocumentBase*, 32>& ModifiedDocs) : QDialog(parent)
{
  m_ModifiedDocs = ModifiedDocs;

  setupUi(this);

  TableDocuments->blockSignals(true);

  TableDocuments->setRowCount(m_ModifiedDocs.GetCount());
  
  QStringList Headers;
  Headers.append("");
  Headers.append(" Type ");
  Headers.append(" Document ");

  TableDocuments->setColumnCount(Headers.size());

  TableDocuments->setSelectionBehavior(QAbstractItemView::SelectionBehavior::SelectRows);
  TableDocuments->setHorizontalHeaderLabels(Headers);
  TableDocuments->horizontalHeader()->show();
  TableDocuments->setSortingEnabled(true);
  EZ_VERIFY(connect(TableDocuments, SIGNAL(currentCellChanged(int, int, int, int)), this, SLOT(SlotSelectionChanged(int, int, int, int))) != nullptr, "signal/slot connection failed");

  ezStringBuilder sPath = ezPathUtils::GetFileDirectory(ezToolsProject::GetInstance()->GetProjectPath());
  ezInt32 iTrimStart = sPath.GetCharacterCount();

  ezInt32 iRow = 0;
  for (ezDocumentBase* pDoc : m_ModifiedDocs)
  {
    ezStringBuilder sText = pDoc->GetDocumentPath();
    sText.Shrink(iTrimStart, 0);

    QPushButton* pButtonSave = new QPushButton(QLatin1String("Save"));
    EZ_VERIFY(connect(pButtonSave, SIGNAL(clicked()), this, SLOT(SlotSaveDocument())) != nullptr, "signal/slot connection failed");

    pButtonSave->setProperty("document", qVariantFromValue((void*) pDoc));

    pButtonSave->setMinimumWidth(100);
    pButtonSave->setMaximumWidth(100);

    TableDocuments->setCellWidget(iRow, 0, pButtonSave);

    QTableWidgetItem* pItem0 = new QTableWidgetItem();
    pItem0->setData(Qt::DisplayRole, QVariant(pDoc->GetDocumentTypeDisplayString()));
    TableDocuments->setItem(iRow, 1, pItem0);

    QTableWidgetItem* pItem1 = new QTableWidgetItem();
    pItem1->setData(Qt::DisplayRole, QVariant(sText.GetData()));
    TableDocuments->setItem(iRow, 2, pItem1);

    ++iRow;
  }

  TableDocuments->resizeColumnsToContents();
  TableDocuments->blockSignals(false);
}

ezResult ezModifiedDocumentsDlg::SaveDocument(ezDocumentBase* pDoc)
{
  if (pDoc->SaveDocument().m_Result.Failed())
  {
    // ... TODO

    return EZ_FAILURE;
  }

  return EZ_SUCCESS;
}

void ezModifiedDocumentsDlg::SlotSaveDocument()
{
  QPushButton* pButtonSave = qobject_cast<QPushButton*>(sender());

  if (!pButtonSave)
    return;

  ezDocumentBase* pDoc = (ezDocumentBase*) pButtonSave->property("document").value<void*>();

  SaveDocument(pDoc);

  pButtonSave->setEnabled(pDoc->IsModified());
}

void ezModifiedDocumentsDlg::SlotSelectionChanged(int currentRow, int currentColumn, int previousRow, int previousColumn)
{
  QPushButton* pButtonSave = qobject_cast<QPushButton*>(TableDocuments->cellWidget(currentRow, 0));
  
  if (!pButtonSave)
    return;

  ezDocumentBase* pDoc = (ezDocumentBase*) pButtonSave->property("document").value<void*>();

  pDoc->EnsureVisible();
}

void ezModifiedDocumentsDlg::on_ButtonSaveSelected_clicked()
{
  for (ezDocumentBase* pDoc : m_ModifiedDocs)
  {
    if (SaveDocument(pDoc).Failed())
      return;
  }

  accept();
}

void ezModifiedDocumentsDlg::on_ButtonDontSave_clicked()
{
  accept();
}

void ezModifiedDocumentsDlg::on_ButtonCancel_clicked()
{
  reject();
}


