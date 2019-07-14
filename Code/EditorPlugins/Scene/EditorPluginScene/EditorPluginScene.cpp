#include <EditorPluginScenePCH.h>

#include <Actions/SceneActions.h>
#include <Core/World/GameObject.h>
#include <EditorFramework/Actions/GameObjectDocumentActions.h>
#include <EditorFramework/Actions/GameObjectSelectionActions.h>
#include <EditorFramework/Actions/ProjectActions.h>
#include <EditorFramework/Actions/QuadViewActions.h>
#include <EditorFramework/Actions/TransformGizmoActions.h>
#include <EditorFramework/Actions/ViewActions.h>
#include <EditorFramework/Assets/AssetCurator.h>
#include <EditorFramework/EditorApp/EditorApp.moc.h>
#include <EditorFramework/Visualizers/VisualizerAdapterRegistry.h>
#include <EditorPluginScene/Actions/GizmoActions.h>
#include <EditorPluginScene/Actions/SelectionActions.h>
#include <EditorPluginScene/Scene/SceneDocumentWindow.moc.h>
#include <EditorPluginScene/Visualizers/PointLightVisualizerAdapter.h>
#include <EditorPluginScene/Visualizers/SpotLightVisualizerAdapter.h>
#include <Foundation/Strings/TranslationLookup.h>
#include <GameEngine/Configuration/RendererProfileConfigs.h>
#include <GuiFoundation/Action/ActionMapManager.h>
#include <GuiFoundation/Action/CommandHistoryActions.h>
#include <GuiFoundation/Action/DocumentActions.h>
#include <GuiFoundation/Action/EditActions.h>
#include <GuiFoundation/Action/StandardMenus.h>
#include <GuiFoundation/PropertyGrid/PropertyMetaState.h>
#include <GuiFoundation/UIServices/DynamicStringEnum.h>
#include <Panels/ScenegraphPanel/ScenegraphPanel.moc.h>
#include <RendererCore/Lights/PointLightComponent.h>
#include <RendererCore/Lights/SpotLightComponent.h>
#include <ToolsFoundation/Settings/ToolsTagRegistry.h>

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
    // ezQtEditorApp::GetSingleton()->AddPluginDataDirDependency(">sdk/Data/Base", "base");
  }
}

void AssetCuratorEventHandler(const ezAssetCuratorEvent& e)
{
  if (e.m_Type == ezAssetCuratorEvent::Type::ActivePlatformChanged)
  {
    ezSet<ezString> allCamPipes;

    auto& dynEnum = ezDynamicStringEnum::GetDynamicEnum("CameraPipelines");
    dynEnum.Clear();

    for (ezUInt32 profileIdx = 0; profileIdx < ezAssetCurator::GetSingleton()->GetNumAssetProfiles(); ++profileIdx)
    {
      const ezPlatformProfile* pProfile = ezAssetCurator::GetSingleton()->GetAssetProfile(profileIdx);

      const ezRenderPipelineProfileConfig* pConfig = pProfile->GetTypeConfig<ezRenderPipelineProfileConfig>();

      for (auto it = pConfig->m_CameraPipelines.GetIterator(); it.IsValid(); ++it)
      {
        dynEnum.AddValidValue(it.Key(), true);
      }
    }
  }
}

void ezCameraComponent_PropertyMetaStateEventHandler(ezPropertyMetaStateEvent& e);

void OnLoadPlugin(bool bReloading)
{
  ezDocumentManager::s_Events.AddEventHandler(ezMakeDelegate(OnDocumentManagerEvent));

  ezQtEditorApp::GetSingleton()->AddRuntimePluginDependency("EditorPluginScene", "ezEnginePluginScene");

  ezQtEditorApp::GetSingleton()->m_Events.AddEventHandler(ToolsProjectEventHandler);

  ezAssetCurator::GetSingleton()->m_Events.AddEventHandler(AssetCuratorEventHandler);

  // Add built in tags
  {
    ezToolsTagRegistry::AddTag(ezToolsTag("Default", "CastShadow", true));
    ezToolsTagRegistry::AddTag(ezToolsTag("Default", "AutoColMesh", true));
    ezToolsTagRegistry::AddTag(ezToolsTag("Default", "SkyLight", true));
  }

  ezSelectionActions::RegisterActions();
  ezSceneGizmoActions::RegisterActions();
  ezSceneActions::RegisterActions();

  // Menu Bar
  ezActionMapManager::RegisterActionMap("EditorPluginScene_DocumentMenuBar");
  ezProjectActions::MapActions("EditorPluginScene_DocumentMenuBar");
  ezStandardMenus::MapActions("EditorPluginScene_DocumentMenuBar", ezStandardMenuTypes::File | ezStandardMenuTypes::Edit |
                                                                     ezStandardMenuTypes::Scene | ezStandardMenuTypes::Panels |
                                                                     ezStandardMenuTypes::View | ezStandardMenuTypes::Help);
  ezDocumentActions::MapActions("EditorPluginScene_DocumentMenuBar", "Menu.File", false);
  ezDocumentActions::MapToolsActions("EditorPluginScene_DocumentMenuBar", "Menu.Tools");
  ezCommandHistoryActions::MapActions("EditorPluginScene_DocumentMenuBar", "Menu.Edit");
  ezTransformGizmoActions::MapMenuActions("EditorPluginScene_DocumentMenuBar", "Menu.Edit");
  ezSceneGizmoActions::MapMenuActions("EditorPluginScene_DocumentMenuBar", "Menu.Edit");
  ezGameObjectSelectionActions::MapActions("EditorPluginScene_DocumentMenuBar", "Menu.Edit");
  ezSelectionActions::MapActions("EditorPluginScene_DocumentMenuBar", "Menu.Edit");
  ezEditActions::MapActions("EditorPluginScene_DocumentMenuBar", "Menu.Edit", true, true);
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
  ezGameObjectDocumentActions::MapToolbarActions("EditorPluginScene_DocumentToolBar", "");
  ezSceneActions::MapToolbarActions();

  // View Tool Bar
  ezActionMapManager::RegisterActionMap("EditorPluginScene_ViewToolBar");
  ezViewActions::MapActions("EditorPluginScene_ViewToolBar", "",
    ezViewActions::PerspectiveMode | ezViewActions::RenderMode | ezViewActions::UsageHint | ezViewActions::ActivateRemoteProcess);
  ezQuadViewActions::MapActions("EditorPluginScene_ViewToolBar", "");

  // Visualizers
  ezVisualizerAdapterRegistry::GetSingleton()->m_Factory.RegisterCreator(ezGetStaticRTTI<ezPointLightVisualizerAttribute>(),
    [](const ezRTTI* pRtti) -> ezVisualizerAdapter* { return EZ_DEFAULT_NEW(ezPointLightVisualizerAdapter); });
  ezVisualizerAdapterRegistry::GetSingleton()->m_Factory.RegisterCreator(ezGetStaticRTTI<ezSpotLightVisualizerAttribute>(),
    [](const ezRTTI* pRtti) -> ezVisualizerAdapter* { return EZ_DEFAULT_NEW(ezSpotLightVisualizerAdapter); });

  // SceneGraph Context Menu
  ezActionMapManager::RegisterActionMap("EditorPluginScene_ScenegraphContextMenu");
  ezGameObjectSelectionActions::MapContextMenuActions("EditorPluginScene_ScenegraphContextMenu", "");
  ezSelectionActions::MapContextMenuActions("EditorPluginScene_ScenegraphContextMenu", "");
  ezEditActions::MapContextMenuActions("EditorPluginScene_ScenegraphContextMenu", "");

  // component property meta states
  ezPropertyMetaState::GetSingleton()->m_Events.AddEventHandler(ezCameraComponent_PropertyMetaStateEventHandler);
}

void OnUnloadPlugin(bool bReloading)
{
  ezDocumentManager::s_Events.RemoveEventHandler(ezMakeDelegate(OnDocumentManagerEvent));
  ezQtEditorApp::GetSingleton()->m_Events.RemoveEventHandler(ToolsProjectEventHandler);
  ezAssetCurator::GetSingleton()->m_Events.RemoveEventHandler(AssetCuratorEventHandler);
  ezPropertyMetaState::GetSingleton()->m_Events.RemoveEventHandler(ezCameraComponent_PropertyMetaStateEventHandler);


  ezSelectionActions::UnregisterActions();
  ezSceneGizmoActions::UnregisterActions();
  ezSceneActions::UnregisterActions();
}

ezPlugin g_Plugin(false, OnLoadPlugin, OnUnloadPlugin);


void ezCameraComponent_PropertyMetaStateEventHandler(ezPropertyMetaStateEvent& e)
{
  static const ezRTTI* pRtti = ezRTTI::FindTypeByName("ezCameraComponent");

  if (e.m_pObject->GetTypeAccessor().GetType() != pRtti)
    return;

  const ezInt64 usage = e.m_pObject->GetTypeAccessor().GetValue("UsageHint").ConvertTo<ezInt64>();
  const bool isRenderTarget = (usage == 3); // ezCameraUsageHint::RenderTarget

  auto& props = *e.m_pPropertyStates;

  props["RenderTarget"].m_Visibility = isRenderTarget ? ezPropertyUiState::Default : ezPropertyUiState::Disabled;
  props["RenderTargetOffset"].m_Visibility = isRenderTarget ? ezPropertyUiState::Default : ezPropertyUiState::Invisible;
  props["RenderTargetSize"].m_Visibility = isRenderTarget ? ezPropertyUiState::Default : ezPropertyUiState::Invisible;
}
