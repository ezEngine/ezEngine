#include <PCH.h>
#include <EditorPluginScene/EditorPluginScene.h>
#include <EditorFramework/EditorApp/EditorApp.moc.h>
#include <EditorFramework/GUI/RawPropertyGridWidget.h>
#include <EditorFramework/GUI/RawDocumentTreeWidget.moc.h>
#include <ToolsFoundation/Reflection/ToolsReflectionUtils.h>
#include <ToolsFoundation/Reflection/PhantomRttiManager.h>
#include <EditorPluginScene/Objects/TestObjects.h>
#include <EditorPluginScene/Objects/SceneObjectManager.h>
#include <EditorPluginScene/Panels/ObjectCreatorPanel/ObjectCreatorList.moc.h>
#include <EditorPluginScene/Scene/SceneDocument.h>
//#include <EditorPluginScene/Document/SceneDocumentManager.h>
#include <EditorPluginScene/Scene/SceneDocumentWindow.moc.h>
#include <Core/World/GameObject.h>
#include <qmainwindow.h>
#include <QMessageBox>
#include <GuiFoundation/Action/ActionMapManager.h>
#include <EditorFramework/Actions/ProjectActions.h>
#include <GuiFoundation/Action/StandardMenus.h>
#include <GuiFoundation/Action/DocumentActions.h>
#include <GuiFoundation/Action/CommandHistoryActions.h>
#include <GuiFoundation/DockWindow/DockWindow.moc.h>

void OnDocumentManagerEvent(const ezDocumentManagerBase::Event& e)
{
  switch (e.m_Type)
  {
  case ezDocumentManagerBase::Event::Type::DocumentWindowRequested:
    {
      if (e.m_pDocument->GetDynamicRTTI() == ezGetStaticRTTI<ezSceneDocument>())
      {
        ezDocumentWindow* pDocWnd = new ezSceneDocumentWindow(e.m_pDocument);

        {
          ezDockWindow* pPropertyPanel = new ezDockWindow(pDocWnd);
          pPropertyPanel->setObjectName("PropertyPanel");
          pPropertyPanel->setWindowTitle("Properties");
          pPropertyPanel->show();

          ezDockWindow* pPanelTree = new ezDockWindow(pDocWnd);
          pPanelTree->setObjectName("TreePanel");
          pPanelTree->setWindowTitle("Hierarchy");
          pPanelTree->show();

          ezDockWindow* pPanelCreator = new ezDockWindow(pDocWnd);
          pPanelCreator->setObjectName("CreatorPanel");
          pPanelCreator->setWindowTitle("Object Creator");
          pPanelCreator->show();

          ezRawPropertyGridWidget* pPropertyGrid = new ezRawPropertyGridWidget(e.m_pDocument, pPropertyPanel);
          pPropertyPanel->setWidget(pPropertyGrid);

          ezRawDocumentTreeWidget* pTreeWidget = new ezRawDocumentTreeWidget(pPanelTree, e.m_pDocument);
          pPanelTree->setWidget(pTreeWidget);

          ezObjectCreatorList* pCreatorWidget = new ezObjectCreatorList(e.m_pDocument->GetObjectManager(), pPanelCreator);
          pPanelCreator->setWidget(pCreatorWidget);

          //ezDocumentObjectBase* pTestObject1 = ((ezDocumentObjectManager*) e.m_pDocument->GetObjectManager())->CreateObject(ezRTTI::FindTypeByName(ezGetStaticRTTI<ezTestObjectProperties>()->GetTypeName()));
          //((ezDocumentObjectManager*) e.m_pDocument->GetObjectManager())->AddObject(pTestObject1, nullptr);

          pDocWnd->addDockWidget(Qt::DockWidgetArea::RightDockWidgetArea, pPropertyPanel);
          pDocWnd->addDockWidget(Qt::DockWidgetArea::LeftDockWidgetArea, pPanelTree);
          pDocWnd->addDockWidget(Qt::DockWidgetArea::LeftDockWidgetArea, pPanelCreator);
        }
      }
    }
    break;
  }
}

void OnLoadPlugin(bool bReloading)    
{
  ezToolsReflectionUtils::RegisterType(ezRTTI::FindTypeByName("ezAssetDocumentInfo"));
  ezToolsReflectionUtils::RegisterType(ezGetStaticRTTI<ezSceneObjectEditorProperties>());
  ezToolsReflectionUtils::RegisterType(ezGetStaticRTTI<ezTestObjectProperties>());
  ezToolsReflectionUtils::RegisterType(ezGetStaticRTTI<ezGameObject>());

  ezDocumentManagerBase::s_Events.AddEventHandler(ezMakeDelegate(OnDocumentManagerEvent));

  ezEditorApp::GetInstance()->RegisterPluginNameForSettings("ScenePlugin");

  // Menu Bar
  ezActionMapManager::RegisterActionMap("EditorTestDocumentMenuBar");
  ezProjectActions::MapActions("EditorTestDocumentMenuBar");
  ezStandardMenus::MapActions("EditorTestDocumentMenuBar", ezStandardMenuTypes::File | ezStandardMenuTypes::Edit);
  ezDocumentActions::MapActions("EditorTestDocumentMenuBar", "File", false);
  ezCommandHistoryActions::MapActions("EditorTestDocumentMenuBar", "Edit");

  // Tool Bar
  ezActionMapManager::RegisterActionMap("EditorTestDocumentToolBar");
  ezDocumentActions::MapActions("EditorTestDocumentToolBar", "", true);
  ezCommandHistoryActions::MapActions("EditorTestDocumentToolBar", "");
}

void OnUnloadPlugin(bool bReloading)  
{
  ezDocumentManagerBase::s_Events.RemoveEventHandler(ezMakeDelegate(OnDocumentManagerEvent));
}

ezPlugin g_Plugin(false, OnLoadPlugin, OnUnloadPlugin);

EZ_DYNAMIC_PLUGIN_IMPLEMENTATION(ezEditorPluginScene);
