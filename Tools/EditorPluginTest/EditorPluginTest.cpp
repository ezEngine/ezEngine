#include <PCH.h>
#include <EditorPluginTest/EditorPluginTest.h>
#include <EditorPluginTest/Panels/TestPanel.moc.h>
#include <EditorFramework/EditorApp.moc.h>
#include <EditorFramework/GUI/RawPropertyGridWidget.h>
#include <EditorFramework/GUI/RawDocumentTreeWidget.moc.h>
#include <ToolsFoundation/Reflection/ToolsReflectionUtils.h>
#include <ToolsFoundation/Reflection/ReflectedTypeManager.h>
#include <EditorPluginTest/Objects/TestObject.h>
#include <EditorPluginTest/Objects/TestObjectManager.h>
#include <EditorPluginTest/Widgets/TestObjectCreator.moc.h>
#include <EditorPluginTest/Document/TestDocument.h>
#include <EditorPluginTest/Document/TestDocumentManager.h>
#include <EditorPluginTest/Document/TestDocumentWindow.moc.h>
#include <Core/World/GameObject.h>
#include <qmainwindow.h>
#include <QMessageBox>
#include <GuiFoundation/Action/ActionMapManager.h>
#include <EditorFramework/Actions/ProjectActions.h>

void RegisterType(const ezRTTI* pRtti)
{
  if (pRtti->GetParentType() != nullptr)
    RegisterType(pRtti->GetParentType());

  ezReflectedTypeDescriptor desc;

  if (pRtti->GetAllocator()->CanAllocate())
  {
    void* pObject = pRtti->GetAllocator()->Allocate();

    ezMemoryStreamStorage storage;
    ezMemoryStreamWriter writer(&storage);
    ezMemoryStreamReader reader(&storage);

    ezReflectionUtils::WriteObjectToJSON(writer, pRtti, pObject);

    pRtti->GetAllocator()->Deallocate(pObject);

    desc.m_sDefaultInitialization.ReadAll(reader);
  }
  
  ezToolsReflectionUtils::GetReflectedTypeDescriptorFromRtti(pRtti, desc);
  ezReflectedTypeManager::RegisterType(desc);
}

void OnDocumentManagerEvent(const ezDocumentManagerBase::Event& e)
{
  switch (e.m_Type)
  {
  case ezDocumentManagerBase::Event::Type::DocumentOpened:
    {
      if (e.m_pDocument->GetDynamicRTTI() == ezGetStaticRTTI<ezTestDocument>())
      {
        ezDocumentWindow* pDocWnd = new ezTestDocumentWindow(e.m_pDocument);

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

        ezEditorApp::GetInstance()->AddDocumentWindow(pDocWnd);
      }
    }
    break;
  }
}

void OnLoadPlugin(bool bReloading)    
{
  RegisterType(ezGetStaticRTTI<ezEnumBase>());
  RegisterType(ezGetStaticRTTI<ezExampleEnum>());
  RegisterType(ezGetStaticRTTI<ezExampleBitflags>());
  RegisterType(ezGetStaticRTTI<ezTestEditorProperties>());
  RegisterType(ezGetStaticRTTI<ezTestObjectProperties>());
  RegisterType(ezGetStaticRTTI<ezGameObject>());

  // TODO: HACK: Need to find a place to register this poor guy.
  RegisterType(ezGetStaticRTTI<ezDocumentInfo>());

  ezDocumentManagerBase::s_Events.AddEventHandler(ezMakeDelegate(OnDocumentManagerEvent));

  ezEditorApp::GetInstance()->RegisterPluginNameForSettings("TestPlugin");


  ezActionMapManager::RegisterActionMap("EditorTestDocumentMenuModel");
  ezProjectActions::MapActions("EditorTestDocumentMenuModel");
}

void OnUnloadPlugin(bool bReloading)  
{
  ezDocumentManagerBase::s_Events.RemoveEventHandler(ezMakeDelegate(OnDocumentManagerEvent));
}

ezPlugin g_Plugin(false, OnLoadPlugin, OnUnloadPlugin);

EZ_DYNAMIC_PLUGIN_IMPLEMENTATION(ezEditorPluginTest);
