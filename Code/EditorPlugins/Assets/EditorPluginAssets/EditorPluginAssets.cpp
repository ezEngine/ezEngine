#include <EditorPluginAssets/EditorPluginAssetsPCH.h>

#include <EditorFramework/Actions/AssetActions.h>
#include <EditorFramework/Actions/CommonAssetActions.h>
#include <EditorFramework/Actions/GameObjectContextActions.h>
#include <EditorFramework/Actions/GameObjectDocumentActions.h>
#include <EditorFramework/Actions/GameObjectSelectionActions.h>
#include <EditorFramework/Actions/ProjectActions.h>
#include <EditorFramework/Actions/QuadViewActions.h>
#include <EditorFramework/Actions/TransformGizmoActions.h>
#include <EditorFramework/Actions/ViewActions.h>
#include <EditorFramework/Actions/ViewLightActions.h>
#include <EditorPluginAssets/AnimatedMeshAsset/AnimatedMeshAssetObjects.h>
#include <EditorPluginAssets/AnimationClipAsset/AnimationClipActions.h>
#include <EditorPluginAssets/AnimationClipAsset/AnimationClipAsset.h>
#include <EditorPluginAssets/DecalAsset/DecalAsset.h>
#include <EditorPluginAssets/Dialogs/ShaderTemplateDlg.moc.h>
#include <EditorPluginAssets/LUTAsset/LUTAssetObjects.h>
#include <EditorPluginAssets/LUTAsset/LUTAssetWindow.moc.h>
#include <EditorPluginAssets/MaterialAsset/MaterialAsset.h>
#include <EditorPluginAssets/MaterialAsset/MaterialAssetWindow.moc.h>
#include <EditorPluginAssets/MeshAsset/MeshAssetObjects.h>
#include <EditorPluginAssets/SkeletonAsset/SkeletonActions.h>
#include <EditorPluginAssets/SkeletonAsset/SkeletonAsset.h>
#include <EditorPluginAssets/TextureAsset/TextureAssetObjects.h>
#include <EditorPluginAssets/TextureAsset/TextureAssetWindow.moc.h>
#include <EditorPluginAssets/TextureCubeAsset/TextureCubeAssetObjects.h>
#include <EditorPluginAssets/TextureCubeAsset/TextureCubeAssetWindow.moc.h>
#include <EditorPluginAssets/VisualShader/VisualShaderActions.h>
#include <GuiFoundation/Action/ActionMapManager.h>
#include <GuiFoundation/Action/CommandHistoryActions.h>
#include <GuiFoundation/Action/DocumentActions.h>
#include <GuiFoundation/Action/EditActions.h>
#include <GuiFoundation/Action/StandardMenus.h>

static void ConfigureAnimationGraphAsset()
{
  // Menu Bar
  {
    ezActionMapManager::RegisterActionMap("AnimationGraphAssetMenuBar", "AssetMenuBar");
    ezEditActions::MapActions("AnimationGraphAssetMenuBar", false, false);
  }

  // Tool Bar
  {
    ezActionMapManager::RegisterActionMap("AnimationGraphAssetToolBar", "AssetToolbar");
  }
}

static void ConfigureTexture2DAsset()
{
  ezPropertyMetaState::GetSingleton()->m_Events.AddEventHandler(ezTextureAssetProperties::PropertyMetaStateEventHandler);

  ezTextureAssetActions::RegisterActions();

  // Menu Bar
  {
    ezActionMapManager::RegisterActionMap("TextureAssetMenuBar", "AssetMenuBar");
  }

  // Tool Bar
  {
    ezActionMapManager::RegisterActionMap("TextureAssetToolBar", "AssetToolbar");
    ezTextureAssetActions::MapToolbarActions("TextureAssetToolBar");
  }
}

static void ConfigureTextureCubeAsset()
{
  ezPropertyMetaState::GetSingleton()->m_Events.AddEventHandler(ezTextureCubeAssetProperties::PropertyMetaStateEventHandler);

  // Menu Bar
  {
    ezActionMapManager::RegisterActionMap("TextureCubeAssetMenuBar", "AssetMenuBar");
  }

  // Tool Bar
  {
    ezActionMapManager::RegisterActionMap("TextureCubeAssetToolBar", "AssetToolbar");
    ezTextureAssetActions::MapToolbarActions("TextureCubeAssetToolBar");
  }
}

static void ConfigureLUTAsset()
{
  ezPropertyMetaState::GetSingleton()->m_Events.AddEventHandler(ezLUTAssetProperties::PropertyMetaStateEventHandler);

  ezLUTAssetActions::RegisterActions();

  // Menu Bar
  {
    ezActionMapManager::RegisterActionMap("LUTAssetMenuBar", "AssetMenuBar");
  }

  // Tool Bar
  {
    ezActionMapManager::RegisterActionMap("LUTAssetToolBar", "AssetToolbar");
  }
}

static void ConfigureMaterialAsset()
{
  ezPropertyMetaState::GetSingleton()->m_Events.AddEventHandler(ezMaterialAssetProperties::PropertyMetaStateEventHandler);

  // Menu Bar
  {
    ezActionMapManager::RegisterActionMap("MaterialAssetMenuBar", "AssetMenuBar");
    ezDocumentActions::MapToolsActions("MaterialAssetMenuBar");
    ezEditActions::MapActions("MaterialAssetMenuBar", false, false);
  }

  // Tool Bar
  {
    ezActionMapManager::RegisterActionMap("MaterialAssetToolBar", "AssetToolbar");

    ezMaterialAssetActions::RegisterActions();
    ezMaterialAssetActions::MapToolbarActions("MaterialAssetToolBar");

    ezVisualShaderActions::RegisterActions();
    ezVisualShaderActions::MapActions("MaterialAssetToolBar");
  }

  // View Tool Bar
  {
    ezActionMapManager::RegisterActionMap("MaterialAssetViewToolBar", "SimpleAssetViewToolbar");
  }
}

static void ConfigureRenderPipelineAsset()
{
  // Menu Bar
  {
    ezActionMapManager::RegisterActionMap("RenderPipelineAssetMenuBar", "AssetMenuBar");
    ezEditActions::MapActions("RenderPipelineAssetMenuBar", false, false);
  }

  // Tool Bar
  {
    ezActionMapManager::RegisterActionMap("RenderPipelineAssetToolBar", "AssetToolbar");
  }
}

static void ConfigureMeshAsset()
{
  ezPropertyMetaState::GetSingleton()->m_Events.AddEventHandler(ezMeshAssetProperties::PropertyMetaStateEventHandler);

  // Menu Bar
  {
    ezActionMapManager::RegisterActionMap("MeshAssetMenuBar", "AssetMenuBar");
  }

  // Tool Bar
  {
    ezActionMapManager::RegisterActionMap("MeshAssetToolBar", "AssetToolbar");
    ezCommonAssetActions::MapToolbarActions("MeshAssetToolBar", ezCommonAssetUiState::Grid);
  }

  // View Tool Bar
  {
    ezActionMapManager::RegisterActionMap("MeshAssetViewToolBar", "SimpleAssetViewToolbar");
  }
}

static void ConfigureSurfaceAsset()
{
  // Menu Bar
  {
    ezActionMapManager::RegisterActionMap("SurfaceAssetMenuBar", "AssetMenuBar");
    ezDocumentActions::MapToolsActions("SurfaceAssetMenuBar");
  }

  // Tool Bar
  {
    ezActionMapManager::RegisterActionMap("SurfaceAssetToolBar", "AssetToolbar");
  }
}

static void ConfigureCollectionAsset()
{
  // Menu Bar
  {
    ezActionMapManager::RegisterActionMap("CollectionAssetMenuBar", "AssetMenuBar");
    ezDocumentActions::MapToolsActions("CollectionAssetMenuBar");
  }

  // Tool Bar
  {
    ezActionMapManager::RegisterActionMap("CollectionAssetToolBar", "AssetToolbar");
  }
}

static void ConfigureColorGradientAsset()
{
  // Menu Bar
  {
    ezActionMapManager::RegisterActionMap("ColorGradientAssetMenuBar", "AssetMenuBar");
    ezDocumentActions::MapToolsActions("ColorGradientAssetMenuBar");
  }

  // Tool Bar
  {
    ezActionMapManager::RegisterActionMap("ColorGradientAssetToolBar", "AssetToolbar");
  }
}

static void ConfigureCurve1DAsset()
{
  // Menu Bar
  {
    ezActionMapManager::RegisterActionMap("Curve1DAssetMenuBar", "AssetMenuBar");
    ezDocumentActions::MapToolsActions("Curve1DAssetMenuBar");
  }

  // Tool Bar
  {
    ezActionMapManager::RegisterActionMap("Curve1DAssetToolBar", "AssetToolbar");
  }
}

static void ConfigurePropertyAnimAsset()
{
  // Menu Bar
  {
    ezActionMapManager::RegisterActionMap("PropertyAnimAssetMenuBar", "AssetMenuBar");
    ezStandardMenus::MapActions("PropertyAnimAssetMenuBar", ezStandardMenuTypes::Scene | ezStandardMenuTypes::View);
    ezDocumentActions::MapToolsActions("PropertyAnimAssetMenuBar");
    ezGameObjectSelectionActions::MapActions("PropertyAnimAssetMenuBar");
    ezGameObjectDocumentActions::MapMenuActions("PropertyAnimAssetMenuBar");
    ezGameObjectDocumentActions::MapMenuSimulationSpeed("PropertyAnimAssetMenuBar");
    ezTransformGizmoActions::MapMenuActions("PropertyAnimAssetMenuBar");
    ezTranslateGizmoAction::MapActions("PropertyAnimAssetMenuBar");
  }

  // Tool Bar
  {
    ezActionMapManager::RegisterActionMap("PropertyAnimAssetToolBar", "AssetToolbar");
    ezGameObjectContextActions::MapToolbarActions("PropertyAnimAssetToolBar");
    ezGameObjectDocumentActions::MapToolbarActions("PropertyAnimAssetToolBar");
    ezTransformGizmoActions::MapToolbarActions("PropertyAnimAssetToolBar");
  }

  // View Tool Bar
  {
    ezActionMapManager::RegisterActionMap("PropertyAnimAssetViewToolBar", "AssetViewToolbar");
    ezViewActions::MapToolbarActions("PropertyAnimAssetViewToolBar", ezViewActions::PerspectiveMode | ezViewActions::RenderMode | ezViewActions::ActivateRemoteProcess);
    ezQuadViewActions::MapToolbarActions("PropertyAnimAssetViewToolBar");
  }

  // SceneGraph Context Menu
  {
    ezActionMapManager::RegisterActionMap("PropertyAnimAsset_ScenegraphContextMenu");
    ezGameObjectSelectionActions::MapContextMenuActions("PropertyAnimAsset_ScenegraphContextMenu");
    ezGameObjectContextActions::MapContextMenuActions("PropertyAnimAsset_ScenegraphContextMenu");
  }
}

static void ConfigureDecalAsset()
{
  ezPropertyMetaState::GetSingleton()->m_Events.AddEventHandler(ezDecalAssetProperties::PropertyMetaStateEventHandler);

  // Menu Bar
  {
    ezActionMapManager::RegisterActionMap("DecalAssetMenuBar", "AssetMenuBar");
  }

  // Tool Bar
  {
    ezActionMapManager::RegisterActionMap("DecalAssetToolBar", "AssetToolbar");
  }

  // View Tool Bar
  {
    ezActionMapManager::RegisterActionMap("DecalAssetViewToolBar", "SimpleAssetViewToolbar");
  }
}

static void ConfigureAnimationClipAsset()
{
  ezAnimationClipActions::RegisterActions();

  ezPropertyMetaState::GetSingleton()->m_Events.AddEventHandler(ezAnimationClipAssetProperties::PropertyMetaStateEventHandler);

  // Menu Bar
  {
    ezActionMapManager::RegisterActionMap("AnimationClipAssetMenuBar", "AssetMenuBar");

    // additionally to the standard menus, also map the 'Assets' menu for asset specific actions
    ezStandardMenus::MapActions("AnimationClipAssetMenuBar", ezStandardMenuTypes::Asset);
    ezAnimationClipActions::MapActions("AnimationClipAssetMenuBar", "G.Asset");
  }

  // Tool Bar
  {
    ezActionMapManager::RegisterActionMap("AnimationClipAssetToolBar", "AssetToolbar");
    ezAnimationClipActions::MapActions("AnimationClipAssetToolBar", "");
    ezCommonAssetActions::MapToolbarActions("AnimationClipAssetToolBar", ezCommonAssetUiState::Loop | ezCommonAssetUiState::Pause | ezCommonAssetUiState::Restart | ezCommonAssetUiState::SimulationSpeed | ezCommonAssetUiState::Grid);
  }

  // View Tool Bar
  {
    ezActionMapManager::RegisterActionMap("AnimationClipAssetViewToolBar", "SimpleAssetViewToolbar");
  }
}

static void ConfigureSkeletonAsset()
{
  ezPropertyMetaState::GetSingleton()->m_Events.AddEventHandler(ezSkeletonAssetDocument::PropertyMetaStateEventHandler);

  ezSkeletonActions::RegisterActions();

  // Menu Bar
  {
    ezActionMapManager::RegisterActionMap("SkeletonAssetMenuBar", "AssetMenuBar");
  }

  // Tool Bar
  {
    ezActionMapManager::RegisterActionMap("SkeletonAssetToolBar", "AssetToolbar");
    ezCommonAssetActions::MapToolbarActions("SkeletonAssetToolBar", ezCommonAssetUiState::Grid);
    ezSkeletonActions::MapActions("SkeletonAssetToolBar");
  }

  // View Tool Bar
  {
    ezActionMapManager::RegisterActionMap("SkeletonAssetViewToolBar", "SimpleAssetViewToolbar");
  }
}

static void ConfigureAnimatedMeshAsset()
{
  ezPropertyMetaState::GetSingleton()->m_Events.AddEventHandler(ezAnimatedMeshAssetProperties::PropertyMetaStateEventHandler);

  // Menu Bar
  {
    ezActionMapManager::RegisterActionMap("AnimatedMeshAssetMenuBar", "AssetMenuBar");
  }

  // Tool Bar
  {
    ezActionMapManager::RegisterActionMap("AnimatedMeshAssetToolBar", "AssetToolbar");
    ezCommonAssetActions::MapToolbarActions("AnimatedMeshAssetToolBar", ezCommonAssetUiState::Grid);
  }

  // View Tool Bar
  {
    ezActionMapManager::RegisterActionMap("AnimatedMeshAssetViewToolBar", "SimpleAssetViewToolbar");
  }
}

static void ConfigureImageDataAsset()
{
  // Menu Bar
  {
    ezActionMapManager::RegisterActionMap("ImageDataAssetMenuBar", "AssetMenuBar");
  }

  // Tool Bar
  {
    ezActionMapManager::RegisterActionMap("ImageDataAssetToolBar", "AssetToolbar");
  }

  // View Tool Bar
  {
    ezActionMapManager::RegisterActionMap("ImageDataAssetViewToolBar", "SimpleAssetViewToolbar");
  }
}

static void ConfigureStateMachineAsset()
{
  // Menu Bar
  {
    ezActionMapManager::RegisterActionMap("StateMachineAssetMenuBar", "AssetMenuBar");
    ezEditActions::MapActions("StateMachineAssetMenuBar", false, false);
  }

  // Tool Bar
  {
    ezActionMapManager::RegisterActionMap("StateMachineAssetToolBar", "AssetToolbar");
  }
}
static void ConfigureBlackboardTemplateAsset()
{
  // Menu Bar
  {
    ezActionMapManager::RegisterActionMap("BlackboardTemplateAssetMenuBar", "AssetMenuBar");
    ezEditActions::MapActions("BlackboardTemplateAssetMenuBar", false, false);
  }

  // Tool Bar
  {
    ezActionMapManager::RegisterActionMap("BlackboardTemplateAssetToolBar", "AssetToolbar");
  }
}

static void ConfigureCustomDataAsset()
{
  // Menu Bar
  {
    ezActionMapManager::RegisterActionMap("CustomDataAssetMenuBar", "AssetMenuBar");
    ezDocumentActions::MapToolsActions("CustomDataAssetMenuBar");
  }

  // Tool Bar
  {
    ezActionMapManager::RegisterActionMap("CustomDataAssetToolBar", "AssetToolbar");
  }
}

ezVariant CustomAction_CreateShaderFromTemplate(const ezDocument* pDoc)
{
  ezQtShaderTemplateDlg dlg(nullptr, pDoc);

  if (dlg.exec() == QDialog::Accepted)
  {
    ezStringBuilder abs;
    if (ezFileSystem::ResolvePath(dlg.m_sResult, &abs, nullptr).Succeeded())
    {
      if (!ezQtUiServices::GetSingleton()->OpenFileInDefaultProgram(abs))
      {
        ezQtUiServices::GetSingleton()->MessageBoxInformation(ezFmt("There is no default program set to open shader files:\n\n{}", abs));
      }
    }

    return dlg.m_sResult;
  }

  return {};
}

void OnLoadPlugin()
{
  ConfigureAnimationGraphAsset();
  ConfigureTexture2DAsset();
  ConfigureTextureCubeAsset();
  ConfigureLUTAsset();
  ConfigureMaterialAsset();
  ConfigureRenderPipelineAsset();
  ConfigureMeshAsset();
  ConfigureSurfaceAsset();
  ConfigureCollectionAsset();
  ConfigureColorGradientAsset();
  ConfigureCurve1DAsset();
  ConfigurePropertyAnimAsset();
  ConfigureDecalAsset();
  ConfigureAnimationClipAsset();
  ConfigureSkeletonAsset();
  ConfigureAnimatedMeshAsset();
  ConfigureImageDataAsset();
  ConfigureStateMachineAsset();
  ConfigureBlackboardTemplateAsset();
  ConfigureCustomDataAsset();

  ezDocumentManager::s_CustomActions["CustomAction_CreateShaderFromTemplate"] = CustomAction_CreateShaderFromTemplate;
}

void OnUnloadPlugin()
{
  ezTextureAssetActions::UnregisterActions();
  ezLUTAssetActions::UnregisterActions();
  ezVisualShaderActions::UnregisterActions();
  ezMaterialAssetActions::UnregisterActions();
  ezSkeletonActions::UnregisterActions();
  ezAnimationClipActions::UnregisterActions();

  ezPropertyMetaState::GetSingleton()->m_Events.RemoveEventHandler(ezAnimatedMeshAssetProperties::PropertyMetaStateEventHandler);
  ezPropertyMetaState::GetSingleton()->m_Events.RemoveEventHandler(ezMeshAssetProperties::PropertyMetaStateEventHandler);
  ezPropertyMetaState::GetSingleton()->m_Events.RemoveEventHandler(ezTextureAssetProperties::PropertyMetaStateEventHandler);
  ezPropertyMetaState::GetSingleton()->m_Events.RemoveEventHandler(ezDecalAssetProperties::PropertyMetaStateEventHandler);
  ezPropertyMetaState::GetSingleton()->m_Events.RemoveEventHandler(ezTextureCubeAssetProperties::PropertyMetaStateEventHandler);
  ezPropertyMetaState::GetSingleton()->m_Events.RemoveEventHandler(ezMaterialAssetProperties::PropertyMetaStateEventHandler);
  ezPropertyMetaState::GetSingleton()->m_Events.RemoveEventHandler(ezSkeletonAssetDocument::PropertyMetaStateEventHandler);
  ezPropertyMetaState::GetSingleton()->m_Events.RemoveEventHandler(ezAnimationClipAssetProperties::PropertyMetaStateEventHandler);
}

EZ_PLUGIN_ON_LOADED()
{
  OnLoadPlugin();
}

EZ_PLUGIN_ON_UNLOADED()
{
  OnUnloadPlugin();
}
