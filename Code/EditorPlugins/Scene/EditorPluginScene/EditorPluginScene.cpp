#include <EditorPluginScene/EditorPluginScenePCH.h>

#include <EditorFramework/Actions/AssetActions.h>
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
#include <EditorPluginScene/Scene/SceneDocumentManager.h>
#include <EditorPluginScene/Scene/SceneDocumentWindow.moc.h>
#include <EditorPluginScene/Visualizers/BoxReflectionProbeVisualizerAdapter.h>
#include <EditorPluginScene/Visualizers/PointLightVisualizerAdapter.h>
#include <EditorPluginScene/Visualizers/SpotLightVisualizerAdapter.h>
#include <GameEngine/Configuration/RendererProfileConfigs.h>
#include <GameEngine/Gameplay/GreyBoxComponent.h>
#include <GuiFoundation/Action/ActionMapManager.h>
#include <GuiFoundation/Action/CommandHistoryActions.h>
#include <GuiFoundation/Action/DocumentActions.h>
#include <GuiFoundation/Action/EditActions.h>
#include <GuiFoundation/Action/StandardMenus.h>
#include <GuiFoundation/PropertyGrid/Implementation/PropertyWidget.moc.h>
#include <GuiFoundation/PropertyGrid/PropertyMetaState.h>
#include <GuiFoundation/UIServices/DynamicStringEnum.h>
#include <RendererCore/Lights/BoxReflectionProbeComponent.h>
#include <RendererCore/Lights/PointLightComponent.h>
#include <RendererCore/Lights/SpotLightComponent.h>
#include <RendererCore/Utils/CoreRenderProfile.h>
#include <ToolsFoundation/Project/ToolsProject.h>
#include <ToolsFoundation/Settings/ToolsTagRegistry.h>

static void ToolsProjectEventHandler(const ezToolsProjectEvent& e);

void OnDocumentManagerEvent(const ezDocumentManager::Event& e)
{
  switch (e.m_Type)
  {
    case ezDocumentManager::Event::Type::DocumentWindowRequested:
    {
      if (e.m_pDocument->GetDynamicRTTI() == ezGetStaticRTTI<ezScene2Document>())
      {
        new ezQtScene2DocumentWindow(static_cast<ezScene2Document*>(e.m_pDocument)); // NOLINT: Not a memory leak
      }
      else if (e.m_pDocument->GetDynamicRTTI() == ezGetStaticRTTI<ezSceneDocument>())
      {
        new ezQtSceneDocumentWindow(static_cast<ezSceneDocument*>(e.m_pDocument)); // NOLINT: Not a memory leak
      }
    }
    break;

    default:
      break;
  }
}

void ToolsProjectEventHandler(const ezToolsProjectEvent& e)
{
  if (e.m_Type == ezToolsProjectEvent::Type::ProjectFirstSetup)
  {
    auto project = ezToolsProject::GetSingleton();

    project->CreateSubFolder("Scenes");
    project->CreateSubFolder("Prefabs");

    for (auto& dm : ezAssetDocumentManager::GetAllDocumentManagers())
    {
      if (dm->IsInstanceOf<ezSceneDocumentManager>())
      {
        ezDocument* doc;

        ezStringBuilder path(project->GetProjectDirectory(), "/Scenes/Main.ezScene");
        dm->CreateDocument("Scene", path, doc).IgnoreResult();
      }
    }
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
void ezGreyBoxComponent_PropertyMetaStateEventHandler(ezPropertyMetaStateEvent& e);
void ezLightComponent_PropertyMetaStateEventHandler(ezPropertyMetaStateEvent& e);
void ezSceneDocument_PropertyMetaStateEventHandler(ezPropertyMetaStateEvent& e);

QImage SliderImageGenerator_LightTemperature(ezUInt32 uiWidth, ezUInt32 uiHeight, double fMinValue, double fMaxValue)
{
  // can use a 1D image, height doesn't need to be all used
  QImage image = QImage(uiWidth, 1, QImage::Format::Format_RGB32);

  for (ezUInt32 x = 0; x < uiWidth; ++x)
  {
    const double pos = (double)x / (uiWidth - 1.0);
    ezColor c = ezColor::MakeFromKelvin(static_cast<ezUInt32>((pos * (fMaxValue - fMinValue)) + fMinValue));

    ezColorGammaUB cg = c;
    image.setPixel(x, 0, qRgb(cg.r, cg.g, cg.b));
  }

  return image;
}

void OnLoadPlugin()
{
  ezToolsProject::GetSingleton()->s_Events.AddEventHandler(ToolsProjectEventHandler);

  ezPropertyMetaState::GetSingleton()->m_Events.AddEventHandler(ezSceneDocument_PropertyMetaStateEventHandler);

  ezDocumentManager::s_Events.AddEventHandler(ezMakeDelegate(OnDocumentManagerEvent));

  ezAssetCurator::GetSingleton()->m_Events.AddEventHandler(AssetCuratorEventHandler);

  // Add built in tags
  {
    ezToolsTagRegistry::AddTag(ezToolsTag("Default", "Exclude From Export", true));
    ezToolsTagRegistry::AddTag(ezToolsTag("Default", "CastShadow", true));
    ezToolsTagRegistry::AddTag(ezToolsTag("Default", "SkyLight", true));
  }

  ezSelectionActions::RegisterActions();
  ezSceneGizmoActions::RegisterActions();
  ezSceneActions::RegisterActions();
  ezLayerActions::RegisterActions();

  // misc
  ezQtImageSliderWidget::s_ImageGenerators["LightTemperature"] = SliderImageGenerator_LightTemperature;

  // Menu Bar
  const char* MenuBars[] = {"EditorPluginScene_DocumentMenuBar", "EditorPluginScene_Scene2MenuBar"};
  for (const char* szMenuBar : MenuBars)
  {
    ezActionMapManager::RegisterActionMap(szMenuBar, "AssetMenuBar");
    ezStandardMenus::MapActions(szMenuBar, ezStandardMenuTypes::Scene | ezStandardMenuTypes::View);
    ezDocumentActions::MapToolsActions(szMenuBar);
    ezTransformGizmoActions::MapMenuActions(szMenuBar);
    ezSceneGizmoActions::MapMenuActions(szMenuBar);
    ezGameObjectSelectionActions::MapActions(szMenuBar);
    ezSelectionActions::MapActions(szMenuBar);
    ezEditActions::MapActions(szMenuBar, true, true);
    ezTranslateGizmoAction::MapActions(szMenuBar);
    ezGameObjectDocumentActions::MapMenuActions(szMenuBar);
    ezGameObjectDocumentActions::MapMenuSimulationSpeed(szMenuBar);
    ezSceneActions::MapMenuActions(szMenuBar);
  }
  // Scene2 Menu bar adjustments
  {
    ezActionMap* pMap = ezActionMapManager::GetActionMap(MenuBars[1]);
    pMap->HideAction(ezDocumentActions::s_hSave, "G.File.Common");
    pMap->MapAction(ezLayerActions::s_hSaveActiveLayer, "G.File.Common", 6.5f);
  }


  // Tool Bar
  const char* ToolBars[] = {"EditorPluginScene_DocumentToolBar", "EditorPluginScene_Scene2ToolBar"};
  for (const char* szToolBar : ToolBars)
  {
    ezActionMapManager::RegisterActionMap(szToolBar, "AssetToolbar");

    ezTransformGizmoActions::MapToolbarActions(szToolBar);
    ezSceneGizmoActions::MapToolbarActions(szToolBar);
    ezGameObjectDocumentActions::MapToolbarActions(szToolBar);
    ezSceneActions::MapToolbarActions(szToolBar);
  }
  // Scene2 Tool bar adjustments
  {
    ezActionMap* pMap = ezActionMapManager::GetActionMap(ToolBars[1]);
    pMap->HideAction(ezDocumentActions::s_hSave, "SaveCategory");
    pMap->MapAction(ezLayerActions::s_hSaveActiveLayer, "SaveCategory", 1.0f);
  }

  // View Tool Bar
  ezActionMapManager::RegisterActionMap("EditorPluginScene_ViewToolBar", "AssetViewToolbar");
  ezViewActions::MapToolbarActions("EditorPluginScene_ViewToolBar", ezViewActions::PerspectiveMode | ezViewActions::RenderMode | ezViewActions::ActivateRemoteProcess);
  ezQuadViewActions::MapToolbarActions("EditorPluginScene_ViewToolBar");

  // Visualizers
  ezVisualizerAdapterRegistry::GetSingleton()->m_Factory.RegisterCreator(ezGetStaticRTTI<ezPointLightVisualizerAttribute>(), [](const ezRTTI* pRtti) -> ezVisualizerAdapter*
    { return EZ_DEFAULT_NEW(ezPointLightVisualizerAdapter); });
  ezVisualizerAdapterRegistry::GetSingleton()->m_Factory.RegisterCreator(ezGetStaticRTTI<ezSpotLightVisualizerAttribute>(), [](const ezRTTI* pRtti) -> ezVisualizerAdapter*
    { return EZ_DEFAULT_NEW(ezSpotLightVisualizerAdapter); });
  ezVisualizerAdapterRegistry::GetSingleton()->m_Factory.RegisterCreator(ezGetStaticRTTI<ezBoxReflectionProbeVisualizerAttribute>(), [](const ezRTTI* pRtti) -> ezVisualizerAdapter*
    { return EZ_DEFAULT_NEW(ezBoxReflectionProbeVisualizerAdapter); });

  // SceneGraph Context Menu
  ezActionMapManager::RegisterActionMap("EditorPluginScene_ScenegraphContextMenu");
  ezGameObjectSelectionActions::MapContextMenuActions("EditorPluginScene_ScenegraphContextMenu");
  ezSelectionActions::MapContextMenuActions("EditorPluginScene_ScenegraphContextMenu");
  ezEditActions::MapContextMenuActions("EditorPluginScene_ScenegraphContextMenu");

  // Layer Context Menu
  ezActionMapManager::RegisterActionMap("EditorPluginScene_LayerContextMenu");
  ezLayerActions::MapContextMenuActions("EditorPluginScene_LayerContextMenu");

  // component property meta states
  ezPropertyMetaState::GetSingleton()->m_Events.AddEventHandler(ezCameraComponent_PropertyMetaStateEventHandler);
  ezPropertyMetaState::GetSingleton()->m_Events.AddEventHandler(ezSkyLightComponent_PropertyMetaStateEventHandler);
  ezPropertyMetaState::GetSingleton()->m_Events.AddEventHandler(ezGreyBoxComponent_PropertyMetaStateEventHandler);
  ezPropertyMetaState::GetSingleton()->m_Events.AddEventHandler(ezLightComponent_PropertyMetaStateEventHandler);
}

void OnUnloadPlugin()
{
  ezPropertyMetaState::GetSingleton()->m_Events.RemoveEventHandler(ezSceneDocument_PropertyMetaStateEventHandler);

  ezToolsProject::GetSingleton()->s_Events.RemoveEventHandler(ToolsProjectEventHandler);
  ezDocumentManager::s_Events.RemoveEventHandler(ezMakeDelegate(OnDocumentManagerEvent));
  ezAssetCurator::GetSingleton()->m_Events.RemoveEventHandler(AssetCuratorEventHandler);
  ezPropertyMetaState::GetSingleton()->m_Events.RemoveEventHandler(ezGreyBoxComponent_PropertyMetaStateEventHandler);
  ezPropertyMetaState::GetSingleton()->m_Events.RemoveEventHandler(ezSkyLightComponent_PropertyMetaStateEventHandler);
  ezPropertyMetaState::GetSingleton()->m_Events.RemoveEventHandler(ezCameraComponent_PropertyMetaStateEventHandler);
  ezPropertyMetaState::GetSingleton()->m_Events.RemoveEventHandler(ezLightComponent_PropertyMetaStateEventHandler);


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
  EZ_ASSERT_DEBUG(pRtti != nullptr, "Did the typename change?");

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
  EZ_ASSERT_DEBUG(pRtti != nullptr, "Did the typename change?");

  if (e.m_pObject->GetTypeAccessor().GetType() != pRtti)
    return;

  const ezInt64 iReflectionProbeMode = e.m_pObject->GetTypeAccessor().GetValue("ReflectionProbeMode").ConvertTo<ezInt64>();
  const bool bIsStatic = (iReflectionProbeMode == 0); // ezReflectionProbeMode::Static

  auto& props = *e.m_pPropertyStates;

  props["CubeMap"].m_Visibility = bIsStatic ? ezPropertyUiState::Default : ezPropertyUiState::Invisible;
  // props["RenderTargetOffset"].m_Visibility = isRenderTarget ? ezPropertyUiState::Default : ezPropertyUiState::Invisible;
  // props["RenderTargetSize"].m_Visibility = isRenderTarget ? ezPropertyUiState::Default : ezPropertyUiState::Invisible;
}

void ezGreyBoxComponent_PropertyMetaStateEventHandler(ezPropertyMetaStateEvent& e)
{
  static const ezRTTI* pRtti = ezRTTI::FindTypeByName("ezGreyBoxComponent");
  EZ_ASSERT_DEBUG(pRtti != nullptr, "Did the typename change?");

  if (e.m_pObject->GetTypeAccessor().GetType() != pRtti)
    return;

  auto& props = *e.m_pPropertyStates;

  const ezInt64 iShapeType = e.m_pObject->GetTypeAccessor().GetValue("Shape").ConvertTo<ezInt64>();

  props["Detail"].m_Visibility = ezPropertyUiState::Invisible;
  props["Detail"].m_sNewLabelText = "Detail";
  props["Curvature"].m_Visibility = ezPropertyUiState::Invisible;
  props["Thickness"].m_Visibility = ezPropertyUiState::Invisible;
  props["SlopedTop"].m_Visibility = ezPropertyUiState::Invisible;
  props["SlopedBottom"].m_Visibility = ezPropertyUiState::Invisible;

  switch (iShapeType)
  {
    case ezGreyBoxShape::Box:
      break;
    case ezGreyBoxShape::RampPosX:
    case ezGreyBoxShape::RampNegX:
    case ezGreyBoxShape::RampPosY:
    case ezGreyBoxShape::RampNegY:
      break;
    case ezGreyBoxShape::Column:
      props["Detail"].m_Visibility = ezPropertyUiState::Default;
      break;
    case ezGreyBoxShape::StairsPosX:
    case ezGreyBoxShape::StairsNegX:
    case ezGreyBoxShape::StairsPosY:
    case ezGreyBoxShape::StairsNegY:
      props["Detail"].m_Visibility = ezPropertyUiState::Default;
      props["Curvature"].m_Visibility = ezPropertyUiState::Default;
      props["SlopedTop"].m_Visibility = ezPropertyUiState::Default;
      props["Detail"].m_sNewLabelText = "Steps";
      break;
    case ezGreyBoxShape::ArchX:
    case ezGreyBoxShape::ArchY:
      props["Detail"].m_Visibility = ezPropertyUiState::Default;
      props["Curvature"].m_Visibility = ezPropertyUiState::Default;
      props["Thickness"].m_Visibility = ezPropertyUiState::Default;
      break;
    case ezGreyBoxShape::SpiralStairs:
      props["Detail"].m_Visibility = ezPropertyUiState::Default;
      props["Curvature"].m_Visibility = ezPropertyUiState::Default;
      props["Thickness"].m_Visibility = ezPropertyUiState::Default;
      props["SlopedTop"].m_Visibility = ezPropertyUiState::Default;
      props["SlopedBottom"].m_Visibility = ezPropertyUiState::Default;
      props["Detail"].m_sNewLabelText = "Steps";
      break;
  }
}

void ezLightComponent_PropertyMetaStateEventHandler(ezPropertyMetaStateEvent& e)
{
  static const ezRTTI* pLightComponentRtti = ezRTTI::FindTypeByName("ezLightComponent");
  static const ezRTTI* pFillLightComponentRtti = ezRTTI::FindTypeByName("ezFillLightComponent");
  EZ_ASSERT_DEBUG(pLightComponentRtti != nullptr && pFillLightComponentRtti != nullptr, "Did the typename change?");

  auto& props = *e.m_pPropertyStates;

  const ezRTTI* pObjectType = e.m_pObject->GetTypeAccessor().GetType();
  const bool bIsLight = pObjectType->IsDerivedFrom(pLightComponentRtti);
  const bool bIsFillLight = pObjectType->IsDerivedFrom(pFillLightComponentRtti);

  if (bIsLight || bIsFillLight)
  {
    const bool bUseColorTemperature = e.m_pObject->GetTypeAccessor().GetValue("UseColorTemperature").ConvertTo<bool>();
    props["Temperature"].m_Visibility = bUseColorTemperature ? ezPropertyUiState::Default : ezPropertyUiState::Invisible;
    props["LightColor"].m_Visibility = bUseColorTemperature ? ezPropertyUiState::Invisible : ezPropertyUiState::Default;
  }

  if (bIsLight)
  {
    const bool bCastShadows = e.m_pObject->GetTypeAccessor().GetValue("CastShadows").ConvertTo<bool>();
    props["TransparentShadows"].m_Visibility = bCastShadows ? ezPropertyUiState::Default : ezPropertyUiState::Invisible;
    props["PenumbraSize"].m_Visibility = bCastShadows ? ezPropertyUiState::Default : ezPropertyUiState::Invisible;
    props["SlopeBias"].m_Visibility = bCastShadows ? ezPropertyUiState::Default : ezPropertyUiState::Invisible;
    props["ConstantBias"].m_Visibility = bCastShadows ? ezPropertyUiState::Default : ezPropertyUiState::Invisible;

    // Point/Spot light
    props["ShadowFadeOutRange"].m_Visibility = bCastShadows ? ezPropertyUiState::Default : ezPropertyUiState::Invisible;

    // Directional light
    props["NumCascades"].m_Visibility = bCastShadows ? ezPropertyUiState::Default : ezPropertyUiState::Invisible;
    props["MinShadowRange"].m_Visibility = bCastShadows ? ezPropertyUiState::Default : ezPropertyUiState::Invisible;
    props["FadeOutStart"].m_Visibility = bCastShadows ? ezPropertyUiState::Default : ezPropertyUiState::Invisible;
    props["SplitModeWeight"].m_Visibility = bCastShadows ? ezPropertyUiState::Default : ezPropertyUiState::Invisible;
    props["NearPlaneOffset"].m_Visibility = bCastShadows ? ezPropertyUiState::Default : ezPropertyUiState::Invisible;
  }
}
