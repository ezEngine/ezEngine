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
#include <EditorPluginTest/MainContainerWnd/MainContainerWnd.moc.h>
#include <qmainwindow.h>
#include <QMessageBox>

ezTestPanel* pPanel = nullptr;
ezTestPanel* pPanelTree = nullptr;
ezTestPanel* pPanelCreator = nullptr;
ezTestWindow* pWindow = nullptr;
ezRawPropertyGridWidget* pProps = nullptr;
ezRawDocumentTreeWidget* pTreeWidget = nullptr;
ezDocumentObjectBase* g_pTestObject = nullptr;
ezDocumentObjectBase* g_pTestObject2 = nullptr;
ezDocumentObjectBase* g_pTestObject3 = nullptr;
ezDocumentObjectBase* g_pTestObject4 = nullptr;
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

void OnEditorRequest(ezEditorFramework::EditorRequest& r)
{
  switch (r.m_Type)
  {
  case ezEditorFramework::EditorRequest::Type::RequestContainerWindow:
    {
      if (r.m_pResult == nullptr && r.m_sContainerName == "Main")
      {
        r.m_pResult = new ezMainContainerWnd();
      }
    }
    break;
  }
}

void OnLoadPlugin(bool bReloading)    
{
  ezEditorFramework::s_EditorRequests.AddEventHandler(ezDelegate<void (ezEditorFramework::EditorRequest&)>(OnEditorRequest));

  ezContainerWindow* pContainer = ezEditorFramework::GetContainerWindow("Main", true);
  EZ_ASSERT(pContainer != nullptr, "Failed to retrieve the main container window");

  RegisterType(ezGetStaticRTTI<ezTestEditorProperties>());
  RegisterType(ezGetStaticRTTI<ezTestObjectProperties>());
  
  g_pDocument = new ezTestDocument();

  g_pTestObject = ((ezDocumentObjectManagerBase*) g_pDocument->GetObjectManager())->CreateObject(ezReflectedTypeManager::GetTypeHandleByName(ezGetStaticRTTI<ezTestObjectProperties>()->GetTypeName()));
  g_pTestObject3 = ((ezDocumentObjectManagerBase*) g_pDocument->GetObjectManager())->CreateObject(ezReflectedTypeManager::GetTypeHandleByName(ezGetStaticRTTI<ezTestObjectProperties>()->GetTypeName()));
  g_pTestObject2 = ((ezDocumentObjectManagerBase*) g_pDocument->GetObjectManager())->CreateObject(ezReflectedTypeManager::GetTypeHandleByName(ezGetStaticRTTI<ezTestEditorProperties>()->GetTypeName()));
  g_pTestObject4 = ((ezDocumentObjectManagerBase*) g_pDocument->GetObjectManager())->CreateObject(ezReflectedTypeManager::GetTypeHandleByName(ezGetStaticRTTI<ezTestEditorProperties>()->GetTypeName()));

  pPanel = new ezTestPanel(pContainer);
  pPanel->show();
  pPanel->setObjectName("PropertyPanel");

  pPanelTree = new ezTestPanel(pContainer);
  pPanelTree->show();
  pPanelTree->setObjectName("TreePanel");

  pPanelCreator = new ezTestPanel(pContainer);
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

  pContainer->addDockWidget(Qt::DockWidgetArea::RightDockWidgetArea, pPanel);
  pContainer->addDockWidget(Qt::DockWidgetArea::LeftDockWidgetArea, pPanelTree);
  pContainer->addDockWidget(Qt::DockWidgetArea::LeftDockWidgetArea, pPanelCreator);

  ezEditorFramework::s_EditorEvents.AddEventHandler(OnEditorEvent);

  ezEditorFramework::GetSettings(ezEditorFramework::SettingsCategory::Editor, "TestPlugin").RegisterValueBool("Awesome", true);
  ezEditorFramework::GetSettings(ezEditorFramework::SettingsCategory::Editor, "TestPlugin").RegisterValueBool("SomeUserSetting", true, ezSettingsFlags::User);
  ezEditorFramework::GetSettings(ezEditorFramework::SettingsCategory::Project, "TestPlugin").RegisterValueString("Legen", "dary");

  if (ezEditorFramework::GetSettings(ezEditorFramework::SettingsCategory::Editor, "TestPlugin").GetValueBool("SomeUserSetting") == false)
    QMessageBox::information(pPanel, QLatin1String(""), QLatin1String(""), QMessageBox::StandardButton::Ok, QMessageBox::StandardButton::Ok);

  pContainer->AddDocumentWindow("ViewerMap.scene");
  pContainer->AddDocumentWindow("CityMap.scene");

  pContainer->RestoreWindowLayout();
  pContainer->show();
}

void OnUnloadPlugin(bool bReloading)  
{
  ezContainerWindow* pContainer = ezEditorFramework::GetContainerWindow("Main", false);
  if (pContainer)
  {
    pContainer->removeDockWidget(pPanel);
    pContainer->removeDockWidget(pPanelTree);
  }

  pPanel->setParent(nullptr);
  delete pPanel;

  pPanelTree->setParent(nullptr);
  delete pPanelTree;

  delete pWindow;

  ezEditorFramework::s_EditorRequests.RemoveEventHandler(ezDelegate<void (ezEditorFramework::EditorRequest&)>(OnEditorRequest));
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
