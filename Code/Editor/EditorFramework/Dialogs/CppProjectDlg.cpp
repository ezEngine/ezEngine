#include <EditorFramework/EditorFrameworkPCH.h>

#include <EditorFramework/Dialogs/CppProjectDlg.moc.h>
#include <EditorFramework/EditorApp/EditorApp.moc.h>

ezQtCppProjectDlg::ezQtCppProjectDlg(QWidget* parent)
  : QDialog(parent)
{
  setupUi(this);
}

void ezQtCppProjectDlg::on_Result_accepted()
{


  accept();
}

void ezQtCppProjectDlg::on_Result_rejected()
{
  reject();
}
