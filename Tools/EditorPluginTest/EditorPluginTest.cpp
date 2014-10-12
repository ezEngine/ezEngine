#include <PCH.h>
#include <EditorPluginTest/EditorPluginTest.h>
#include <EditorPluginTest/Panels/TestPanel.moc.h>
#include <EditorFramework/EditorFramework.h>
#include <qmainwindow.h>

ezTestPanel* pPanel = nullptr;

void OnLoadPlugin(bool bReloading)    
{
  pPanel = new ezTestPanel((QWidget*) ezEditorFramework::GetMainWindow());
  pPanel->show();

  ezEditorFramework::GetMainWindow()->addDockWidget(Qt::DockWidgetArea::RightDockWidgetArea, pPanel);
}

void OnUnloadPlugin(bool bReloading)  
{
  ezEditorFramework::GetMainWindow()->removeDockWidget(pPanel);

  pPanel->setParent(nullptr);
  delete pPanel;
}

ezPlugin g_Plugin(false, OnLoadPlugin, OnUnloadPlugin);

EZ_DYNAMIC_PLUGIN_IMPLEMENTATION(ezEditorPluginTest);
