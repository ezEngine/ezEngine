#include <PCH.h>
#include <EditorFramework/Dialogs/DocumentList.moc.h>
#include <EditorFramework/EditorFramework.h>

DocumentList::DocumentList(QWidget* parent, const ezHybridArray<ezDocumentBase*, 32>& ModifiedDocs) : QDialog(parent)
{
  setupUi(this);

  ListDocuments->blockSignals(true);

  for (ezDocumentBase* pDoc : ModifiedDocs)
  {
    QListWidgetItem* pItem = new QListWidgetItem();
    pItem->setFlags(Qt::ItemFlag::ItemIsEnabled | Qt::ItemFlag::ItemIsSelectable | Qt::ItemFlag::ItemIsUserCheckable);

    ezStringBuilder sText = pDoc->GetDocumentPath();

    pItem->setText(sText.GetData());
    //pItem->setData(Qt::UserRole + 1, pDoc);
    pItem->setCheckState(Qt::CheckState::Checked);
    ListDocuments->addItem(pItem);
  }

  ListDocuments->blockSignals(false);
}

void DocumentList::on_ButtonSaveAll_clicked()
{

  accept();
}

void DocumentList::on_ButtonDiscardAll_clicked()
{

  accept();
}

void DocumentList::on_ButtonCancel_clicked()
{
  reject();
}


