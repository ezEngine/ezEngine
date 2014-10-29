#include <PCH.h>
#include <EditorPluginTest/EditorPluginTest.h>
#include <EditorPluginTest/Panels/TestPanel.moc.h>
#include <EditorFramework/EditorFramework.h>
#include <EditorFramework/GUI/RawPropertyGridWidget.h>
#include <EditorFramework/GUI/RawDocumentTreeWidget.moc.h>
#include <ToolsFoundation/Reflection/ToolsReflectionUtils.h>
#include <ToolsFoundation/Reflection/ReflectedTypeManager.h>
#include <EditorPluginTest/Objects/TestObject.h>
#include <EditorPluginTest/Objects/TestObjectManager.h>
#include <EditorPluginTest/Widgets/TestObjectCreator.moc.h>
#include <EditorPluginTest/Document/TestDocument.h>
#include <EditorPluginTest/Document/TestDocumentManager.h>
#include <qmainwindow.h>
#include <QMessageBox>

void RegisterType(const ezRTTI* pRtti)
{
  if (pRtti->GetParentType() != nullptr)
    RegisterType(pRtti->GetParentType());

  ezReflectedTypeDescriptor desc;
  ezToolsReflectionUtils::GetReflectedTypeDescriptorFromRtti(pRtti, desc);
  ezReflectedTypeManager::RegisterType(desc);
}

void OnPluginEvent(const ezPlugin::PluginEvent& e)
{
  // this function is a hack

  static bool bDone = false;

  if (e.m_EventType == ezPlugin::PluginEvent::Type::AfterPluginChanges && !bDone)
  {
    bDone = true;

    ezDocumentBase* pDoc;
    ezTestDocumentManager::s_pSingleton->CreateDocument("ezScene", "bla/blub1.scene", pDoc);

    ezDocumentBase* pDoc2;
    ezTestDocumentManager::s_pSingleton->CreateDocument("ezScene", "bla/blub2.scene", pDoc2);

    ezDocumentBase* pDoc3;
    ezTestDocumentManager::s_pSingleton->CreateDocument("ezScene", "bla/blub3.scene", pDoc3);
  }
}

void OnDocumentManagerEvent(const ezDocumentManagerBase::Event& e)
{
  switch (e.m_Type)
  {
  case ezDocumentManagerBase::Event::Type::DocumentOpened:
    {
      if (e.m_pDocument->GetDynamicRTTI() == ezGetStaticRTTI<ezTestDocument>())
      {
        ezDocumentWindow* pDocWnd = new ezDocumentWindow(e.m_pDocument->GetDocumentPath());

        {
          ezTestPanel* pPropertyPanel = new ezTestPanel(pDocWnd);
          pPropertyPanel->setObjectName("PropertyPanel");
          pPropertyPanel->setWindowTitle("Properties");
          pPropertyPanel->show();

          ezTestPanel* pPropertyPanel2 = new ezTestPanel(pDocWnd);
          pPropertyPanel2->setObjectName("PropertyPanel2");
          pPropertyPanel2->setWindowTitle("Properties 2");
          pPropertyPanel2->show();

          ezTestPanel* pPanelTree = new ezTestPanel(pDocWnd);
          pPanelTree->setObjectName("TreePanel");
          pPanelTree->setWindowTitle("Hierarchy");
          pPanelTree->show();

          ezTestPanel* pPanelCreator = new ezTestPanel(pDocWnd);
          pPanelCreator->setObjectName("CreatorPanel");
          pPanelCreator->setWindowTitle("Object Creator");
          pPanelCreator->show();

          ezRawPropertyGridWidget* pPropertyGrid = new ezRawPropertyGridWidget(e.m_pDocument, pPropertyPanel);
          pPropertyPanel->setWidget(pPropertyGrid);

          ezRawPropertyGridWidget* pPropertyGrid2 = new ezRawPropertyGridWidget(e.m_pDocument, pPropertyPanel2);
          pPropertyPanel2->setWidget(pPropertyGrid2);

          ezRawDocumentTreeWidget* pTreeWidget = new ezRawDocumentTreeWidget(pPanelTree, e.m_pDocument);
          pPanelTree->setWidget(pTreeWidget);

          ezTestObjectCreatorWidget* pCreatorWidget = new ezTestObjectCreatorWidget(e.m_pDocument->GetObjectManager(), pPanelCreator);
          pPanelCreator->setWidget(pCreatorWidget);

          //ezDocumentObjectBase* pTestObject1 = ((ezDocumentObjectManagerBase*) e.m_pDocument->GetObjectManager())->CreateObject(ezReflectedTypeManager::GetTypeHandleByName(ezGetStaticRTTI<ezTestObjectProperties>()->GetTypeName()));
          //((ezDocumentObjectTree*) e.m_pDocument->GetObjectTree())->AddObject(pTestObject1, nullptr);

          pDocWnd->addDockWidget(Qt::DockWidgetArea::RightDockWidgetArea, pPropertyPanel);
          pDocWnd->addDockWidget(Qt::DockWidgetArea::RightDockWidgetArea, pPropertyPanel2);
          pDocWnd->addDockWidget(Qt::DockWidgetArea::LeftDockWidgetArea, pPanelTree);
          pDocWnd->addDockWidget(Qt::DockWidgetArea::LeftDockWidgetArea, pPanelCreator);
        }

        ezEditorFramework::AddDocumentWindow(pDocWnd);
      }
    }
    break;
  }
}

void OnLoadPlugin(bool bReloading)    
{
  RegisterType(ezGetStaticRTTI<ezTestEditorProperties>());
  RegisterType(ezGetStaticRTTI<ezTestObjectProperties>());

  ezPlugin::s_PluginEvents.AddEventHandler(ezDelegate<void (const ezPlugin::PluginEvent&)>(OnPluginEvent));
  ezDocumentManagerBase::s_Events.AddEventHandler(ezDelegate<void (const ezDocumentManagerBase::Event&)>(OnDocumentManagerEvent));

  //g_pTestObject = ((ezDocumentObjectManagerBase*) g_pDocument->GetObjectManager())->CreateObject(ezReflectedTypeManager::GetTypeHandleByName(ezGetStaticRTTI<ezTestObjectProperties>()->GetTypeName()));
  //g_pTestObject3 = ((ezDocumentObjectManagerBase*) g_pDocument->GetObjectManager())->CreateObject(ezReflectedTypeManager::GetTypeHandleByName(ezGetStaticRTTI<ezTestObjectProperties>()->GetTypeName()));
  //g_pTestObject2 = ((ezDocumentObjectManagerBase*) g_pDocument->GetObjectManager())->CreateObject(ezReflectedTypeManager::GetTypeHandleByName(ezGetStaticRTTI<ezTestEditorProperties>()->GetTypeName()));
  //g_pTestObject4 = ((ezDocumentObjectManagerBase*) g_pDocument->GetObjectManager())->CreateObject(ezReflectedTypeManager::GetTypeHandleByName(ezGetStaticRTTI<ezTestEditorProperties>()->GetTypeName()));

  //((ezDocumentObjectTree*) g_pDocument->GetObjectTree())->AddObject(g_pTestObject, nullptr);
  //((ezDocumentObjectTree*) g_pDocument->GetObjectTree())->AddObject(g_pTestObject2, g_pTestObject);
  //((ezDocumentObjectTree*) g_pDocument->GetObjectTree())->AddObject(g_pTestObject3, g_pTestObject);
  //((ezDocumentObjectTree*) g_pDocument->GetObjectTree())->AddObject(g_pTestObject4, g_pTestObject3);

  //ezDeque<const ezDocumentObjectBase*> sel;
  //sel.PushBack(g_pTestObject2);

  //pProps->SetSelection(sel);
}

void OnUnloadPlugin(bool bReloading)  
{
  ezDocumentManagerBase::s_Events.RemoveEventHandler(ezDelegate<void (const ezDocumentManagerBase::Event&)>(OnDocumentManagerEvent));
  ezPlugin::s_PluginEvents.RemoveEventHandler(ezDelegate<void (const ezPlugin::PluginEvent&)>(OnPluginEvent));
}

ezPlugin g_Plugin(false, OnLoadPlugin, OnUnloadPlugin);

EZ_DYNAMIC_PLUGIN_IMPLEMENTATION(ezEditorPluginTest);
