#include <PCH.h>
#include <EditorFramework/Dialogs/PluginDlg.moc.h>
#include <EditorFramework/EditorApp.moc.h>
#include <EditorFramework/EditorGUI.moc.h>
#include <Foundation/IO/OSFile.h>
#include <QMessageBox>

void ezEditorApp::ShowPluginConfigDialog()
{
  PluginDlg dlg(nullptr);
  dlg.exec();
}

PluginDlg::PluginDlg(QWidget* parent) : QDialog(parent)
{
  setupUi(this);

  FillPluginList();
}

void PluginDlg::FillPluginList()
{
  const ezPluginSet& PluginsAvailable = ezEditorApp::GetInstance()->GetEditorPluginsAvailable();
  const ezPluginSet& PluginsActive = ezEditorApp::GetInstance()->GetEditorPluginsActive();
  const ezPluginSet& PluginsToBeLoaded = ezEditorApp::GetInstance()->GetEditorPluginsToBeLoaded();

  ListPlugins->blockSignals(true);

  for (auto it = PluginsAvailable.m_Plugins.GetIterator(); it.IsValid(); ++it)
  {
    QListWidgetItem* pItem = new QListWidgetItem();
    pItem->setFlags(Qt::ItemFlag::ItemIsEnabled | Qt::ItemFlag::ItemIsSelectable | Qt::ItemFlag::ItemIsUserCheckable);

    bool bActive = PluginsActive.m_Plugins.Find(it.Key()).IsValid();
    bool bToBeLoaded = PluginsToBeLoaded.m_Plugins.Find(it.Key()).IsValid();

    ezStringBuilder sText = it.Key();

    if (bActive)
      sText.Append(" (active)");

    pItem->setText(sText.GetData());
    pItem->setData(Qt::UserRole + 1, QString(it.Key().GetData()));
    pItem->setCheckState(bToBeLoaded ? Qt::CheckState::Checked : Qt::CheckState::Unchecked);
    ListPlugins->addItem(pItem);
  }

  ListPlugins->blockSignals(false);
}

void PluginDlg::on_ButtonOK_clicked()
{
  ezPluginSet ToBeLoaded;

  for (int i = 0; i < ListPlugins->count(); ++i)
  {
    QListWidgetItem* pItem = ListPlugins->item(i);

    if (pItem->checkState() == Qt::CheckState::Checked)
    {
      ToBeLoaded.m_Plugins.Insert(pItem->data(Qt::UserRole + 1).toString().toUtf8().data());
    }
  }

  const ezPluginSet& PluginsToBeLoaded = ezEditorApp::GetInstance()->GetEditorPluginsToBeLoaded();

  auto it1 = PluginsToBeLoaded.m_Plugins.GetIterator();
  auto it2 = ToBeLoaded.m_Plugins.GetIterator(); 

  bool bDifferent = false;

  for ( ; it1.IsValid() && it2.IsValid(); ++it1, ++it2)
  {
    if (it1.Key() != it2.Key())
    {
      bDifferent = true;
      break;
    }
  }

  if (it1.IsValid() != it2.IsValid())
    bDifferent = true;

  if (bDifferent)
  {
    ezEditorGUI::MessageBoxInformation("Plugins are only loaded at startup.\n\nYou need to restart the program for this change to take effect.");

    ezEditorApp::GetInstance()->SetEditorPluginsToBeLoaded(ToBeLoaded);
  }

  accept();
}

void PluginDlg::on_ButtonCancel_clicked()
{
  reject();
}


