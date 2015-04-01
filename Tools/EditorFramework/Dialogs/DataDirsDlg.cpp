#include <PCH.h>
#include <EditorFramework/Dialogs/DataDirsDlg.moc.h>
#include <EditorFramework/EditorApp/EditorApp.moc.h>
#include <GuiFoundation/UIServices/UIServices.moc.h>
#include <Foundation/IO/OSFile.h>
#include <QMessageBox>
#include <QFileDialog>

DataDirsDlg::DataDirsDlg(QWidget* parent, const ezApplicationFileSystemConfig& cfg) : QDialog(parent)
{
  setupUi(this);

  m_Config = cfg;
  m_iSelection = -1;
  FillList();
}

void DataDirsDlg::FillList()
{
  if (m_Config.m_DataDirs.IsEmpty())
    m_iSelection = -1;

  if (m_iSelection != -1)
    m_iSelection = ezMath::Clamp<ezInt32>(m_iSelection, 0, m_Config.m_DataDirs.GetCount() - 1);

  ListDataDirs->blockSignals(true);

  ListDataDirs->clear();

  for (auto dd : m_Config.m_DataDirs)
  {
    QListWidgetItem* pItem = new QListWidgetItem(ListDataDirs);
    pItem->setFlags(Qt::ItemFlag::ItemIsEnabled | Qt::ItemFlag::ItemIsSelectable /*| Qt::ItemFlag::ItemIsUserCheckable*/);

    pItem->setText(QString::fromUtf8(dd.m_sRelativePath.GetData()));
  //  pItem->setCheckState(bToBeLoaded ? Qt::CheckState::Checked : Qt::CheckState::Unchecked);
    ListDataDirs->addItem(pItem);
  }

  ListDataDirs->setSelectionMode(QAbstractItemView::SelectionMode::SingleSelection);

  if (m_iSelection == -1)
    ListDataDirs->clearSelection();
  else
    ListDataDirs->setItemSelected(ListDataDirs->item(m_iSelection), true);

  ListDataDirs->blockSignals(false);

  on_ListDataDirs_itemSelectionChanged();
}

void DataDirsDlg::on_ButtonOK_clicked()
{
  accept();
}

void DataDirsDlg::on_ButtonCancel_clicked()
{
  reject();
}

void DataDirsDlg::on_ButtonUp_clicked()
{
  ezMath::Swap(m_Config.m_DataDirs[m_iSelection - 1], m_Config.m_DataDirs[m_iSelection]);
  --m_iSelection;

  FillList();
}

void DataDirsDlg::on_ButtonDown_clicked()
{
  ezMath::Swap(m_Config.m_DataDirs[m_iSelection], m_Config.m_DataDirs[m_iSelection + 1]);
  ++m_iSelection;

  FillList();
}

void DataDirsDlg::on_ButtonAdd_clicked()
{
  static QString sPreviousFolder;
  if (sPreviousFolder.isEmpty())
  {
    sPreviousFolder = QString::fromUtf8(ezToolsProject::GetInstance()->GetProjectPath().GetData());
  }

  QString sFolder = QFileDialog::getExistingDirectory(this, QLatin1String("Select Directory"), sPreviousFolder, QFileDialog::Option::ShowDirsOnly);

  if (sFolder.isEmpty())
    return;

  sPreviousFolder = sFolder;

  ezStringBuilder sProjectPath = ezToolsProject::GetInstance()->GetProjectPath();
  sProjectPath.PathParentDirectory();

  ezStringBuilder sRelPath = sFolder.toUtf8().data();
  sRelPath.MakeRelativeTo(sProjectPath);

  ezApplicationFileSystemConfig::DataDirConfig dd;
  dd.m_sRelativePath = sRelPath;
  dd.m_bWritable = false;
  m_Config.m_DataDirs.PushBack(dd);

  m_iSelection = m_Config.m_DataDirs.GetCount() - 1;

  FillList();
}

void DataDirsDlg::on_ButtonRemove_clicked()
{
  m_Config.m_DataDirs.RemoveAt(m_iSelection);

  FillList();
}

void DataDirsDlg::on_ListDataDirs_itemSelectionChanged()
{
  if (ListDataDirs->selectedItems().isEmpty())
    m_iSelection = -1;
  else
    m_iSelection = ListDataDirs->selectionModel()->selectedIndexes()[0].row();

  ButtonUp->setEnabled(m_iSelection > 0);
  ButtonDown->setEnabled(m_iSelection != -1 && m_iSelection < (ezInt32) m_Config.m_DataDirs.GetCount() - 1);
}


