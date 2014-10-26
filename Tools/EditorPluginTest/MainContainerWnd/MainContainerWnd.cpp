#include <PCH.h>
#include <EditorFramework/EditorFramework.h>
#include <EditorPluginTest/MainContainerWnd/MainContainerWnd.moc.h>
#include <QMenuBar>
#include <QMenu>

ezMainContainerWnd::ezMainContainerWnd() : ezContainerWindow("Main")
{
  QMenu* pMenuSettings = menuBar()->addMenu("Settings");
  QAction* pAction = pMenuSettings->addAction("Plugins");

  EZ_VERIFY(connect(pAction, SIGNAL(triggered()), this, SLOT(OnMenuSettingsPlugins())) != nullptr, "signal/slot connection failed");
}

void ezMainContainerWnd::OnMenuSettingsPlugins()
{
  ezEditorFramework::ShowPluginConfigDialog();

}