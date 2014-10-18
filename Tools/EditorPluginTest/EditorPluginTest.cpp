#include <PCH.h>
#include <EditorPluginTest/EditorPluginTest.h>
#include <EditorPluginTest/Panels/TestPanel.moc.h>
#include <EditorPluginTest/Windows/TestWindow.moc.h>
#include <EditorFramework/EditorFramework.h>
#include <qmainwindow.h>

ezTestPanel* pPanel = nullptr;
ezTestWindow* pWindow = nullptr;

void OnEditorEvent(const ezEditorFramework::EditorEvent& e);

void OnLoadPlugin(bool bReloading)    
{
  //pWindow = new ezTestWindow(nullptr);
  //pWindow->show();

  pPanel = new ezTestPanel(/*(QWidget*) pWindow);*/ezEditorFramework::GetMainWindow());
  pPanel->show();

  /*pWindow*/ezEditorFramework::GetMainWindow()->addDockWidget(Qt::DockWidgetArea::RightDockWidgetArea, pPanel);

  ezEditorFramework::s_EditorEvents.AddEventHandler(OnEditorEvent);

  ezEditorFramework::GetSettings(ezEditorFramework::SettingsCategory::Editor, "TestPlugin").RegisterValueBool("Awesome", true);
  ezEditorFramework::GetSettings(ezEditorFramework::SettingsCategory::Project, "TestPlugin").RegisterValueString("Legen", "dary");
}

void OnUnloadPlugin(bool bReloading)  
{
  ezEditorFramework::GetMainWindow()->removeDockWidget(pPanel);

  pPanel->setParent(nullptr);
  delete pPanel;

  delete pWindow;
}

void OnEditorEvent(const ezEditorFramework::EditorEvent& e)
{
  switch (e.m_Type)
  {
  case ezEditorFramework::EditorEvent::EventType::AfterOpenProject:
    {
    ezEditorFramework::GetSettings(ezEditorFramework::SettingsCategory::Project, "TestPlugin").RegisterValueString("Legen", "dary");
    }
    break;
  case ezEditorFramework::EditorEvent::EventType::AfterOpenScene:
    {
    ezEditorFramework::GetSettings(ezEditorFramework::SettingsCategory::Scene, "TestPlugin").RegisterValueString("Legen2", "dary2");
    }
    break; 
  }
}

ezPlugin g_Plugin(false, OnLoadPlugin, OnUnloadPlugin);

EZ_DYNAMIC_PLUGIN_IMPLEMENTATION(ezEditorPluginTest);
