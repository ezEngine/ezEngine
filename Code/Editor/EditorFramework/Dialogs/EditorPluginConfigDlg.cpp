#include <EditorFrameworkPCH.h>

#include <EditorFramework/Dialogs/EditorPluginConfigDlg.moc.h>
#include <EditorFramework/EditorApp/EditorApp.moc.h>
#include <Foundation/IO/OSFile.h>
#include <GuiFoundation/UIServices/UIServices.moc.h>
#include <QMessageBox>

ezQtEditorPluginConfigDlg::ezQtEditorPluginConfigDlg(QWidget* parent)
    : QDialog(parent)
{
  setupUi(this);

  FillPluginList();
}

void ezQtEditorPluginConfigDlg::FillPluginList()
{
  ezPluginSet& Plugins = ezQtEditorApp::GetSingleton()->GetEditorPlugins();

  ListPlugins->blockSignals(true);

  for (auto it = Plugins.m_Plugins.GetIterator(); it.IsValid(); ++it)
  {
    QListWidgetItem* pItem = new QListWidgetItem();
    pItem->setFlags(Qt::ItemFlag::ItemIsEnabled | Qt::ItemFlag::ItemIsSelectable | Qt::ItemFlag::ItemIsUserCheckable);

    ezStringBuilder sText = it.Key();

    if (it.Value().m_bLoadCopy)
    {
      sText.Append(" (load copy)");
    }

    if (!it.Value().m_bAvailable)
    {
      sText.Append(" (missing)");

      pItem->setBackground(Qt::red);
    }
    else if (it.Value().m_bActive)
    {
      sText.Append(" (active)");
    }

    pItem->setText(sText.GetData());
    pItem->setData(Qt::UserRole + 1, QString(it.Key().GetData()));
    pItem->setCheckState(it.Value().m_bToBeLoaded ? Qt::CheckState::Checked : Qt::CheckState::Unchecked);
    ListPlugins->addItem(pItem);
  }

  ListPlugins->blockSignals(false);
}

void ezQtEditorPluginConfigDlg::on_ButtonOK_clicked()
{
  ezPluginSet& Plugins = ezQtEditorApp::GetSingleton()->GetEditorPlugins();

  bool bChange = false;

  for (int i = 0; i < ListPlugins->count(); ++i)
  {
    QListWidgetItem* pItem = ListPlugins->item(i);

    const bool bLoad = pItem->checkState() == Qt::CheckState::Checked;

    bool& ToBeLoaded = Plugins.m_Plugins[pItem->data(Qt::UserRole + 1).toString().toUtf8().data()].m_bToBeLoaded;

    if (ToBeLoaded != bLoad)
    {
      ToBeLoaded = bLoad;
      bChange = true;
    }
  }

  if (bChange)
  {
    ezQtEditorApp::GetSingleton()->StoreEditorPluginsToBeLoaded();
  }

  accept();
}

void ezQtEditorPluginConfigDlg::on_ButtonCancel_clicked()
{
  reject();
}
