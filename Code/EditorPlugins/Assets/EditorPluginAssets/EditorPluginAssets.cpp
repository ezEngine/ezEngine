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
#include <EditorPluginAssets/SkeletonAsset/SkeletonAsset.h>
#include <EditorPluginAssets/TextureAsset/TextureAssetObjects.h>
#include <EditorPluginAssets/TextureAsset/TextureAssetWindow.moc.h>
#include <EditorPluginAssets/TextureCubeAsset/TextureCubeAssetObjects.h>
#include <EditorPluginAssets/TextureCubeAsset/TextureCubeAssetWindow.moc.h>
#include <EditorPluginAssets/VisualScriptAsset/VisualScriptActions.h>
#include <EditorPluginAssets/VisualShader/VisualShaderActions.h>
#include <GuiFoundation/Action/ActionMapManager.h>
#include <GuiFoundation/Action/CommandHistoryActions.h>
#include <GuiFoundation/Action/DocumentActions.h>
#include <GuiFoundation/Action/EditActions.h>
#include <GuiFoundation/Action/StandardMenus.h>

static void ConfigureAnimationControllerAsset()
{
  // Menu Bar
  {
    ezActionMapManager::RegisterActionMap("AnimationControllerAssetMenuBar").IgnoreResult();

    ezStandardMenus::MapActions("AnimationControllerAssetMenuBar", ezStandardMenuTypes::File | ezStandardMenuTypes::Edit | ezStandardMenuTypes::Panels | ezStandardMenuTypes::Help);
    ezProjectActions::MapActions("AnimationControllerAssetMenuBar");
    ezDocumentActions::MapActions("AnimationControllerAssetMenuBar", "Menu.File", false);
    ezCommandHistoryActions::MapActions("AnimationControllerAssetMenuBar", "Menu.Edit");
    ezEditActions::MapActions("AnimationControllerAssetMenuBar", "Menu.Edit", false, false);
  }

  // Tool Bar
  {
    ezActionMapManager::RegisterActionMap("AnimationControllerAssetToolBar").IgnoreResult();
    ezDocumentActions::MapActions("AnimationControllerAssetToolBar", "", true);
    ezCommandHistoryActions::MapActions("AnimationControllerAssetToolBar", "");
    ezAssetActions::MapActions("AnimationControllerAssetToolBar", true);
  }
}

static void ConfigureTexture2DAsset()
{
  ezPropertyMetaState::GetSingleton()->m_Events.AddEventHandler(ezTextureAssetProperties::PropertyMetaStateEventHandler);

  ezTextureAssetActions::RegisterActions();

  // Menu Bar
  {
    ezActionMapManager::RegisterActionMap("TextureAssetMenuBar").IgnoreResult();
    ezStandardMenus::MapActions("TextureAssetMenuBar", ezStandardMenuTypes::File | ezStandardMenuTypes::Edit | ezStandardMenuTypes::Panels | ezStandardMenuTypes::Help);
    ezProjectActions::MapActions("TextureAssetMenuBar");
    ezDocumentActions::MapActions("TextureAssetMenuBar", "Menu.File", false);
    ezCommandHistoryActions::MapActions("TextureAssetMenuBar", "Menu.Edit");
  }

  // Tool Bar
  {
    ezActionMapManager::RegisterActionMap("TextureAssetToolBar").IgnoreResult();
    ezDocumentActions::MapActions("TextureAssetToolBar", "", true);
    ezCommandHistoryActions::MapActions("TextureAssetToolBar", "");
    ezAssetActions::MapActions("TextureAssetToolBar", true);
    ezTextureAssetActions::MapActions("TextureAssetToolBar", "");
  }
}

static void ConfigureTextureCubeAsset()
{
  ezPropertyMetaState::GetSingleton()->m_Events.AddEventHandler(ezTextureCubeAssetProperties::PropertyMetaStateEventHandler);

  ezTextureCubeAssetActions::RegisterActions();

  // Menu Bar
  {
    ezActionMapManager::RegisterActionMap("TextureCubeAssetMenuBar").IgnoreResult();
    ezStandardMenus::MapActions("TextureCubeAssetMenuBar", ezStandardMenuTypes::File | ezStandardMenuTypes::Edit | ezStandardMenuTypes::Panels | ezStandardMenuTypes::Help);
    ezProjectActions::MapActions("TextureCubeAssetMenuBar");
    ezDocumentActions::MapActions("TextureCubeAssetMenuBar", "Menu.File", false);
    ezCommandHistoryActions::MapActions("TextureCubeAssetMenuBar", "Menu.Edit");
  }

  // Tool Bar
  {
    ezActionMapManager::RegisterActionMap("TextureCubeAssetToolBar").IgnoreResult();
    ezDocumentActions::MapActions("TextureCubeAssetToolBar", "", true);
    ezCommandHistoryActions::MapActions("TextureCubeAssetToolBar", "");
    ezAssetActions::MapActions("TextureCubeAssetToolBar", true);
    ezTextureAssetActions::MapActions("TextureCubeAssetToolBar", "");
  }
}

static void ConfigureLUTAsset()
{
  ezPropertyMetaState::GetSingleton()->m_Events.AddEventHandler(ezLUTAssetProperties::PropertyMetaStateEventHandler);

  ezLUTAssetActions::RegisterActions();

  // Menu Bar
  {
    ezActionMapManager::RegisterActionMap("LUTAssetMenuBar").IgnoreResult();
    ezStandardMenus::MapActions("LUTAssetMenuBar", ezStandardMenuTypes::File | ezStandardMenuTypes::Edit | ezStandardMenuTypes::Panels | ezStandardMenuTypes::Help);
    ezProjectActions::MapActions("LUTAssetMenuBar");
    ezDocumentActions::MapActions("LUTAssetMenuBar", "Menu.File", false);
    ezCommandHistoryActions::MapActions("LUTAssetMenuBar", "Menu.Edit");
  }

  // Tool Bar
  {
    ezActionMapManager::RegisterActionMap("LUTAssetToolBar").IgnoreResult();
    ezDocumentActions::MapActions("LUTAssetToolBar", "", true);
    ezCommandHistoryActions::MapActions("LUTAssetToolBar", "");
    ezAssetActions::MapActions("LUTAssetToolBar", true);
  }
}

static void ConfigureMaterialAsset()
{
  ezPropertyMetaState::GetSingleton()->m_Events.AddEventHandler(ezMaterialAssetProperties::PropertyMetaStateEventHandler);

  // Menu Bar
  {
    ezActionMapManager::RegisterActionMap("MaterialAssetMenuBar").IgnoreResult();
    ezStandardMenus::MapActions("MaterialAssetMenuBar", ezStandardMenuTypes::File | ezStandardMenuTypes::Edit | ezStandardMenuTypes::Panels | ezStandardMenuTypes::Help);
    ezProjectActions::MapActions("MaterialAssetMenuBar");
    ezDocumentActions::MapActions("MaterialAssetMenuBar", "Menu.File", false);
    ezDocumentActions::MapToolsActions("MaterialAssetMenuBar", "Menu.Tools");
    ezCommandHistoryActions::MapActions("MaterialAssetMenuBar", "Menu.Edit");
    ezEditActions::MapActions("MaterialAssetMenuBar", "Menu.Edit", false, false);
  }

  // Tool Bar
  {
    ezActionMapManager::RegisterActionMap("MaterialAssetToolBar").IgnoreResult();
    ezDocumentActions::MapActions("MaterialAssetToolBar", "", true);
    ezCommandHistoryActions::MapActions("MaterialAssetToolBar", "");
    ezAssetActions::MapActions("MaterialAssetToolBar", true);

    ezMaterialAssetActions::RegisterActions();
    ezMaterialAssetActions::MapActions("MaterialAssetToolBar", "");

    ezVisualShaderActions::RegisterActions();
    ezVisualShaderActions::MapActions("MaterialAssetToolBar");
  }

  // View Tool Bar
  {
    ezActionMapManager::RegisterActionMap("MaterialAssetViewToolBar").IgnoreResult();
    ezViewActions::MapActions("MaterialAssetViewToolBar", "", ezViewActions::RenderMode | ezViewActions::ActivateRemoteProcess);
    ezViewLightActions::MapActions("MaterialAssetViewToolBar", "");
  }
}

static void ConfigureRenderPipelineAsset()
{
  // Menu Bar
  {
    ezActionMapManager::RegisterActionMap("RenderPipelineAssetMenuBar").IgnoreResult();

    ezStandardMenus::MapActions("RenderPipelineAssetMenuBar", ezStandardMenuTypes::File | ezStandardMenuTypes::Edit | ezStandardMenuTypes::Panels | ezStandardMenuTypes::Help);
    ezProjectActions::MapActions("RenderPipelineAssetMenuBar");
    ezDocumentActions::MapActions("RenderPipelineAssetMenuBar", "Menu.File", false);
    ezCommandHistoryActions::MapActions("RenderPipelineAssetMenuBar", "Menu.Edit");
    ezEditActions::MapActions("RenderPipelineAssetMenuBar", "Menu.Edit", false, false);
  }

  // Tool Bar
  {
    ezActionMapManager::RegisterActionMap("RenderPipelineAssetToolBar").IgnoreResult();
    ezDocumentActions::MapActions("RenderPipelineAssetToolBar", "", true);
    ezCommandHistoryActions::MapActions("RenderPipelineAssetToolBar", "");
    ezAssetActions::MapActions("RenderPipelineAssetToolBar", true);
  }
}

static void ConfigureMeshAsset()
{
  ezPropertyMetaState::GetSingleton()->m_Events.AddEventHandler(ezMeshAssetProperties::PropertyMetaStateEventHandler);

  // Menu Bar
  {
    ezActionMapManager::RegisterActionMap("MeshAssetMenuBar").IgnoreResult();
    ezStandardMenus::MapActions("MeshAssetMenuBar", ezStandardMenuTypes::File | ezStandardMenuTypes::Edit | ezStandardMenuTypes::Panels | ezStandardMenuTypes::Help);
    ezProjectActions::MapActions("MeshAssetMenuBar");
    ezDocumentActions::MapActions("MeshAssetMenuBar", "Menu.File", false);
    ezCommandHistoryActions::MapActions("MeshAssetMenuBar", "Menu.Edit");
  }

  // Tool Bar
  {
    ezActionMapManager::RegisterActionMap("MeshAssetToolBar").IgnoreResult();
    ezDocumentActions::MapActions("MeshAssetToolBar", "", true);
    ezCommandHistoryActions::MapActions("MeshAssetToolBar", "");
    ezAssetActions::MapActions("MeshAssetToolBar", true);
    ezCommonAssetActions::MapActions("MeshAssetToolBar", "", ezCommonAssetUiState::Grid);
  }

  // View Tool Bar
  {
    ezActionMapManager::RegisterActionMap("MeshAssetViewToolBar").IgnoreResult();
    ezViewActions::MapActions("MeshAssetViewToolBar", "", ezViewActions::RenderMode | ezViewActions::ActivateRemoteProcess);
    ezViewLightActions::MapActions("MeshAssetViewToolBar", "");
  }
}

static void ConfigureSurfaceAsset()
{
  // Menu Bar
  {
    ezActionMapManager::RegisterActionMap("SurfaceAssetMenuBar").IgnoreResult();
    ezStandardMenus::MapActions("SurfaceAssetMenuBar", ezStandardMenuTypes::File | ezStandardMenuTypes::Edit | ezStandardMenuTypes::Panels | ezStandardMenuTypes::Help);
    ezProjectActions::MapActions("SurfaceAssetMenuBar");
    ezDocumentActions::MapActions("SurfaceAssetMenuBar", "Menu.File", false);
    ezDocumentActions::MapToolsActions("SurfaceAssetMenuBar", "Menu.Tools");
    ezCommandHistoryActions::MapActions("SurfaceAssetMenuBar", "Menu.Edit");
  }

  // Tool Bar
  {
    ezActionMapManager::RegisterActionMap("SurfaceAssetToolBar").IgnoreResult();
    ezDocumentActions::MapActions("SurfaceAssetToolBar", "", true);
    ezCommandHistoryActions::MapActions("SurfaceAssetToolBar", "");
    ezAssetActions::MapActions("SurfaceAssetToolBar", true);
  }
}

static void ConfigureCollectionAsset()
{
  // Menu Bar
  {
    ezActionMapManager::RegisterActionMap("CollectionAssetMenuBar").IgnoreResult();
    ezStandardMenus::MapActions("CollectionAssetMenuBar", ezStandardMenuTypes::File | ezStandardMenuTypes::Edit | ezStandardMenuTypes::Panels | ezStandardMenuTypes::Help);
    ezProjectActions::MapActions("CollectionAssetMenuBar");
    ezDocumentActions::MapActions("CollectionAssetMenuBar", "Menu.File", false);
    ezDocumentActions::MapToolsActions("CollectionAssetMenuBar", "Menu.Tools");
    ezCommandHistoryActions::MapActions("CollectionAssetMenuBar", "Menu.Edit");
  }

  // Tool Bar
  {
    ezActionMapManager::RegisterActionMap("CollectionAssetToolBar").IgnoreResult();
    ezDocumentActions::MapActions("CollectionAssetToolBar", "", true);
    ezCommandHistoryActions::MapActions("CollectionAssetToolBar", "");
    ezAssetActions::MapActions("CollectionAssetToolBar", true);
  }
}

static void ConfigureColorGradientAsset()
{
  // Menu Bar
  {
    ezActionMapManager::RegisterActionMap("ColorGradientAssetMenuBar").IgnoreResult();
    ezStandardMenus::MapActions("ColorGradientAssetMenuBar", ezStandardMenuTypes::File | ezStandardMenuTypes::Edit | ezStandardMenuTypes::Panels | ezStandardMenuTypes::Help);
    ezProjectActions::MapActions("ColorGradientAssetMenuBar");
    ezDocumentActions::MapActions("ColorGradientAssetMenuBar", "Menu.File", false);
    ezDocumentActions::MapToolsActions("ColorGradientAssetMenuBar", "Menu.Tools");
    ezCommandHistoryActions::MapActions("ColorGradientAssetMenuBar", "Menu.Edit");
  }

  // Tool Bar
  {
    ezActionMapManager::RegisterActionMap("ColorGradientAssetToolBar").IgnoreResult();
    ezDocumentActions::MapActions("ColorGradientAssetToolBar", "", true);
    ezCommandHistoryActions::MapActions("ColorGradientAssetToolBar", "");
    ezAssetActions::MapActions("ColorGradientAssetToolBar", true);
  }
}

static void ConfigureCurve1DAsset()
{
  // Menu Bar
  {
    ezActionMapManager::RegisterActionMap("Curve1DAssetMenuBar").IgnoreResult();
    ezStandardMenus::MapActions("Curve1DAssetMenuBar", ezStandardMenuTypes::File | ezStandardMenuTypes::Edit | ezStandardMenuTypes::Panels | ezStandardMenuTypes::Help);
    ezProjectActions::MapActions("Curve1DAssetMenuBar");
    ezDocumentActions::MapActions("Curve1DAssetMenuBar", "Menu.File", false);
    ezDocumentActions::MapToolsActions("Curve1DAssetMenuBar", "Menu.Tools");
    ezCommandHistoryActions::MapActions("Curve1DAssetMenuBar", "Menu.Edit");
  }

  // Tool Bar
  {
    ezActionMapManager::RegisterActionMap("Curve1DAssetToolBar").IgnoreResult();
    ezDocumentActions::MapActions("Curve1DAssetToolBar", "", true);
    ezCommandHistoryActions::MapActions("Curve1DAssetToolBar", "");
    ezAssetActions::MapActions("Curve1DAssetToolBar", true);
  }
}

static void ConfigurePropertyAnimAsset()
{
  // Menu Bar
  {
    ezActionMapManager::RegisterActionMap("PropertyAnimAssetMenuBar").IgnoreResult();
    ezStandardMenus::MapActions("PropertyAnimAssetMenuBar", ezStandardMenuTypes::File | ezStandardMenuTypes::Edit | ezStandardMenuTypes::Panels | ezStandardMenuTypes::Scene | ezStandardMenuTypes::View | ezStandardMenuTypes::Help);
    ezProjectActions::MapActions("PropertyAnimAssetMenuBar");
    ezDocumentActions::MapActions("PropertyAnimAssetMenuBar", "Menu.File", false);
    ezDocumentActions::MapToolsActions("PropertyAnimAssetMenuBar", "Menu.Tools");
    ezCommandHistoryActions::MapActions("PropertyAnimAssetMenuBar", "Menu.Edit");
    ezGameObjectSelectionActions::MapActions("PropertyAnimAssetMenuBar", "Menu.Edit");
    ezGameObjectDocumentActions::MapMenuActions("PropertyAnimAssetMenuBar", "Menu.View");
    ezGameObjectDocumentActions::MapMenuSimulationSpeed("PropertyAnimAssetMenuBar", "Menu.Scene");
    ezTransformGizmoActions::MapMenuActions("PropertyAnimAssetMenuBar", "Menu.Edit");
    ezTranslateGizmoAction::MapActions("PropertyAnimAssetMenuBar", "Menu.Edit/Gizmo.Menu");
  }

  // Tool Bar
  {
    ezActionMapManager::RegisterActionMap("PropertyAnimAssetToolBar").IgnoreResult();
    ezDocumentActions::MapActions("PropertyAnimAssetToolBar", "", true);
    ezCommandHistoryActions::MapActions("PropertyAnimAssetToolBar", "");
    ezAssetActions::MapActions("PropertyAnimAssetToolBar", true);
    ezGameObjectContextActions::MapToolbarActions("PropertyAnimAssetToolBar", "");
    ezGameObjectDocumentActions::MapToolbarActions("PropertyAnimAssetToolBar", "");
    ezTransformGizmoActions::MapToolbarActions("PropertyAnimAssetToolBar", "");
  }

  // View Tool Bar
  {
    ezActionMapManager::RegisterActionMap("PropertyAnimAssetViewToolBar").IgnoreResult();
    ezViewActions::MapActions("PropertyAnimAssetViewToolBar", "", ezViewActions::PerspectiveMode | ezViewActions::RenderMode | ezViewActions::ActivateRemoteProcess);
    ezQuadViewActions::MapActions("PropertyAnimAssetViewToolBar", "");
  }

  // SceneGraph Context Menu
  {
    ezActionMapManager::RegisterActionMap("PropertyAnimAsset_ScenegraphContextMenu").IgnoreResult();
    ezGameObjectSelectionActions::MapContextMenuActions("PropertyAnimAsset_ScenegraphContextMenu", "");
    ezGameObjectContextActions::MapContextMenuActions("PropertyAnimAsset_ScenegraphContextMenu", "");
  }
}

static void ConfigureVisualScriptAsset()
{
  ezVisualScriptActions::RegisterActions();

  // Menu Bar
  {
    ezActionMapManager::RegisterActionMap("VisualScriptAssetMenuBar").IgnoreResult();
    ezStandardMenus::MapActions("VisualScriptAssetMenuBar", ezStandardMenuTypes::File | ezStandardMenuTypes::Edit | ezStandardMenuTypes::Panels | ezStandardMenuTypes::Help);
    ezProjectActions::MapActions("VisualScriptAssetMenuBar");
    ezDocumentActions::MapActions("VisualScriptAssetMenuBar", "Menu.File", false);
    ezCommandHistoryActions::MapActions("VisualScriptAssetMenuBar", "Menu.Edit");
    ezEditActions::MapActions("VisualScriptAssetMenuBar", "Menu.Edit", false, false);
  }

  // Tool Bar
  {
    ezActionMapManager::RegisterActionMap("VisualScriptAssetToolBar").IgnoreResult();
    ezDocumentActions::MapActions("VisualScriptAssetToolBar", "", true);
    ezCommandHistoryActions::MapActions("VisualScriptAssetToolBar", "");
    ezAssetActions::MapActions("VisualScriptAssetToolBar", true);
    ezVisualScriptActions::MapActions("VisualScriptAssetToolBar", "");
  }
}

static void ConfigureDecalAsset()
{
  ezPropertyMetaState::GetSingleton()->m_Events.AddEventHandler(ezDecalAssetProperties::PropertyMetaStateEventHandler);

  // Menu Bar
  {
    ezActionMapManager::RegisterActionMap("DecalAssetMenuBar").IgnoreResult();
    ezStandardMenus::MapActions("DecalAssetMenuBar", ezStandardMenuTypes::File | ezStandardMenuTypes::Edit | ezStandardMenuTypes::Panels | ezStandardMenuTypes::Help);
    ezProjectActions::MapActions("DecalAssetMenuBar");
    ezDocumentActions::MapActions("DecalAssetMenuBar", "Menu.File", false);
    ezCommandHistoryActions::MapActions("DecalAssetMenuBar", "Menu.Edit");
  }

  // Tool Bar
  {
    ezActionMapManager::RegisterActionMap("DecalAssetToolBar").IgnoreResult();
    ezDocumentActions::MapActions("DecalAssetToolBar", "", true);
    ezCommandHistoryActions::MapActions("DecalAssetToolBar", "");
    ezAssetActions::MapActions("DecalAssetToolBar", true);
  }

  // View Tool Bar
  {
    ezActionMapManager::RegisterActionMap("DecalAssetViewToolBar").IgnoreResult();
    ezViewActions::MapActions("DecalAssetViewToolBar", "", ezViewActions::RenderMode | ezViewActions::ActivateRemoteProcess);
    ezViewLightActions::MapActions("DecalAssetViewToolBar", "");
  }
}

static void ConfigureAnimationClipAsset()
{
  ezPropertyMetaState::GetSingleton()->m_Events.AddEventHandler(ezAnimationClipAssetProperties::PropertyMetaStateEventHandler);

  // Menu Bar
  {
    ezActionMapManager::RegisterActionMap("AnimationClipAssetMenuBar").IgnoreResult();
    ezStandardMenus::MapActions("AnimationClipAssetMenuBar", ezStandardMenuTypes::File | ezStandardMenuTypes::Edit | ezStandardMenuTypes::Panels | ezStandardMenuTypes::Help);
    ezProjectActions::MapActions("AnimationClipAssetMenuBar");
    ezDocumentActions::MapActions("AnimationClipAssetMenuBar", "Menu.File", false);
    ezCommandHistoryActions::MapActions("AnimationClipAssetMenuBar", "Menu.Edit");
  }

  // Tool Bar
  {
    ezActionMapManager::RegisterActionMap("AnimationClipAssetToolBar").IgnoreResult();
    ezDocumentActions::MapActions("AnimationClipAssetToolBar", "", true);
    ezCommandHistoryActions::MapActions("AnimationClipAssetToolBar", "");
    ezAssetActions::MapActions("AnimationClipAssetToolBar", true);
    ezCommonAssetActions::MapActions("AnimationClipAssetToolBar", "", ezCommonAssetUiState::Loop | ezCommonAssetUiState::Pause | ezCommonAssetUiState::Restart | ezCommonAssetUiState::SimulationSpeed | ezCommonAssetUiState::Grid);
  }

  // View Tool Bar
  {
    ezActionMapManager::RegisterActionMap("AnimationClipAssetViewToolBar").IgnoreResult();
    ezViewActions::MapActions("AnimationClipAssetViewToolBar", "", ezViewActions::RenderMode | ezViewActions::ActivateRemoteProcess);
    ezViewLightActions::MapActions("AnimationClipAssetViewToolBar", "");
  }
}

static void ConfigureSkeletonAsset()
{
  ezPropertyMetaState::GetSingleton()->m_Events.AddEventHandler(ezSkeletonAssetDocument::PropertyMetaStateEventHandler);

  // Menu Bar
  {
    ezActionMapManager::RegisterActionMap("SkeletonAssetMenuBar").IgnoreResult();
    ezStandardMenus::MapActions("SkeletonAssetMenuBar", ezStandardMenuTypes::File | ezStandardMenuTypes::Edit | ezStandardMenuTypes::Panels | ezStandardMenuTypes::Help);
    ezProjectActions::MapActions("SkeletonAssetMenuBar");
    ezDocumentActions::MapActions("SkeletonAssetMenuBar", "Menu.File", false);
    ezCommandHistoryActions::MapActions("SkeletonAssetMenuBar", "Menu.Edit");
  }

  // Tool Bar
  {
    ezActionMapManager::RegisterActionMap("SkeletonAssetToolBar").IgnoreResult();
    ezDocumentActions::MapActions("SkeletonAssetToolBar", "", true);
    ezCommandHistoryActions::MapActions("SkeletonAssetToolBar", "");
    ezAssetActions::MapActions("SkeletonAssetToolBar", true);
    ezCommonAssetActions::MapActions("SkeletonAssetToolBar", "", ezCommonAssetUiState::Grid);
  }

  // View Tool Bar
  {
    ezActionMapManager::RegisterActionMap("SkeletonAssetViewToolBar").IgnoreResult();
    ezViewActions::MapActions("SkeletonAssetViewToolBar", "", ezViewActions::RenderMode | ezViewActions::ActivateRemoteProcess);
    ezViewLightActions::MapActions("SkeletonAssetViewToolBar", "");
  }
}

static void ConfigureAnimatedMeshAsset()
{
  // Menu Bar
  {
    ezActionMapManager::RegisterActionMap("AnimatedMeshAssetMenuBar").IgnoreResult();
    ezStandardMenus::MapActions("AnimatedMeshAssetMenuBar", ezStandardMenuTypes::File | ezStandardMenuTypes::Edit | ezStandardMenuTypes::Panels | ezStandardMenuTypes::Help);
    ezProjectActions::MapActions("AnimatedMeshAssetMenuBar");
    ezDocumentActions::MapActions("AnimatedMeshAssetMenuBar", "Menu.File", false);
    ezCommandHistoryActions::MapActions("AnimatedMeshAssetMenuBar", "Menu.Edit");
  }

  // Tool Bar
  {
    ezActionMapManager::RegisterActionMap("AnimatedMeshAssetToolBar").IgnoreResult();
    ezDocumentActions::MapActions("AnimatedMeshAssetToolBar", "", true);
    ezCommandHistoryActions::MapActions("AnimatedMeshAssetToolBar", "");
    ezAssetActions::MapActions("AnimatedMeshAssetToolBar", true);
    ezCommonAssetActions::MapActions("AnimatedMeshAssetToolBar", "", ezCommonAssetUiState::Grid);
  }

  // View Tool Bar
  {
    ezActionMapManager::RegisterActionMap("AnimatedMeshAssetViewToolBar").IgnoreResult();
    ezViewActions::MapActions("AnimatedMeshAssetViewToolBar", "", ezViewActions::RenderMode | ezViewActions::ActivateRemoteProcess);
    ezViewLightActions::MapActions("AnimatedMeshAssetViewToolBar", "");
  }
}

static void ConfigureImageDataAsset()
{
  // Menu Bar
  {
    ezActionMapManager::RegisterActionMap("ImageDataAssetMenuBar").IgnoreResult();
    ezStandardMenus::MapActions("ImageDataAssetMenuBar", ezStandardMenuTypes::File | ezStandardMenuTypes::Edit | ezStandardMenuTypes::Panels | ezStandardMenuTypes::Help);
    ezProjectActions::MapActions("ImageDataAssetMenuBar");
    ezDocumentActions::MapActions("ImageDataAssetMenuBar", "Menu.File", false);
    ezCommandHistoryActions::MapActions("ImageDataAssetMenuBar", "Menu.Edit");
  }

  // Tool Bar
  {
    ezActionMapManager::RegisterActionMap("ImageDataAssetToolBar").IgnoreResult();
    ezDocumentActions::MapActions("ImageDataAssetToolBar", "", true);
    ezCommandHistoryActions::MapActions("ImageDataAssetToolBar", "");
    ezAssetActions::MapActions("ImageDataAssetToolBar", true);
  }

  // View Tool Bar
  {
    ezActionMapManager::RegisterActionMap("ImageDataAssetViewToolBar").IgnoreResult();
    ezViewActions::MapActions("ImageDataAssetViewToolBar", "", ezViewActions::RenderMode | ezViewActions::ActivateRemoteProcess);
    ezViewLightActions::MapActions("ImageDataAssetViewToolBar", "");
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
  ezQtEditorApp::GetSingleton()->AddRuntimePluginDependency("EditorPluginAssets", "ezEnginePluginAssets");

  ConfigureAnimationControllerAsset();
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
  ConfigureVisualScriptAsset();
  ConfigureDecalAsset();
  ConfigureAnimationClipAsset();
  ConfigureSkeletonAsset();
  ConfigureAnimatedMeshAsset();
  ConfigureImageDataAsset();

  ezDocumentManager::s_CustomActions["CustomAction_CreateShaderFromTemplate"] = CustomAction_CreateShaderFromTemplate;
}

void OnUnloadPlugin()
{
  ezTextureAssetActions::UnregisterActions();
  ezTextureCubeAssetActions::UnregisterActions();
  ezLUTAssetActions::UnregisterActions();
  ezVisualShaderActions::UnregisterActions();
  ezVisualScriptActions::UnregisterActions();
  ezMaterialAssetActions::UnregisterActions();

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
