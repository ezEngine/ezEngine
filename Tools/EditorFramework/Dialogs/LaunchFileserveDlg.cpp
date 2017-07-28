#include <PCH.h>
#include <EditorFramework/Dialogs/LaunchFileserveDlg.moc.h>
#include <EditorFramework/EditorApp/EditorApp.moc.h>
#include <GuiFoundation/UIServices/UIServices.moc.h>
#include <QSettings>

ezQtLaunchFileserveDlg::ezQtLaunchFileserveDlg(QWidget* parent) : QDialog(parent)
{
  setupUi(this);

}

ezQtLaunchFileserveDlg::~ezQtLaunchFileserveDlg()
{
}

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

