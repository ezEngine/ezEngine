#include <PCH.h>
#include <EditorPluginScene/EditorPluginScene.h>
#include <EditorFramework/EditorApp/EditorApp.moc.h>
#include <EditorPluginScene/Objects/TestObjects.h>
#include <EditorPluginScene/Actions/GizmoActions.h>
#include <EditorPluginScene/Actions/SelectionActions.h>
#include <EditorPluginScene/Actions/SceneViewActions.h>
#include <EditorPluginScene/Scene/SceneDocumentWindow.moc.h>
#include <Core/World/GameObject.h>
#include <GuiFoundation/Action/ActionMapManager.h>
#include <EditorFramework/Actions/ProjectActions.h>
#include <EditorFramework/Actions/ViewActions.h>
#include <GuiFoundation/Action/StandardMenus.h>
#include <GuiFoundation/Action/DocumentActions.h>
#include <GuiFoundation/Action/CommandHistoryActions.h>
#include <GuiFoundation/Action/EditActions.h>
#include <CoreUtils/Localization/TranslationLookup.h>
#include <Panels/ScenegraphPanel/ScenegraphPanel.moc.h>

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
  ezTranslatorFromFiles::AddTranslationFile("ScenePlugin.txt");

  ezDocumentManagerBase::s_Events.AddEventHandler(ezMakeDelegate(OnDocumentManagerEvent));

  ezEditorApp::GetInstance()->RegisterPluginNameForSettings("ScenePlugin");

  ezGizmoActions::RegisterActions();
  ezSelectionActions::RegisterActions();
  ezRotateGizmoAction::RegisterActions();
  ezScaleGizmoAction::RegisterActions();
  ezTranslateGizmoAction::RegisterActions();
  ezSceneViewActions::RegisterActions();
  ezScenegraphPanel::RegisterActions();

  // Menu Bar
  ezActionMapManager::RegisterActionMap("EditorPluginScene_DocumentMenuBar");
  ezProjectActions::MapActions("EditorPluginScene_DocumentMenuBar");
  ezStandardMenus::MapActions("EditorPluginScene_DocumentMenuBar", ezStandardMenuTypes::File | ezStandardMenuTypes::Edit | ezStandardMenuTypes::Panels | ezStandardMenuTypes::Help);
  ezDocumentActions::MapActions("EditorPluginScene_DocumentMenuBar", "MenuFile", false);
  ezCommandHistoryActions::MapActions("EditorPluginScene_DocumentMenuBar", "MenuEdit");
  ezGizmoActions::MapActions("EditorPluginScene_DocumentMenuBar", "MenuEdit");
  ezSelectionActions::MapActions("EditorPluginScene_DocumentMenuBar", "MenuEdit");
  ezEditActions::MapActions("EditorPluginScene_DocumentMenuBar", "MenuEdit");
  ezRotateGizmoAction::MapActions("EditorPluginScene_DocumentMenuBar", "MenuEdit/GizmoCategory");
  ezScaleGizmoAction::MapActions("EditorPluginScene_DocumentMenuBar", "MenuEdit/GizmoCategory");
  ezTranslateGizmoAction::MapActions("EditorPluginScene_DocumentMenuBar", "MenuEdit/GizmoCategory");

  // Tool Bar
  ezActionMapManager::RegisterActionMap("EditorPluginScene_DocumentToolBar");
  ezDocumentActions::MapActions("EditorPluginScene_DocumentToolBar", "", true);
  ezCommandHistoryActions::MapActions("EditorPluginScene_DocumentToolBar", "");
  ezGizmoActions::MapActions("EditorPluginScene_DocumentToolBar", "");
  ezRotateGizmoAction::MapActions("EditorPluginScene_DocumentToolBar", "");
  ezScaleGizmoAction::MapActions("EditorPluginScene_DocumentToolBar", "");
  ezTranslateGizmoAction::MapActions("EditorPluginScene_DocumentToolBar", "");

  // View Tool Bar
  ezActionMapManager::RegisterActionMap("EditorPluginScene_ViewToolBar");
  ezViewActions::MapActions("EditorPluginScene_ViewToolBar", "");
  ezSceneViewActions::MapActions("EditorPluginScene_ViewToolBar", "");

}

void OnUnloadPlugin(bool bReloading)  
{
  ezDocumentManagerBase::s_Events.RemoveEventHandler(ezMakeDelegate(OnDocumentManagerEvent));

  ezGizmoActions::UnregisterActions();
  ezSelectionActions::UnregisterActions();
  ezRotateGizmoAction::UnregisterActions();
  ezScaleGizmoAction::UnregisterActions();
  ezTranslateGizmoAction::UnregisterActions();
  ezSceneViewActions::UnregisterActions();
}

ezPlugin g_Plugin(false, OnLoadPlugin, OnUnloadPlugin);

EZ_DYNAMIC_PLUGIN_IMPLEMENTATION(ezEditorPluginScene);
