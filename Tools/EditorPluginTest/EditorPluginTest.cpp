#include <PCH.h>
#include <EditorPluginTest/EditorPluginTest.h>
#include <EditorPluginTest/Panels/TestPanel.moc.h>
#include <EditorPluginTest/Windows/TestWindow.moc.h>
#include <EditorFramework/EditorFramework.h>
#include <EditorFramework/GUI/RawPropertyGridWidget.h>
#include <ToolsFoundation/Reflection/ToolsReflectionUtils.h>
#include <ToolsFoundation/Reflection/ReflectedTypeManager.h>
#include <EditorPluginTest/Objects/TestObject.h>
#include <qmainwindow.h>
#include <QMessageBox>

ezTestPanel* pPanel = nullptr;
ezTestWindow* pWindow = nullptr;
ezRawPropertyGridWidget* pProps = nullptr;
ezTestObject* g_pTestObject = nullptr;

void OnEditorEvent(const ezEditorFramework::EditorEvent& e);

void RegisterType(const ezRTTI* pRtti)
{
  if (pRtti->GetParentType() != nullptr)
    RegisterType(pRtti->GetParentType());

  ezReflectedTypeDescriptor desc;
  ezToolsReflectionUtils::GetReflectedTypeDescriptorFromRtti(pRtti, desc);
  ezReflectedTypeManager::RegisterType(desc);
}

void OnLoadPlugin(bool bReloading)    
{
  RegisterType(ezGetStaticRTTI<ezTestEditorProperties>());
  RegisterType(ezGetStaticRTTI<ezTestObjectProperties>());
  

  //pWindow = new ezTestWindow(nullptr);
  //pWindow->show();

  pPanel = new ezTestPanel(/*(QWidget*) pWindow);*/ezEditorFramework::GetMainWindow());
  pPanel->show();

  pProps = new ezRawPropertyGridWidget(nullptr);//
  pPanel->setWidget(pProps);

  g_pTestObject = new ezTestObject;

  ezHybridArray<ezDocumentObjectBase*, 32> sel;
  sel.PushBack(g_pTestObject);

  pProps->SetSelection(sel);

  /*pWindow*/ezEditorFramework::GetMainWindow()->addDockWidget(Qt::DockWidgetArea::RightDockWidgetArea, pPanel);

  ezEditorFramework::s_EditorEvents.AddEventHandler(OnEditorEvent);

  ezEditorFramework::GetSettings(ezEditorFramework::SettingsCategory::Editor, "TestPlugin").RegisterValueBool("Awesome", true);
  ezEditorFramework::GetSettings(ezEditorFramework::SettingsCategory::Editor, "TestPlugin").RegisterValueBool("SomeUserSetting", true, ezSettingsFlags::User);
  ezEditorFramework::GetSettings(ezEditorFramework::SettingsCategory::Project, "TestPlugin").RegisterValueString("Legen", "dary");

  if (ezEditorFramework::GetSettings(ezEditorFramework::SettingsCategory::Editor, "TestPlugin").GetValueBool("SomeUserSetting") == false)
    QMessageBox::information(pPanel, QLatin1String(""), QLatin1String(""), QMessageBox::StandardButton::Ok, QMessageBox::StandardButton::Ok);
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

      pProps->ClearSelection();
    }
    break;
  case ezEditorFramework::EditorEvent::EventType::AfterOpenScene:
    {
      ezEditorFramework::GetSettings(ezEditorFramework::SettingsCategory::Scene, "TestPlugin").RegisterValueString("Legen2", "dary2");

      ezHybridArray<ezDocumentObjectBase*, 32> sel;
      sel.PushBack(g_pTestObject);

      pProps->SetSelection(sel);

    }
    break; 
  }
}

ezPlugin g_Plugin(false, OnLoadPlugin, OnUnloadPlugin);

EZ_DYNAMIC_PLUGIN_IMPLEMENTATION(ezEditorPluginTest);
