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
    ezActionMapManager::RegisterActionMap("AnimationGraphAssetMenuBar", "AssetMenuBar").AssertSuccess();
    ezEditActions::MapActions("AnimationGraphAssetMenuBar", false, false);
  }

  // Tool Bar
  {
    ezActionMapManager::RegisterActionMap("AnimationGraphAssetToolBar", "AssetToolbar").AssertSuccess();
  }
}

static void ConfigureTexture2DAsset()
{
  ezPropertyMetaState::GetSingleton()->m_Events.AddEventHandler(ezTextureAssetProperties::PropertyMetaStateEventHandler);

  ezTextureAssetActions::RegisterActions();

  // Menu Bar
  {
    ezActionMapManager::RegisterActionMap("TextureAssetMenuBar", "AssetMenuBar").AssertSuccess();
  }

  // Tool Bar
  {
    ezActionMapManager::RegisterActionMap("TextureAssetToolBar", "AssetToolbar").AssertSuccess();
    ezTextureAssetActions::MapToolbarActions("TextureAssetToolBar");
  }
}

static void ConfigureTextureCubeAsset()
{
  ezPropertyMetaState::GetSingleton()->m_Events.AddEventHandler(ezTextureCubeAssetProperties::PropertyMetaStateEventHandler);

  // Menu Bar
  {
    ezActionMapManager::RegisterActionMap("TextureCubeAssetMenuBar", "AssetMenuBar").AssertSuccess();
  }

  // Tool Bar
  {
    ezActionMapManager::RegisterActionMap("TextureCubeAssetToolBar", "AssetToolbar").AssertSuccess();
    ezTextureAssetActions::MapToolbarActions("TextureCubeAssetToolBar");
  }
}

static void ConfigureLUTAsset()
{
  ezPropertyMetaState::GetSingleton()->m_Events.AddEventHandler(ezLUTAssetProperties::PropertyMetaStateEventHandler);

  ezLUTAssetActions::RegisterActions();

  // Menu Bar
  {
    ezActionMapManager::RegisterActionMap("LUTAssetMenuBar", "AssetMenuBar").AssertSuccess();
  }

  // Tool Bar
  {
    ezActionMapManager::RegisterActionMap("LUTAssetToolBar", "AssetToolbar").AssertSuccess();
  }
}

static void ConfigureMaterialAsset()
{
  ezPropertyMetaState::GetSingleton()->m_Events.AddEventHandler(ezMaterialAssetProperties::PropertyMetaStateEventHandler);

  // Menu Bar
  {
    ezActionMapManager::RegisterActionMap("MaterialAssetMenuBar", "AssetMenuBar").AssertSuccess();
    ezDocumentActions::MapToolsActions("MaterialAssetMenuBar");
    ezEditActions::MapActions("MaterialAssetMenuBar", false, false);
  }

  // Tool Bar
  {
    ezActionMapManager::RegisterActionMap("MaterialAssetToolBar", "AssetToolbar").AssertSuccess();

    ezMaterialAssetActions::RegisterActions();
    ezMaterialAssetActions::MapToolbarActions("MaterialAssetToolBar");

    ezVisualShaderActions::RegisterActions();
    ezVisualShaderActions::MapActions("MaterialAssetToolBar");
  }

  // View Tool Bar
  {
    ezActionMapManager::RegisterActionMap("MaterialAssetViewToolBar", "SimpleAssetViewToolbar").AssertSuccess();
  }
}

static void ConfigureRenderPipelineAsset()
{
  // Menu Bar
  {
    ezActionMapManager::RegisterActionMap("RenderPipelineAssetMenuBar", "AssetMenuBar").AssertSuccess();
    ezEditActions::MapActions("RenderPipelineAssetMenuBar", false, false);
  }

  // Tool Bar
  {
    ezActionMapManager::RegisterActionMap("RenderPipelineAssetToolBar", "AssetToolbar").AssertSuccess();
  }
}

static void ConfigureMeshAsset()
{
  ezPropertyMetaState::GetSingleton()->m_Events.AddEventHandler(ezMeshAssetProperties::PropertyMetaStateEventHandler);

  // Menu Bar
  {
    ezActionMapManager::RegisterActionMap("MeshAssetMenuBar", "AssetMenuBar").AssertSuccess();
  }

  // Tool Bar
  {
    ezActionMapManager::RegisterActionMap("MeshAssetToolBar", "AssetToolbar").AssertSuccess();
    ezCommonAssetActions::MapToolbarActions("MeshAssetToolBar", ezCommonAssetUiState::Grid);
  }

  // View Tool Bar
  {
    ezActionMapManager::RegisterActionMap("MeshAssetViewToolBar", "SimpleAssetViewToolbar").AssertSuccess();
  }
}

static void ConfigureSurfaceAsset()
{
  // Menu Bar
  {
    ezActionMapManager::RegisterActionMap("SurfaceAssetMenuBar", "AssetMenuBar").AssertSuccess();
    ezDocumentActions::MapToolsActions("SurfaceAssetMenuBar");
  }

  // Tool Bar
  {
    ezActionMapManager::RegisterActionMap("SurfaceAssetToolBar", "AssetToolbar").AssertSuccess();
  }
}

static void ConfigureCollectionAsset()
{
  // Menu Bar
  {
    ezActionMapManager::RegisterActionMap("CollectionAssetMenuBar", "AssetMenuBar").AssertSuccess();
    ezDocumentActions::MapToolsActions("CollectionAssetMenuBar");
  }

  // Tool Bar
  {
    ezActionMapManager::RegisterActionMap("CollectionAssetToolBar", "AssetToolbar").AssertSuccess();
  }
}

static void ConfigureColorGradientAsset()
{
  // Menu Bar
  {
    ezActionMapManager::RegisterActionMap("ColorGradientAssetMenuBar", "AssetMenuBar").AssertSuccess();
    ezDocumentActions::MapToolsActions("ColorGradientAssetMenuBar");
  }

  // Tool Bar
  {
    ezActionMapManager::RegisterActionMap("ColorGradientAssetToolBar", "AssetToolbar").AssertSuccess();
  }
}

static void ConfigureCurve1DAsset()
{
  // Menu Bar
  {
    ezActionMapManager::RegisterActionMap("Curve1DAssetMenuBar", "AssetMenuBar").AssertSuccess();
    ezDocumentActions::MapToolsActions("Curve1DAssetMenuBar");
  }

  // Tool Bar
  {
    ezActionMapManager::RegisterActionMap("Curve1DAssetToolBar", "AssetToolbar").AssertSuccess();
  }
}

static void ConfigurePropertyAnimAsset()
{
  // Menu Bar
  {
    ezActionMapManager::RegisterActionMap("PropertyAnimAssetMenuBar", "AssetMenuBar").AssertSuccess();
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
    ezActionMapManager::RegisterActionMap("PropertyAnimAssetToolBar", "AssetToolbar").AssertSuccess();
    ezGameObjectContextActions::MapToolbarActions("PropertyAnimAssetToolBar");
    ezGameObjectDocumentActions::MapToolbarActions("PropertyAnimAssetToolBar");
    ezTransformGizmoActions::MapToolbarActions("PropertyAnimAssetToolBar");
  }

  // View Tool Bar
  {
    ezActionMapManager::RegisterActionMap("PropertyAnimAssetViewToolBar", "AssetViewToolbar").AssertSuccess();
    ezViewActions::MapToolbarActions("PropertyAnimAssetViewToolBar", ezViewActions::PerspectiveMode | ezViewActions::RenderMode | ezViewActions::ActivateRemoteProcess);
    ezQuadViewActions::MapToolbarActions("PropertyAnimAssetViewToolBar");
  }

  // SceneGraph Context Menu
  {
    ezActionMapManager::RegisterActionMap("PropertyAnimAsset_ScenegraphContextMenu").AssertSuccess();
    ezGameObjectSelectionActions::MapContextMenuActions("PropertyAnimAsset_ScenegraphContextMenu");
    ezGameObjectContextActions::MapContextMenuActions("PropertyAnimAsset_ScenegraphContextMenu");
  }
}

static void ConfigureDecalAsset()
{
  ezPropertyMetaState::GetSingleton()->m_Events.AddEventHandler(ezDecalAssetProperties::PropertyMetaStateEventHandler);

  // Menu Bar
  {
    ezActionMapManager::RegisterActionMap("DecalAssetMenuBar", "AssetMenuBar").AssertSuccess();
  }

  // Tool Bar
  {
    ezActionMapManager::RegisterActionMap("DecalAssetToolBar", "AssetToolbar").AssertSuccess();
  }

  // View Tool Bar
  {
    ezActionMapManager::RegisterActionMap("DecalAssetViewToolBar", "SimpleAssetViewToolbar").AssertSuccess();
  }
}

static void ConfigureAnimationClipAsset()
{
  ezPropertyMetaState::GetSingleton()->m_Events.AddEventHandler(ezAnimationClipAssetProperties::PropertyMetaStateEventHandler);

  // Menu Bar
  {
    ezActionMapManager::RegisterActionMap("AnimationClipAssetMenuBar", "AssetMenuBar").AssertSuccess();
  }

  // Tool Bar
  {
    ezActionMapManager::RegisterActionMap("AnimationClipAssetToolBar", "AssetToolbar").AssertSuccess();
    ezCommonAssetActions::MapToolbarActions("AnimationClipAssetToolBar", ezCommonAssetUiState::Loop | ezCommonAssetUiState::Pause | ezCommonAssetUiState::Restart | ezCommonAssetUiState::SimulationSpeed | ezCommonAssetUiState::Grid);
  }

  // View Tool Bar
  {
    ezActionMapManager::RegisterActionMap("AnimationClipAssetViewToolBar", "SimpleAssetViewToolbar").AssertSuccess();
  }
}

static void ConfigureSkeletonAsset()
{
  ezPropertyMetaState::GetSingleton()->m_Events.AddEventHandler(ezSkeletonAssetDocument::PropertyMetaStateEventHandler);

  ezSkeletonActions::RegisterActions();

  // Menu Bar
  {
    ezActionMapManager::RegisterActionMap("SkeletonAssetMenuBar", "AssetMenuBar").AssertSuccess();
  }

  // Tool Bar
  {
    ezActionMapManager::RegisterActionMap("SkeletonAssetToolBar", "AssetToolbar").AssertSuccess();
    ezCommonAssetActions::MapToolbarActions("SkeletonAssetToolBar", ezCommonAssetUiState::Grid);
    ezSkeletonActions::MapActions("SkeletonAssetToolBar");
  }

  // View Tool Bar
  {
    ezActionMapManager::RegisterActionMap("SkeletonAssetViewToolBar", "SimpleAssetViewToolbar").AssertSuccess();
  }
}

static void ConfigureAnimatedMeshAsset()
{
  // Menu Bar
  {
    ezActionMapManager::RegisterActionMap("AnimatedMeshAssetMenuBar", "AssetMenuBar").AssertSuccess();
  }

  // Tool Bar
  {
    ezActionMapManager::RegisterActionMap("AnimatedMeshAssetToolBar", "AssetToolbar").AssertSuccess();
    ezCommonAssetActions::MapToolbarActions("AnimatedMeshAssetToolBar", ezCommonAssetUiState::Grid);
  }

  // View Tool Bar
  {
    ezActionMapManager::RegisterActionMap("AnimatedMeshAssetViewToolBar", "SimpleAssetViewToolbar").AssertSuccess();
  }
}

static void ConfigureImageDataAsset()
{
  // Menu Bar
  {
    ezActionMapManager::RegisterActionMap("ImageDataAssetMenuBar", "AssetMenuBar").AssertSuccess();
  }

  // Tool Bar
  {
    ezActionMapManager::RegisterActionMap("ImageDataAssetToolBar", "AssetToolbar").AssertSuccess();
  }

  // View Tool Bar
  {
    ezActionMapManager::RegisterActionMap("ImageDataAssetViewToolBar", "SimpleAssetViewToolbar").AssertSuccess();
  }
}

static void ConfigureStateMachineAsset()
{
  // Menu Bar
  {
    ezActionMapManager::RegisterActionMap("StateMachineAssetMenuBar", "AssetMenuBar").AssertSuccess();
    ezEditActions::MapActions("StateMachineAssetMenuBar", false, false);
  }

  // Tool Bar
  {
    ezActionMapManager::RegisterActionMap("StateMachineAssetToolBar", "AssetToolbar").AssertSuccess();
  }
}
static void ConfigureBlackboardTemplateAsset()
{
  // Menu Bar
  {
    ezActionMapManager::RegisterActionMap("BlackboardTemplateAssetMenuBar", "AssetMenuBar").AssertSuccess();
    ezEditActions::MapActions("BlackboardTemplateAssetMenuBar", false, false);
  }

  // Tool Bar
  {
    ezActionMapManager::RegisterActionMap("BlackboardTemplateAssetToolBar", "AssetToolbar").AssertSuccess();
  }
}

static void ConfigureCustomDataAsset()
{
  // Menu Bar
  {
    ezActionMapManager::RegisterActionMap("CustomDataAssetMenuBar", "AssetMenuBar").AssertSuccess();
    ezDocumentActions::MapToolsActions("CustomDataAssetMenuBar");
  }

  // Tool Bar
  {
    ezActionMapManager::RegisterActionMap("CustomDataAssetToolBar", "AssetToolbar").AssertSuccess();
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
