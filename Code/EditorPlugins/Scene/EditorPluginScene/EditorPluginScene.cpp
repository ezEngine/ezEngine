#include <EditorPluginScene/EditorPluginScenePCH.h>

#include <EditorFramework/Actions/GameObjectDocumentActions.h>
#include <EditorFramework/Actions/GameObjectSelectionActions.h>
#include <EditorFramework/Actions/ProjectActions.h>
#include <EditorFramework/Actions/QuadViewActions.h>
#include <EditorFramework/Actions/TransformGizmoActions.h>
#include <EditorFramework/Actions/ViewActions.h>
#include <EditorFramework/Assets/AssetCurator.h>
#include <EditorFramework/EditorApp/EditorApp.moc.h>
#include <EditorFramework/Visualizers/VisualizerAdapterRegistry.h>
#include <EditorPluginScene/Actions/LayerActions.h>
#include <EditorPluginScene/Actions/SceneActions.h>
#include <EditorPluginScene/Actions/SelectionActions.h>
#include <EditorPluginScene/Scene/Scene2Document.h>
#include <EditorPluginScene/Scene/Scene2DocumentWindow.moc.h>
#include <EditorPluginScene/Scene/SceneDocumentWindow.moc.h>
#include <EditorPluginScene/Visualizers/BoxReflectionProbeVisualizerAdapter.h>
#include <EditorPluginScene/Visualizers/PointLightVisualizerAdapter.h>
#include <EditorPluginScene/Visualizers/SpotLightVisualizerAdapter.h>
#include <GameEngine/Configuration/RendererProfileConfigs.h>
#include <GuiFoundation/Action/ActionMapManager.h>
#include <GuiFoundation/Action/CommandHistoryActions.h>
#include <GuiFoundation/Action/DocumentActions.h>
#include <GuiFoundation/Action/EditActions.h>
#include <GuiFoundation/Action/StandardMenus.h>
#include <GuiFoundation/PropertyGrid/PropertyMetaState.h>
#include <GuiFoundation/UIServices/DynamicStringEnum.h>
#include <RendererCore/Lights/BoxReflectionProbeComponent.h>
#include <RendererCore/Lights/PointLightComponent.h>
#include <RendererCore/Lights/SpotLightComponent.h>
#include <ToolsFoundation/Settings/ToolsTagRegistry.h>

void OnDocumentManagerEvent(const ezDocumentManager::Event& e)
{
  switch (e.m_Type)
  {
    case ezDocumentManager::Event::Type::DocumentWindowRequested:
    {
      if (e.m_pDocument->GetDynamicRTTI() == ezGetStaticRTTI<ezScene2Document>())
      {
        ezQtDocumentWindow* pDocWnd = new ezQtScene2DocumentWindow(static_cast<ezScene2Document*>(e.m_pDocument));
      }
      else if (e.m_pDocument->GetDynamicRTTI() == ezGetStaticRTTI<ezSceneDocument>())
      {
        ezQtDocumentWindow* pDocWnd = new ezQtSceneDocumentWindow(static_cast<ezSceneDocument*>(e.m_pDocument));
      }
    }
    break;

    default:
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

    auto& dynEnum = ezDynamicStringEnum::CreateDynamicEnum("CameraPipelines");

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
void ezSkyLightComponent_PropertyMetaStateEventHandler(ezPropertyMetaStateEvent& e);

void OnLoadPlugin()
{
  ezDocumentManager::s_Events.AddEventHandler(ezMakeDelegate(OnDocumentManagerEvent));

  ezQtEditorApp::GetSingleton()->m_Events.AddEventHandler(ToolsProjectEventHandler);

  ezAssetCurator::GetSingleton()->m_Events.AddEventHandler(AssetCuratorEventHandler);

  // Add built in tags
  {
    ezToolsTagRegistry::AddTag(ezToolsTag("Default", "CastShadow", true));
    ezToolsTagRegistry::AddTag(ezToolsTag("Default", "SkyLight", true));
  }

  ezSelectionActions::RegisterActions();
  ezSceneGizmoActions::RegisterActions();
  ezSceneActions::RegisterActions();
  ezLayerActions::RegisterActions();

  // Menu Bar
  const char* MenuBars[] = {"EditorPluginScene_DocumentMenuBar", "EditorPluginScene_Scene2MenuBar"};
  for (const char* szMenuBar : MenuBars)
  {
    ezActionMapManager::RegisterActionMap(szMenuBar).IgnoreResult();
    ezStandardMenus::MapActions(szMenuBar, ezStandardMenuTypes::File | ezStandardMenuTypes::Edit | ezStandardMenuTypes::Scene | ezStandardMenuTypes::Panels | ezStandardMenuTypes::View | ezStandardMenuTypes::Help);
    ezProjectActions::MapActions(szMenuBar);
    ezDocumentActions::MapActions(szMenuBar, "Menu.File", false);
    ezDocumentActions::MapToolsActions(szMenuBar, "Menu.Tools");
    ezCommandHistoryActions::MapActions(szMenuBar, "Menu.Edit");
    ezTransformGizmoActions::MapMenuActions(szMenuBar, "Menu.Edit");
    ezSceneGizmoActions::MapMenuActions(szMenuBar, "Menu.Edit");
    ezGameObjectSelectionActions::MapActions(szMenuBar, "Menu.Edit");
    ezSelectionActions::MapActions(szMenuBar, "Menu.Edit");
    ezEditActions::MapActions(szMenuBar, "Menu.Edit", true, true);
    ezTranslateGizmoAction::MapActions(szMenuBar, "Menu.Edit/Gizmo.Menu");
    ezGameObjectDocumentActions::MapMenuActions(szMenuBar, "Menu.View");
    ezGameObjectDocumentActions::MapMenuSimulationSpeed(szMenuBar, "Menu.Scene");
    ezSceneActions::MapMenuActions(szMenuBar);
  }
  // Scene2 Menu bar adjustments
  {
    ezActionMap* pMap = ezActionMapManager::GetActionMap(MenuBars[1]);
    pMap->UnmapAction(ezDocumentActions::s_hSave, "Menu.File/SaveCategory").IgnoreResult();
    pMap->MapAction(ezLayerActions::s_hSaveActiveLayer, "Menu.File/SaveCategory", 1.0f);
  }


  // Tool Bar
  const char* ToolBars[] = {"EditorPluginScene_DocumentToolBar", "EditorPluginScene_Scene2ToolBar"};
  for (const char* szToolBar : ToolBars)
  {
    ezActionMapManager::RegisterActionMap(szToolBar).IgnoreResult();
    ezDocumentActions::MapActions(szToolBar, "", true);
    ezCommandHistoryActions::MapActions(szToolBar, "");
    ezTransformGizmoActions::MapToolbarActions(szToolBar, "");
    ezSceneGizmoActions::MapToolbarActions(szToolBar, "");
    ezGameObjectDocumentActions::MapToolbarActions(szToolBar, "");
    ezSceneActions::MapToolbarActions(szToolBar);
  }
  // Scene2 Tool bar adjustments
  {
    ezActionMap* pMap = ezActionMapManager::GetActionMap(ToolBars[1]);
    pMap->UnmapAction(ezDocumentActions::s_hSave, "SaveCategory").IgnoreResult();
    pMap->MapAction(ezLayerActions::s_hSaveActiveLayer, "SaveCategory", 1.0f);
  }

  // View Tool Bar
  ezActionMapManager::RegisterActionMap("EditorPluginScene_ViewToolBar").IgnoreResult();
  ezViewActions::MapActions("EditorPluginScene_ViewToolBar", "", ezViewActions::PerspectiveMode | ezViewActions::RenderMode | ezViewActions::ActivateRemoteProcess);
  ezQuadViewActions::MapActions("EditorPluginScene_ViewToolBar", "");

  // Visualizers
  ezVisualizerAdapterRegistry::GetSingleton()->m_Factory.RegisterCreator(ezGetStaticRTTI<ezPointLightVisualizerAttribute>(), [](const ezRTTI* pRtti) -> ezVisualizerAdapter* { return EZ_DEFAULT_NEW(ezPointLightVisualizerAdapter); });
  ezVisualizerAdapterRegistry::GetSingleton()->m_Factory.RegisterCreator(ezGetStaticRTTI<ezSpotLightVisualizerAttribute>(), [](const ezRTTI* pRtti) -> ezVisualizerAdapter* { return EZ_DEFAULT_NEW(ezSpotLightVisualizerAdapter); });
  ezVisualizerAdapterRegistry::GetSingleton()->m_Factory.RegisterCreator(ezGetStaticRTTI<ezBoxReflectionProbeVisualizerAttribute>(), [](const ezRTTI* pRtti) -> ezVisualizerAdapter* { return EZ_DEFAULT_NEW(ezBoxReflectionProbeVisualizerAdapter); });

  // SceneGraph Context Menu
  ezActionMapManager::RegisterActionMap("EditorPluginScene_ScenegraphContextMenu").IgnoreResult();
  ezGameObjectSelectionActions::MapContextMenuActions("EditorPluginScene_ScenegraphContextMenu", "");
  ezSelectionActions::MapContextMenuActions("EditorPluginScene_ScenegraphContextMenu", "");
  ezEditActions::MapContextMenuActions("EditorPluginScene_ScenegraphContextMenu", "");

  // Layer Context Menu
  ezActionMapManager::RegisterActionMap("EditorPluginScene_LayerContextMenu").IgnoreResult();
  ezLayerActions::MapContextMenuActions("EditorPluginScene_LayerContextMenu", "");

  // component property meta states
  ezPropertyMetaState::GetSingleton()->m_Events.AddEventHandler(ezCameraComponent_PropertyMetaStateEventHandler);
  ezPropertyMetaState::GetSingleton()->m_Events.AddEventHandler(ezSkyLightComponent_PropertyMetaStateEventHandler);
}

void OnUnloadPlugin()
{
  ezDocumentManager::s_Events.RemoveEventHandler(ezMakeDelegate(OnDocumentManagerEvent));
  ezQtEditorApp::GetSingleton()->m_Events.RemoveEventHandler(ToolsProjectEventHandler);
  ezAssetCurator::GetSingleton()->m_Events.RemoveEventHandler(AssetCuratorEventHandler);
  ezPropertyMetaState::GetSingleton()->m_Events.RemoveEventHandler(ezSkyLightComponent_PropertyMetaStateEventHandler);
  ezPropertyMetaState::GetSingleton()->m_Events.RemoveEventHandler(ezCameraComponent_PropertyMetaStateEventHandler);


  ezSelectionActions::UnregisterActions();
  ezSceneGizmoActions::UnregisterActions();
  ezLayerActions::UnregisterActions();
  ezSceneActions::UnregisterActions();
}

EZ_PLUGIN_ON_LOADED()
{
  OnLoadPlugin();
}

EZ_PLUGIN_ON_UNLOADED()
{
  OnUnloadPlugin();
}

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

void ezSkyLightComponent_PropertyMetaStateEventHandler(ezPropertyMetaStateEvent& e)
{
  static const ezRTTI* pRtti = ezRTTI::FindTypeByName("ezSkyLightComponent");

  if (e.m_pObject->GetTypeAccessor().GetType() != pRtti)
    return;

  const ezInt64 iReflectionProbeMode = e.m_pObject->GetTypeAccessor().GetValue("ReflectionProbeMode").ConvertTo<ezInt64>();
  const bool bIsStatic = (iReflectionProbeMode == 0); // ezReflectionProbeMode::Static

  auto& props = *e.m_pPropertyStates;

  props["CubeMap"].m_Visibility = bIsStatic ? ezPropertyUiState::Default : ezPropertyUiState::Invisible;
  //props["RenderTargetOffset"].m_Visibility = isRenderTarget ? ezPropertyUiState::Default : ezPropertyUiState::Invisible;
  //props["RenderTargetSize"].m_Visibility = isRenderTarget ? ezPropertyUiState::Default : ezPropertyUiState::Invisible;
}
