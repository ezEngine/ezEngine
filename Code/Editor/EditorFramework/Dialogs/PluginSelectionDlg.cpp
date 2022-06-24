#include <EditorFramework/EditorFrameworkPCH.h>

#include <EditorFramework/Dialogs/PluginSelectionDlg.moc.h>
#include <Foundation/IO/OpenDdlWriter.h>

ezQtPluginSelectionDlg::ezQtPluginSelectionDlg(ezPluginBundleSet* pPluginSet, QWidget* parent)
  : QDialog(parent)
{
  setupUi(this);

  m_pPluginSet = pPluginSet;
  m_LocalPluginSet = *pPluginSet;

  PluginSelectionWidget->SetPluginSet(&m_LocalPluginSet);
}

ezQtPluginSelectionDlg::~ezQtPluginSelectionDlg() = default;

void ezQtPluginSelectionDlg::on_Buttons_clicked(QAbstractButton* pButton)
{
  if (Buttons->standardButton(pButton) == QDialogButtonBox::Ok)
  {
    PluginSelectionWidget->SyncStateToSet();

    if (!m_pPluginSet->IsStateEqual(m_LocalPluginSet))
    {
      *m_pPluginSet = m_LocalPluginSet;

      ezFileWriter file;
      file.Open(":project/Editor/PluginSelection.ddl").AssertSuccess();

      ezOpenDdlWriter ddl;
      ddl.SetOutputStream(&file);
      m_pPluginSet->WriteStateToDDL(ddl);

      ezQtEditorApp::GetSingleton()->AddRestartRequiredReason("The set of active plugins has changed.");
    }

    accept();
  }
  else
  {
    reject();
  }
}
