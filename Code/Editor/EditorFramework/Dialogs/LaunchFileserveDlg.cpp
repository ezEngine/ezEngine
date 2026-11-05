#include <EditorFramework/EditorFrameworkPCH.h>

#include <EditorFramework/Dialogs/LaunchFileserveDlg.moc.h>
#include <EditorFramework/EditorApp/EditorApp.moc.h>
#include <QDesktopServices>

ezQtLaunchFileserveDlg::ezQtLaunchFileserveDlg(QWidget* pParent)
  : ezQtDialog(pParent)
{
  setupUi(this);
}

ezQtLaunchFileserveDlg::~ezQtLaunchFileserveDlg() = default;

void ezQtLaunchFileserveDlg::showEvent(QShowEvent* event)
{
  ezStringBuilder sCmdLine = ezQtEditorApp::GetSingleton()->BuildFileserveCommandLine();
  EditFileserve->setPlainText(sCmdLine.GetData());

  QDialog::showEvent(event);
}

void ezQtLaunchFileserveDlg::on_ButtonLaunch_clicked()
{
  ezQtEditorApp::GetSingleton()->RunFileserve();
  accept();
}

void ezQtLaunchFileserveDlg::on_HelpButton_clicked()
{
  QDesktopServices::openUrl(QUrl("https://ezengine.net/pages/docs/tools/fileserve.html"));
}
