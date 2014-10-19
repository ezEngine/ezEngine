#include <PCH.h>
#include <EditorPluginTest/EditorPluginTest.h>
#include <EditorPluginTest/Panels/TestPanel.moc.h>
#include <EditorPluginTest/Windows/TestWindow.moc.h>
#include <EditorFramework/EditorFramework.h>
#include <EditorFramework/GUI/RawPropertyGridWidget.h>
#include <EditorFramework/GUI/RawDocumentTreeWidget.moc.h>
#include <ToolsFoundation/Reflection/ToolsReflectionUtils.h>
#include <ToolsFoundation/Reflection/ReflectedTypeManager.h>
#include <EditorPluginTest/Objects/TestObject.h>
#include <EditorPluginTest/Objects/TestObjectManager.h>
#include <EditorPluginTest/Widgets/TestObjectCreator.moc.h>
#include <EditorPluginTest/Document/TestDocument.h>
#include <qmainwindow.h>
#include <QMessageBox>

ezTestPanel* pPanel = nullptr;
ezTestPanel* pPanelTree = nullptr;
ezTestPanel* pPanelCreator = nullptr;
ezTestWindow* pWindow = nullptr;
ezRawPropertyGridWidget* pProps = nullptr;
ezRawDocumentTreeWidget* pTreeWidget = nullptr;
ezTestObject* g_pTestObject = nullptr;
ezTestObject2* g_pTestObject2 = nullptr;
ezTestObject* g_pTestObject3 = nullptr;
ezTestObject2* g_pTestObject4 = nullptr;
ezTestObjectCreatorWidget* g_pCreator = nullptr;
ezTestDocument* g_pDocument = nullptr;

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

  g_pDocument = new ezTestDocument();

  g_pTestObject = new ezTestObject(new ezTestObjectProperties);
  g_pTestObject3 = new ezTestObject(new ezTestObjectProperties);
  g_pTestObject2 = new ezTestObject2(ezReflectedTypeManager::GetTypeHandleByName("ezTestObjectProperties"));
  g_pTestObject4 = new ezTestObject2(ezReflectedTypeManager::GetTypeHandleByName("ezTestObjectProperties"));

  pPanel = new ezTestPanel(/*(QWidget*) pWindow);*/ezEditorFramework::GetMainWindow());
  pPanel->show();
  pPanel->setObjectName("PropertyPanel");

  pPanelTree = new ezTestPanel(/*(QWidget*) pWindow);*/ezEditorFramework::GetMainWindow());
  pPanelTree->show();
  pPanelTree->setObjectName("TreePanel");

  pPanelCreator = new ezTestPanel(/*(QWidget*) pWindow);*/ezEditorFramework::GetMainWindow());
  pPanelCreator->show();
  pPanelCreator->setObjectName("CreatorPanel");

  pProps = new ezRawPropertyGridWidget(g_pDocument, pPanel);//
  pPanel->setWidget(pProps);

  ((ezDocumentObjectTree*) g_pDocument->GetObjectTree())->AddObject(g_pTestObject, nullptr);
  ((ezDocumentObjectTree*) g_pDocument->GetObjectTree())->AddObject(g_pTestObject2, g_pTestObject);
  ((ezDocumentObjectTree*) g_pDocument->GetObjectTree())->AddObject(g_pTestObject3, g_pTestObject);
  ((ezDocumentObjectTree*) g_pDocument->GetObjectTree())->AddObject(g_pTestObject4, g_pTestObject3);

  pTreeWidget = new ezRawDocumentTreeWidget(pPanelTree, g_pDocument);
  pPanelTree->setWidget(pTreeWidget);

  g_pCreator = new ezTestObjectCreatorWidget(g_pDocument->GetObjectManager(), pPanelCreator);

  pPanelCreator->setWidget(g_pCreator);



  ezDeque<const ezDocumentObjectBase*> sel;
  sel.PushBack(g_pTestObject2);

  pProps->SetSelection(sel);

  /*pWindow*/ezEditorFramework::GetMainWindow()->addDockWidget(Qt::DockWidgetArea::RightDockWidgetArea, pPanel);
  ezEditorFramework::GetMainWindow()->addDockWidget(Qt::DockWidgetArea::LeftDockWidgetArea, pPanelTree);
  ezEditorFramework::GetMainWindow()->addDockWidget(Qt::DockWidgetArea::LeftDockWidgetArea, pPanelCreator);

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

  ezEditorFramework::GetMainWindow()->removeDockWidget(pPanelTree);

  pPanelTree->setParent(nullptr);
  delete pPanelTree;

  delete pWindow;
}

void OnEditorEvent(const ezEditorFramework::EditorEvent& e)
{
  switch (e.m_Type)
  {
  case ezEditorFramework::EditorEvent::EventType::AfterOpenProject:
    {
      ezEditorFramework::GetSettings(ezEditorFramework::SettingsCategory::Project, "TestPlugin").RegisterValueString("Legen", "dary");
      auto& sel = g_pDocument->GetSelectionManager()->GetSelection();

      while (!g_pDocument->GetSelectionManager()->GetSelection().IsEmpty())
      {
        // TODO cast cast cast !!!
        ((ezDocumentObjectTree*)g_pDocument->GetObjectTree())->RemoveObject((ezDocumentObjectBase*) g_pDocument->GetSelectionManager()->GetSelection()[0]);
      }
      //g_pObjectTree->MoveObject(g_pTestObject2, nullptr);
      //pProps->ClearSelection();
    }
    break;
  case ezEditorFramework::EditorEvent::EventType::AfterOpenScene:
    {
      ezEditorFramework::GetSettings(ezEditorFramework::SettingsCategory::Scene, "TestPlugin").RegisterValueString("Legen2", "dary2");

      //g_pObjectTree->MoveObject(g_pTestObject2, g_pTestObject);

      //ezHybridArray<ezDocumentObjectBase*, 32> sel;
      //sel.PushBack(g_pTestObject2);

      //pProps->SetSelection(sel);

    }
    break; 
  }
}

ezPlugin g_Plugin(false, OnLoadPlugin, OnUnloadPlugin);

EZ_DYNAMIC_PLUGIN_IMPLEMENTATION(ezEditorPluginTest);
