#include <PCH.h>
#include <EditorPluginScene/EditorPluginScene.h>
#include <EditorFramework/EditorApp/EditorApp.moc.h>
#include <EditorPluginScene/Actions/GizmoActions.h>
#include <EditorPluginScene/Actions/SelectionActions.h>
#include <EditorPluginScene/Scene/SceneDocumentWindow.moc.h>
#include <EditorPluginScene/Visualizers/PointLightVisualizerAdapter.h>
#include <EditorPluginScene/Visualizers/SpotLightVisualizerAdapter.h>
#include <RendererCore/Lights/PointLightComponent.h>
#include <RendererCore/Lights/SpotLightComponent.h>
#include <Core/World/GameObject.h>
#include <GuiFoundation/Action/ActionMapManager.h>
#include <EditorFramework/Actions/ProjectActions.h>
#include <EditorFramework/Actions/ViewActions.h>
#include <EditorFramework/Visualizers/VisualizerAdapterRegistry.h>
#include <GuiFoundation/Action/StandardMenus.h>
#include <GuiFoundation/Action/DocumentActions.h>
#include <GuiFoundation/Action/CommandHistoryActions.h>
#include <GuiFoundation/Action/EditActions.h>
#include <Foundation/Strings/TranslationLookup.h>
#include <Panels/ScenegraphPanel/ScenegraphPanel.moc.h>
#include <Actions/SceneActions.h>
#include <ToolsFoundation/Settings/ToolsTagRegistry.h>
#include <EditorFramework/Actions/GameObjectDocumentActions.h>
#include <EditorFramework/Actions/GameObjectSelectionActions.h>
#include <EditorFramework/Actions/QuadViewActions.h>
#include <EditorFramework/Actions/TransformGizmoActions.h>

void OnDocumentManagerEvent(const ezDocumentManager::Event& e)
{
  switch (e.m_Type)
  {
  case ezDocumentManager::Event::Type::DocumentWindowRequested:
    {
      if (e.m_pDocument->GetDynamicRTTI() == ezGetStaticRTTI<ezSceneDocument>())
      {
        ezQtDocumentWindow* pDocWnd = new ezQtSceneDocumentWindow(static_cast<ezSceneDocument*>(e.m_pDocument));
      }
    }
    break;
  }
}

void ToolsProjectEventHandler(const ezEditorAppEvent& e)
{
  if (e.m_Type == ezEditorAppEvent::Type::BeforeApplyDataDirectories)
  {
    //ezQtEditorApp::GetSingleton()->AddPluginDataDirDependency(">sdk/Data/Base", "base");
  }
}

void OnLoadPlugin(bool bReloading)
{
  ezDocumentManager::s_Events.AddEventHandler(ezMakeDelegate(OnDocumentManagerEvent));

  ezQtEditorApp::GetSingleton()->AddRuntimePluginDependency("EditorPluginScene", "ezEnginePluginScene");

  ezQtEditorApp::GetSingleton()->m_Events.AddEventHandler(ToolsProjectEventHandler);

  // Add built in tags
  {
    ezToolsTagRegistry::AddTag(ezToolsTag("Default", "CastShadow", true));
    ezToolsTagRegistry::AddTag(ezToolsTag("Default", "AutoColMesh", true));
  }

  ezSelectionActions::RegisterActions();
  ezSceneGizmoActions::RegisterActions();
  ezSceneActions::RegisterActions();

  // Menu Bar
  ezActionMapManager::RegisterActionMap("EditorPluginScene_DocumentMenuBar");
  ezProjectActions::MapActions("EditorPluginScene_DocumentMenuBar");
  ezStandardMenus::MapActions("EditorPluginScene_DocumentMenuBar", ezStandardMenuTypes::File | ezStandardMenuTypes::Edit | ezStandardMenuTypes::Scene | ezStandardMenuTypes::Panels | ezStandardMenuTypes::View | ezStandardMenuTypes::Help);
  ezDocumentActions::MapActions("EditorPluginScene_DocumentMenuBar", "Menu.File", false);
  ezDocumentActions::MapToolsActions("EditorPluginScene_DocumentMenuBar", "Menu.Tools");
  ezCommandHistoryActions::MapActions("EditorPluginScene_DocumentMenuBar", "Menu.Edit");
  ezTransformGizmoActions::MapMenuActions("EditorPluginScene_DocumentMenuBar", "Menu.Edit");
  ezSceneGizmoActions::MapMenuActions("EditorPluginScene_DocumentMenuBar", "Menu.Edit");
  ezGameObjectSelectionActions::MapActions("EditorPluginScene_DocumentMenuBar", "Menu.Edit");
  ezSelectionActions::MapActions("EditorPluginScene_DocumentMenuBar", "Menu.Edit");
  ezEditActions::MapActions("EditorPluginScene_DocumentMenuBar", "Menu.Edit", true, true);
  ezRotateGizmoAction::MapActions("EditorPluginScene_DocumentMenuBar", "Menu.Edit/Gizmo.Menu");
  ezScaleGizmoAction::MapActions("EditorPluginScene_DocumentMenuBar", "Menu.Edit/Gizmo.Menu");
  ezTranslateGizmoAction::MapActions("EditorPluginScene_DocumentMenuBar", "Menu.Edit/Gizmo.Menu");
  ezGameObjectDocumentActions::MapMenuActions("EditorPluginScene_DocumentMenuBar", "Menu.View");
  ezGameObjectDocumentActions::MapMenuSimulationSpeed("EditorPluginScene_DocumentMenuBar", "Menu.Scene");
  ezSceneActions::MapMenuActions();

  // Tool Bar
  ezActionMapManager::RegisterActionMap("EditorPluginScene_DocumentToolBar");
  ezDocumentActions::MapActions("EditorPluginScene_DocumentToolBar", "", true);
  ezCommandHistoryActions::MapActions("EditorPluginScene_DocumentToolBar", "");
  ezTransformGizmoActions::MapToolbarActions("EditorPluginScene_DocumentToolBar", "");
  ezSceneGizmoActions::MapToolbarActions("EditorPluginScene_DocumentToolBar", "");
  ezRotateGizmoAction::MapActions("EditorPluginScene_DocumentToolBar", "");
  ezScaleGizmoAction::MapActions("EditorPluginScene_DocumentToolBar", "");
  ezTranslateGizmoAction::MapActions("EditorPluginScene_DocumentToolBar", "");
  ezGameObjectDocumentActions::MapToolbarActions("EditorPluginScene_DocumentToolBar", "");
  ezSceneActions::MapToolbarActions();

  // View Tool Bar
  ezActionMapManager::RegisterActionMap("EditorPluginScene_ViewToolBar");
  ezViewActions::MapActions("EditorPluginScene_ViewToolBar", "", ezViewActions::PerspectiveMode | ezViewActions::RenderMode | ezViewActions::UsageHint | ezViewActions::ActivateRemoteProcess);
  ezQuadViewActions::MapActions("EditorPluginScene_ViewToolBar", "");

  // Visualizers
  ezVisualizerAdapterRegistry::GetSingleton()->m_Factory.RegisterCreator(ezGetStaticRTTI<ezPointLightVisualizerAttribute>(), [](const ezRTTI* pRtti)->ezVisualizerAdapter* { return EZ_DEFAULT_NEW(ezPointLightVisualizerAdapter); });
  ezVisualizerAdapterRegistry::GetSingleton()->m_Factory.RegisterCreator(ezGetStaticRTTI<ezSpotLightVisualizerAttribute>(), [](const ezRTTI* pRtti)->ezVisualizerAdapter* { return EZ_DEFAULT_NEW(ezSpotLightVisualizerAdapter); });

  // SceneGraph Context Menu
  ezActionMapManager::RegisterActionMap("EditorPluginScene_ScenegraphContextMenu");
  ezGameObjectSelectionActions::MapContextMenuActions("EditorPluginScene_ScenegraphContextMenu", "");
  ezSelectionActions::MapContextMenuActions("EditorPluginScene_ScenegraphContextMenu", "");
  ezEditActions::MapContextMenuActions("EditorPluginScene_ScenegraphContextMenu", "");
}

void OnUnloadPlugin(bool bReloading)
{
  ezQtEditorApp::GetSingleton()->m_Events.RemoveEventHandler(ToolsProjectEventHandler);
  ezDocumentManager::s_Events.RemoveEventHandler(ezMakeDelegate(OnDocumentManagerEvent));

  ezSelectionActions::UnregisterActions();
  ezSceneGizmoActions::UnregisterActions();
  ezSceneActions::UnregisterActions();
}

ezPlugin g_Plugin(false, OnLoadPlugin, OnUnloadPlugin);

EZ_DYNAMIC_PLUGIN_IMPLEMENTATION(EZ_EDITORPLUGINSCENE_DLL, ezEditorPluginScene);
