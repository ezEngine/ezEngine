#include <PCH.h>
#include <EditorPluginScene/EditorPluginScene.h>
#include <EditorFramework/EditorApp/EditorApp.moc.h>
#include <EditorPluginScene/Objects/TestObjects.h>
#include <EditorPluginScene/Actions/GizmoActions.h>
#include <EditorPluginScene/Actions/SelectionActions.h>
#include <EditorPluginScene/Scene/SceneDocumentWindow.moc.h>
#include <Core/World/GameObject.h>
#include <GuiFoundation/Action/ActionMapManager.h>
#include <EditorFramework/Actions/ProjectActions.h>
#include <GuiFoundation/Action/StandardMenus.h>
#include <GuiFoundation/Action/DocumentActions.h>
#include <GuiFoundation/Action/CommandHistoryActions.h>

void OnDocumentManagerEvent(const ezDocumentManagerBase::Event& e)
{
  switch (e.m_Type)
  {
  case ezDocumentManagerBase::Event::Type::DocumentWindowRequested:
    {
      if (e.m_pDocument->GetDynamicRTTI() == ezGetStaticRTTI<ezSceneDocument>())
      {
        ezDocumentWindow* pDocWnd = new ezSceneDocumentWindow(e.m_pDocument);
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

  ezGizmoActions::RegisterActions();
  ezSelectionActions::RegisterActions();
  ezRotateGizmoAction::RegisterActions();

  // Menu Bar
  ezActionMapManager::RegisterActionMap("EditorPluginScene_DocumentMenuBar");
  ezProjectActions::MapActions("EditorPluginScene_DocumentMenuBar");
  ezStandardMenus::MapActions("EditorPluginScene_DocumentMenuBar", ezStandardMenuTypes::File | ezStandardMenuTypes::Edit | ezStandardMenuTypes::Panels | ezStandardMenuTypes::Help);
  ezDocumentActions::MapActions("EditorPluginScene_DocumentMenuBar", "File", false);
  ezCommandHistoryActions::MapActions("EditorPluginScene_DocumentMenuBar", "Edit");
  ezGizmoActions::MapActions("EditorPluginScene_DocumentMenuBar", "Edit");
  ezSelectionActions::MapActions("EditorPluginScene_DocumentMenuBar", "Edit");
  ezRotateGizmoAction::MapActions("EditorPluginScene_DocumentMenuBar", "Edit/GizmoCategory");

  // Tool Bar
  ezActionMapManager::RegisterActionMap("EditorPluginScene_DocumentToolBar");
  ezDocumentActions::MapActions("EditorPluginScene_DocumentToolBar", "", true);
  ezCommandHistoryActions::MapActions("EditorPluginScene_DocumentToolBar", "");
  ezGizmoActions::MapActions("EditorPluginScene_DocumentToolBar", "");
  ezRotateGizmoAction::MapActions("EditorPluginScene_DocumentToolBar", "");
}

void OnUnloadPlugin(bool bReloading)  
{
  ezDocumentManagerBase::s_Events.RemoveEventHandler(ezMakeDelegate(OnDocumentManagerEvent));

  ezGizmoActions::UnregisterActions();
  ezSelectionActions::UnregisterActions();
  ezRotateGizmoAction::UnregisterActions();
}

ezPlugin g_Plugin(false, OnLoadPlugin, OnUnloadPlugin);

EZ_DYNAMIC_PLUGIN_IMPLEMENTATION(ezEditorPluginScene);
