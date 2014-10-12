#include <PCH.h>
#include <Editor/Windows/EditorMainWnd.moc.h>
#include <Editor/Dialogs/PluginDlg.moc.h>
#include <EditorFramework/EditorFramework.h>

ezEditorMainWnd* ezEditorMainWnd::s_pWidget = nullptr;

ezEditorMainWnd::ezEditorMainWnd() : QMainWindow()
{
  s_pWidget = this;

  setupUi(this);
}

ezEditorMainWnd::~ezEditorMainWnd()
{
  s_pWidget = nullptr;
}

void ezEditorMainWnd::closeEvent(QCloseEvent* event) 
{
  ezEditorFramework::SaveWindowLayout();
}

void ezEditorMainWnd::on_ActionConfigurePlugins_triggered()
{
  PluginDlg dlg(this);
  dlg.exec();
}



