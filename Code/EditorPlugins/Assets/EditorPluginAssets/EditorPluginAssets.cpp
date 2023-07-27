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
#include <EditorPluginAssets/VisualScriptAsset/VisualScriptActions.h>
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
    ezActionMapManager::RegisterActionMap("AnimationGraphAssetMenuBar").AssertSuccess();

    ezStandardMenus::MapActions("AnimationGraphAssetMenuBar", ezStandardMenuTypes::Default | ezStandardMenuTypes::Edit);
    ezProjectActions::MapActions("AnimationGraphAssetMenuBar");
    ezDocumentActions::MapMenuActions("AnimationGraphAssetMenuBar");
    ezAssetActions::MapMenuActions("AnimationGraphAssetMenuBar");
    ezCommandHistoryActions::MapActions("AnimationGraphAssetMenuBar");
    ezEditActions::MapActions("AnimationGraphAssetMenuBar", false, false);
  }

  // Tool Bar
  {
    ezActionMapManager::RegisterActionMap("AnimationGraphAssetToolBar").AssertSuccess();
    ezDocumentActions::MapToolbarActions("AnimationGraphAssetToolBar");
    ezCommandHistoryActions::MapActions("AnimationGraphAssetToolBar", "");
    ezAssetActions::MapToolBarActions("AnimationGraphAssetToolBar", true);
  }
}

static void ConfigureTexture2DAsset()
{
  ezPropertyMetaState::GetSingleton()->m_Events.AddEventHandler(ezTextureAssetProperties::PropertyMetaStateEventHandler);

  ezTextureAssetActions::RegisterActions();

  // Menu Bar
  {
    ezActionMapManager::RegisterActionMap("TextureAssetMenuBar").AssertSuccess();
    ezStandardMenus::MapActions("TextureAssetMenuBar", ezStandardMenuTypes::Default | ezStandardMenuTypes::Edit);
    ezProjectActions::MapActions("TextureAssetMenuBar");
    ezDocumentActions::MapMenuActions("TextureAssetMenuBar");
    ezAssetActions::MapMenuActions("TextureAssetMenuBar");
    ezCommandHistoryActions::MapActions("TextureAssetMenuBar");
  }

  // Tool Bar
  {
    ezActionMapManager::RegisterActionMap("TextureAssetToolBar").AssertSuccess();
    ezDocumentActions::MapToolbarActions("TextureAssetToolBar");
    ezCommandHistoryActions::MapActions("TextureAssetToolBar", "");
    ezAssetActions::MapToolBarActions("TextureAssetToolBar", true);
    ezTextureAssetActions::MapToolbarActions("TextureAssetToolBar");
  }
}

static void ConfigureTextureCubeAsset()
{
  ezPropertyMetaState::GetSingleton()->m_Events.AddEventHandler(ezTextureCubeAssetProperties::PropertyMetaStateEventHandler);

  // Menu Bar
  {
    ezActionMapManager::RegisterActionMap("TextureCubeAssetMenuBar").AssertSuccess();
    ezStandardMenus::MapActions("TextureCubeAssetMenuBar", ezStandardMenuTypes::Default | ezStandardMenuTypes::Edit);
    ezProjectActions::MapActions("TextureCubeAssetMenuBar");
    ezDocumentActions::MapMenuActions("TextureCubeAssetMenuBar");
    ezAssetActions::MapMenuActions("TextureCubeAssetMenuBar");
    ezCommandHistoryActions::MapActions("TextureCubeAssetMenuBar");
  }

  // Tool Bar
  {
    ezActionMapManager::RegisterActionMap("TextureCubeAssetToolBar").AssertSuccess();
    ezDocumentActions::MapToolbarActions("TextureCubeAssetToolBar");
    ezCommandHistoryActions::MapActions("TextureCubeAssetToolBar", "");
    ezAssetActions::MapToolBarActions("TextureCubeAssetToolBar", true);
    ezTextureAssetActions::MapToolbarActions("TextureCubeAssetToolBar");
  }
}

static void ConfigureLUTAsset()
{
  ezPropertyMetaState::GetSingleton()->m_Events.AddEventHandler(ezLUTAssetProperties::PropertyMetaStateEventHandler);

  ezLUTAssetActions::RegisterActions();

  // Menu Bar
  {
    ezActionMapManager::RegisterActionMap("LUTAssetMenuBar").AssertSuccess();
    ezStandardMenus::MapActions("LUTAssetMenuBar", ezStandardMenuTypes::Default | ezStandardMenuTypes::Edit);
    ezProjectActions::MapActions("LUTAssetMenuBar");
    ezDocumentActions::MapMenuActions("LUTAssetMenuBar");
    ezAssetActions::MapMenuActions("LUTAssetMenuBar");
    ezCommandHistoryActions::MapActions("LUTAssetMenuBar");
  }

  // Tool Bar
  {
    ezActionMapManager::RegisterActionMap("LUTAssetToolBar").AssertSuccess();
    ezDocumentActions::MapToolbarActions("LUTAssetToolBar");
    ezCommandHistoryActions::MapActions("LUTAssetToolBar", "");
    ezAssetActions::MapToolBarActions("LUTAssetToolBar", true);
  }
}

static void ConfigureMaterialAsset()
{
  ezPropertyMetaState::GetSingleton()->m_Events.AddEventHandler(ezMaterialAssetProperties::PropertyMetaStateEventHandler);

  // Menu Bar
  {
    ezActionMapManager::RegisterActionMap("MaterialAssetMenuBar").AssertSuccess();
    ezStandardMenus::MapActions("MaterialAssetMenuBar", ezStandardMenuTypes::Default | ezStandardMenuTypes::Edit);
    ezProjectActions::MapActions("MaterialAssetMenuBar");
    ezDocumentActions::MapMenuActions("MaterialAssetMenuBar");
    ezDocumentActions::MapToolsActions("MaterialAssetMenuBar");
    ezAssetActions::MapMenuActions("MaterialAssetMenuBar");
    ezCommandHistoryActions::MapActions("MaterialAssetMenuBar");
    ezEditActions::MapActions("MaterialAssetMenuBar", false, false);
  }

  // Tool Bar
  {
    ezActionMapManager::RegisterActionMap("MaterialAssetToolBar").AssertSuccess();
    ezDocumentActions::MapToolbarActions("MaterialAssetToolBar");
    ezCommandHistoryActions::MapActions("MaterialAssetToolBar", "");
    ezAssetActions::MapToolBarActions("MaterialAssetToolBar", true);

    ezMaterialAssetActions::RegisterActions();
    ezMaterialAssetActions::MapToolbarActions("MaterialAssetToolBar");

    ezVisualShaderActions::RegisterActions();
    ezVisualShaderActions::MapActions("MaterialAssetToolBar");
  }

  // View Tool Bar
  {
    ezActionMapManager::RegisterActionMap("MaterialAssetViewToolBar").AssertSuccess();
    ezViewActions::MapToolbarActions("MaterialAssetViewToolBar", ezViewActions::RenderMode | ezViewActions::ActivateRemoteProcess);
    ezViewLightActions::MapToolbarActions("MaterialAssetViewToolBar");
  }
}

static void ConfigureRenderPipelineAsset()
{
  // Menu Bar
  {
    ezActionMapManager::RegisterActionMap("RenderPipelineAssetMenuBar").AssertSuccess();

    ezStandardMenus::MapActions("RenderPipelineAssetMenuBar", ezStandardMenuTypes::Default | ezStandardMenuTypes::Edit);
    ezProjectActions::MapActions("RenderPipelineAssetMenuBar");
    ezDocumentActions::MapMenuActions("RenderPipelineAssetMenuBar");
    ezAssetActions::MapMenuActions("RenderPipelineAssetMenuBar");
    ezCommandHistoryActions::MapActions("RenderPipelineAssetMenuBar");
    ezEditActions::MapActions("RenderPipelineAssetMenuBar", false, false);
  }

  // Tool Bar
  {
    ezActionMapManager::RegisterActionMap("RenderPipelineAssetToolBar").AssertSuccess();
    ezDocumentActions::MapToolbarActions("RenderPipelineAssetToolBar");
    ezCommandHistoryActions::MapActions("RenderPipelineAssetToolBar", "");
    ezAssetActions::MapToolBarActions("RenderPipelineAssetToolBar", true);
  }
}

static void ConfigureMeshAsset()
{
  ezPropertyMetaState::GetSingleton()->m_Events.AddEventHandler(ezMeshAssetProperties::PropertyMetaStateEventHandler);

  // Menu Bar
  {
    ezActionMapManager::RegisterActionMap("MeshAssetMenuBar").AssertSuccess();
    ezStandardMenus::MapActions("MeshAssetMenuBar", ezStandardMenuTypes::Default | ezStandardMenuTypes::Edit);
    ezProjectActions::MapActions("MeshAssetMenuBar");
    ezDocumentActions::MapMenuActions("MeshAssetMenuBar");
    ezAssetActions::MapMenuActions("MeshAssetMenuBar");
    ezCommandHistoryActions::MapActions("MeshAssetMenuBar");
  }

  // Tool Bar
  {
    ezActionMapManager::RegisterActionMap("MeshAssetToolBar").AssertSuccess();
    ezDocumentActions::MapToolbarActions("MeshAssetToolBar");
    ezCommandHistoryActions::MapActions("MeshAssetToolBar", "");
    ezAssetActions::MapToolBarActions("MeshAssetToolBar", true);
    ezCommonAssetActions::MapToolbarActions("MeshAssetToolBar", ezCommonAssetUiState::Grid);
  }

  // View Tool Bar
  {
    ezActionMapManager::RegisterActionMap("MeshAssetViewToolBar").AssertSuccess();
    ezViewActions::MapToolbarActions("MeshAssetViewToolBar", ezViewActions::RenderMode | ezViewActions::ActivateRemoteProcess);
    ezViewLightActions::MapToolbarActions("MeshAssetViewToolBar");
  }
}

static void ConfigureSurfaceAsset()
{
  // Menu Bar
  {
    ezActionMapManager::RegisterActionMap("SurfaceAssetMenuBar").AssertSuccess();
    ezStandardMenus::MapActions("SurfaceAssetMenuBar", ezStandardMenuTypes::Default | ezStandardMenuTypes::Edit);
    ezProjectActions::MapActions("SurfaceAssetMenuBar");
    ezDocumentActions::MapMenuActions("SurfaceAssetMenuBar");
    ezAssetActions::MapMenuActions("SurfaceAssetMenuBar");
    ezDocumentActions::MapToolsActions("SurfaceAssetMenuBar");
    ezCommandHistoryActions::MapActions("SurfaceAssetMenuBar");
  }

  // Tool Bar
  {
    ezActionMapManager::RegisterActionMap("SurfaceAssetToolBar").AssertSuccess();
    ezDocumentActions::MapToolbarActions("SurfaceAssetToolBar");
    ezCommandHistoryActions::MapActions("SurfaceAssetToolBar", "");
    ezAssetActions::MapToolBarActions("SurfaceAssetToolBar", true);
  }
}

static void ConfigureCollectionAsset()
{
  // Menu Bar
  {
    ezActionMapManager::RegisterActionMap("CollectionAssetMenuBar").AssertSuccess();
    ezStandardMenus::MapActions("CollectionAssetMenuBar", ezStandardMenuTypes::Default | ezStandardMenuTypes::Edit);
    ezProjectActions::MapActions("CollectionAssetMenuBar");
    ezDocumentActions::MapMenuActions("CollectionAssetMenuBar");
    ezAssetActions::MapMenuActions("CollectionAssetMenuBar");
    ezDocumentActions::MapToolsActions("CollectionAssetMenuBar");
    ezCommandHistoryActions::MapActions("CollectionAssetMenuBar");
  }

  // Tool Bar
  {
    ezActionMapManager::RegisterActionMap("CollectionAssetToolBar").AssertSuccess();
    ezDocumentActions::MapToolbarActions("CollectionAssetToolBar");
    ezCommandHistoryActions::MapActions("CollectionAssetToolBar", "");
    ezAssetActions::MapToolBarActions("CollectionAssetToolBar", true);
  }
}

static void ConfigureColorGradientAsset()
{
  // Menu Bar
  {
    ezActionMapManager::RegisterActionMap("ColorGradientAssetMenuBar").AssertSuccess();
    ezStandardMenus::MapActions("ColorGradientAssetMenuBar", ezStandardMenuTypes::Default | ezStandardMenuTypes::Edit);
    ezProjectActions::MapActions("ColorGradientAssetMenuBar");
    ezDocumentActions::MapMenuActions("ColorGradientAssetMenuBar");
    ezAssetActions::MapMenuActions("ColorGradientAssetMenuBar");
    ezDocumentActions::MapToolsActions("ColorGradientAssetMenuBar");
    ezCommandHistoryActions::MapActions("ColorGradientAssetMenuBar");
  }

  // Tool Bar
  {
    ezActionMapManager::RegisterActionMap("ColorGradientAssetToolBar").AssertSuccess();
    ezDocumentActions::MapToolbarActions("ColorGradientAssetToolBar");
    ezCommandHistoryActions::MapActions("ColorGradientAssetToolBar", "");
    ezAssetActions::MapToolBarActions("ColorGradientAssetToolBar", true);
  }
}

static void ConfigureCurve1DAsset()
{
  // Menu Bar
  {
    ezActionMapManager::RegisterActionMap("Curve1DAssetMenuBar").AssertSuccess();
    ezStandardMenus::MapActions("Curve1DAssetMenuBar", ezStandardMenuTypes::Default | ezStandardMenuTypes::Edit);
    ezProjectActions::MapActions("Curve1DAssetMenuBar");
    ezDocumentActions::MapMenuActions("Curve1DAssetMenuBar");
    ezAssetActions::MapMenuActions("Curve1DAssetMenuBar");
    ezDocumentActions::MapToolsActions("Curve1DAssetMenuBar");
    ezCommandHistoryActions::MapActions("Curve1DAssetMenuBar");
  }

  // Tool Bar
  {
    ezActionMapManager::RegisterActionMap("Curve1DAssetToolBar").AssertSuccess();
    ezDocumentActions::MapToolbarActions("Curve1DAssetToolBar");
    ezCommandHistoryActions::MapActions("Curve1DAssetToolBar", "");
    ezAssetActions::MapToolBarActions("Curve1DAssetToolBar", true);
  }
}

static void ConfigurePropertyAnimAsset()
{
  // Menu Bar
  {
    ezActionMapManager::RegisterActionMap("PropertyAnimAssetMenuBar").AssertSuccess();
    ezStandardMenus::MapActions("PropertyAnimAssetMenuBar", ezStandardMenuTypes::Default | ezStandardMenuTypes::Edit | ezStandardMenuTypes::Scene | ezStandardMenuTypes::View);
    ezProjectActions::MapActions("PropertyAnimAssetMenuBar");
    ezDocumentActions::MapMenuActions("PropertyAnimAssetMenuBar");
    ezAssetActions::MapMenuActions("PropertyAnimAssetMenuBar");
    ezDocumentActions::MapToolsActions("PropertyAnimAssetMenuBar");
    ezCommandHistoryActions::MapActions("PropertyAnimAssetMenuBar");
    ezGameObjectSelectionActions::MapActions("PropertyAnimAssetMenuBar");
    ezGameObjectDocumentActions::MapMenuActions("PropertyAnimAssetMenuBar");
    ezGameObjectDocumentActions::MapMenuSimulationSpeed("PropertyAnimAssetMenuBar");
    ezTransformGizmoActions::MapMenuActions("PropertyAnimAssetMenuBar");
    ezTranslateGizmoAction::MapActions("PropertyAnimAssetMenuBar");
  }

  // Tool Bar
  {
    ezActionMapManager::RegisterActionMap("PropertyAnimAssetToolBar").AssertSuccess();
    ezDocumentActions::MapToolbarActions("PropertyAnimAssetToolBar");
    ezCommandHistoryActions::MapActions("PropertyAnimAssetToolBar", "");
    ezAssetActions::MapToolBarActions("PropertyAnimAssetToolBar", true);
    ezGameObjectContextActions::MapToolbarActions("PropertyAnimAssetToolBar");
    ezGameObjectDocumentActions::MapToolbarActions("PropertyAnimAssetToolBar");
    ezTransformGizmoActions::MapToolbarActions("PropertyAnimAssetToolBar");
  }

  // View Tool Bar
  {
    ezActionMapManager::RegisterActionMap("PropertyAnimAssetViewToolBar").AssertSuccess();
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

static void ConfigureVisualScriptAsset()
{
  ezVisualScriptActions::RegisterActions();

  // Menu Bar
  {
    ezActionMapManager::RegisterActionMap("VisualScriptAssetMenuBar_Legacy").AssertSuccess();
    ezStandardMenus::MapActions("VisualScriptAssetMenuBar_Legacy", ezStandardMenuTypes::Default | ezStandardMenuTypes::Edit);
    ezProjectActions::MapActions("VisualScriptAssetMenuBar_Legacy");
    ezDocumentActions::MapMenuActions("VisualScriptAssetMenuBar_Legacy");
    ezAssetActions::MapMenuActions("VisualScriptAssetMenuBar_Legacy");
    ezCommandHistoryActions::MapActions("VisualScriptAssetMenuBar_Legacy");
    ezEditActions::MapActions("VisualScriptAssetMenuBar_Legacy", false, false);
  }

  // Tool Bar
  {
    ezActionMapManager::RegisterActionMap("VisualScriptAssetToolBar_Legacy").AssertSuccess();
    ezDocumentActions::MapToolbarActions("VisualScriptAssetToolBar_Legacy");
    ezCommandHistoryActions::MapActions("VisualScriptAssetToolBar_Legacy", "");
    ezAssetActions::MapToolBarActions("VisualScriptAssetToolBar_Legacy", true);
    ezVisualScriptActions::MapActions("VisualScriptAssetToolBar_Legacy");
  }
}

static void ConfigureDecalAsset()
{
  ezPropertyMetaState::GetSingleton()->m_Events.AddEventHandler(ezDecalAssetProperties::PropertyMetaStateEventHandler);

  // Menu Bar
  {
    ezActionMapManager::RegisterActionMap("DecalAssetMenuBar").AssertSuccess();
    ezStandardMenus::MapActions("DecalAssetMenuBar", ezStandardMenuTypes::Default | ezStandardMenuTypes::Edit);
    ezProjectActions::MapActions("DecalAssetMenuBar");
    ezDocumentActions::MapMenuActions("DecalAssetMenuBar");
    ezAssetActions::MapMenuActions("DecalAssetMenuBar");
    ezCommandHistoryActions::MapActions("DecalAssetMenuBar");
  }

  // Tool Bar
  {
    ezActionMapManager::RegisterActionMap("DecalAssetToolBar").AssertSuccess();
    ezDocumentActions::MapToolbarActions("DecalAssetToolBar");
    ezCommandHistoryActions::MapActions("DecalAssetToolBar", "");
    ezAssetActions::MapToolBarActions("DecalAssetToolBar", true);
  }

  // View Tool Bar
  {
    ezActionMapManager::RegisterActionMap("DecalAssetViewToolBar").AssertSuccess();
    ezViewActions::MapToolbarActions("DecalAssetViewToolBar", ezViewActions::RenderMode | ezViewActions::ActivateRemoteProcess);
    ezViewLightActions::MapToolbarActions("DecalAssetViewToolBar");
  }
}

static void ConfigureAnimationClipAsset()
{
  ezPropertyMetaState::GetSingleton()->m_Events.AddEventHandler(ezAnimationClipAssetProperties::PropertyMetaStateEventHandler);

  // Menu Bar
  {
    ezActionMapManager::RegisterActionMap("AnimationClipAssetMenuBar").AssertSuccess();
    ezStandardMenus::MapActions("AnimationClipAssetMenuBar", ezStandardMenuTypes::Default | ezStandardMenuTypes::Edit);
    ezProjectActions::MapActions("AnimationClipAssetMenuBar");
    ezDocumentActions::MapMenuActions("AnimationClipAssetMenuBar");
    ezAssetActions::MapMenuActions("AnimationClipAssetMenuBar");
    ezCommandHistoryActions::MapActions("AnimationClipAssetMenuBar");
  }

  // Tool Bar
  {
    ezActionMapManager::RegisterActionMap("AnimationClipAssetToolBar").AssertSuccess();
    ezDocumentActions::MapToolbarActions("AnimationClipAssetToolBar");
    ezCommandHistoryActions::MapActions("AnimationClipAssetToolBar", "");
    ezAssetActions::MapToolBarActions("AnimationClipAssetToolBar", true);
    ezCommonAssetActions::MapToolbarActions("AnimationClipAssetToolBar", ezCommonAssetUiState::Loop | ezCommonAssetUiState::Pause | ezCommonAssetUiState::Restart | ezCommonAssetUiState::SimulationSpeed | ezCommonAssetUiState::Grid);
  }

  // View Tool Bar
  {
    ezActionMapManager::RegisterActionMap("AnimationClipAssetViewToolBar").AssertSuccess();
    ezViewActions::MapToolbarActions("AnimationClipAssetViewToolBar", ezViewActions::RenderMode | ezViewActions::ActivateRemoteProcess);
    ezViewLightActions::MapToolbarActions("AnimationClipAssetViewToolBar");
  }
}

static void ConfigureSkeletonAsset()
{
  ezPropertyMetaState::GetSingleton()->m_Events.AddEventHandler(ezSkeletonAssetDocument::PropertyMetaStateEventHandler);

  ezSkeletonActions::RegisterActions();

  // Menu Bar
  {
    ezActionMapManager::RegisterActionMap("SkeletonAssetMenuBar").AssertSuccess();
    ezStandardMenus::MapActions("SkeletonAssetMenuBar", ezStandardMenuTypes::Default | ezStandardMenuTypes::Edit);
    ezProjectActions::MapActions("SkeletonAssetMenuBar");
    ezDocumentActions::MapMenuActions("SkeletonAssetMenuBar");
    ezAssetActions::MapMenuActions("SkeletonAssetMenuBar");
    ezCommandHistoryActions::MapActions("SkeletonAssetMenuBar");
  }

  // Tool Bar
  {
    ezActionMapManager::RegisterActionMap("SkeletonAssetToolBar").AssertSuccess();
    ezDocumentActions::MapToolbarActions("SkeletonAssetToolBar");
    ezCommandHistoryActions::MapActions("SkeletonAssetToolBar", "");
    ezAssetActions::MapToolBarActions("SkeletonAssetToolBar", true);
    ezCommonAssetActions::MapToolbarActions("SkeletonAssetToolBar", ezCommonAssetUiState::Grid);
    ezSkeletonActions::MapActions("SkeletonAssetToolBar");
  }

  // View Tool Bar
  {
    ezActionMapManager::RegisterActionMap("SkeletonAssetViewToolBar").AssertSuccess();
    ezViewActions::MapToolbarActions("SkeletonAssetViewToolBar", ezViewActions::RenderMode | ezViewActions::ActivateRemoteProcess);
    ezViewLightActions::MapToolbarActions("SkeletonAssetViewToolBar");
  }
}

static void ConfigureAnimatedMeshAsset()
{
  // Menu Bar
  {
    ezActionMapManager::RegisterActionMap("AnimatedMeshAssetMenuBar").AssertSuccess();
    ezStandardMenus::MapActions("AnimatedMeshAssetMenuBar", ezStandardMenuTypes::Default | ezStandardMenuTypes::Edit);
    ezProjectActions::MapActions("AnimatedMeshAssetMenuBar");
    ezDocumentActions::MapMenuActions("AnimatedMeshAssetMenuBar");
    ezAssetActions::MapMenuActions("AnimatedMeshAssetMenuBar");
    ezCommandHistoryActions::MapActions("AnimatedMeshAssetMenuBar");
  }

  // Tool Bar
  {
    ezActionMapManager::RegisterActionMap("AnimatedMeshAssetToolBar").AssertSuccess();
    ezDocumentActions::MapToolbarActions("AnimatedMeshAssetToolBar");
    ezCommandHistoryActions::MapActions("AnimatedMeshAssetToolBar", "");
    ezAssetActions::MapToolBarActions("AnimatedMeshAssetToolBar", true);
    ezCommonAssetActions::MapToolbarActions("AnimatedMeshAssetToolBar", ezCommonAssetUiState::Grid);
  }

  // View Tool Bar
  {
    ezActionMapManager::RegisterActionMap("AnimatedMeshAssetViewToolBar").AssertSuccess();
    ezViewActions::MapToolbarActions("AnimatedMeshAssetViewToolBar", ezViewActions::RenderMode | ezViewActions::ActivateRemoteProcess);
    ezViewLightActions::MapToolbarActions("AnimatedMeshAssetViewToolBar");
  }
}

static void ConfigureImageDataAsset()
{
  // Menu Bar
  {
    ezActionMapManager::RegisterActionMap("ImageDataAssetMenuBar").AssertSuccess();
    ezStandardMenus::MapActions("ImageDataAssetMenuBar", ezStandardMenuTypes::Default | ezStandardMenuTypes::Edit);
    ezProjectActions::MapActions("ImageDataAssetMenuBar");
    ezDocumentActions::MapMenuActions("ImageDataAssetMenuBar");
    ezAssetActions::MapMenuActions("ImageDataAssetMenuBar");
    ezCommandHistoryActions::MapActions("ImageDataAssetMenuBar");
  }

  // Tool Bar
  {
    ezActionMapManager::RegisterActionMap("ImageDataAssetToolBar").AssertSuccess();
    ezDocumentActions::MapToolbarActions("ImageDataAssetToolBar");
    ezCommandHistoryActions::MapActions("ImageDataAssetToolBar", "");
    ezAssetActions::MapToolBarActions("ImageDataAssetToolBar", true);
  }

  // View Tool Bar
  {
    ezActionMapManager::RegisterActionMap("ImageDataAssetViewToolBar").AssertSuccess();
    ezViewActions::MapToolbarActions("ImageDataAssetViewToolBar", ezViewActions::RenderMode | ezViewActions::ActivateRemoteProcess);
    ezViewLightActions::MapToolbarActions("ImageDataAssetViewToolBar");
  }
}

static void ConfigureStateMachineAsset()
{
  // Menu Bar
  {
    ezActionMapManager::RegisterActionMap("StateMachineAssetMenuBar").AssertSuccess();

    ezStandardMenus::MapActions("StateMachineAssetMenuBar", ezStandardMenuTypes::Default | ezStandardMenuTypes::Edit);
    ezProjectActions::MapActions("StateMachineAssetMenuBar");
    ezDocumentActions::MapMenuActions("StateMachineAssetMenuBar");
    ezAssetActions::MapMenuActions("StateMachineAssetMenuBar");
    ezCommandHistoryActions::MapActions("StateMachineAssetMenuBar");
    ezEditActions::MapActions("StateMachineAssetMenuBar", false, false);
  }

  // Tool Bar
  {
    ezActionMapManager::RegisterActionMap("StateMachineAssetToolBar").AssertSuccess();
    ezDocumentActions::MapToolbarActions("StateMachineAssetToolBar");
    ezCommandHistoryActions::MapActions("StateMachineAssetToolBar", "");
    ezAssetActions::MapToolBarActions("StateMachineAssetToolBar", true);
  }
}
static void ConfigureBlackboardTemplateAsset()
{
  // Menu Bar
  {
    ezActionMapManager::RegisterActionMap("BlackboardTemplateAssetMenuBar").AssertSuccess();

    ezStandardMenus::MapActions("BlackboardTemplateAssetMenuBar", ezStandardMenuTypes::Default | ezStandardMenuTypes::Edit);
    ezProjectActions::MapActions("BlackboardTemplateAssetMenuBar");
    ezDocumentActions::MapMenuActions("BlackboardTemplateAssetMenuBar");
    ezAssetActions::MapMenuActions("BlackboardTemplateAssetMenuBar");
    ezCommandHistoryActions::MapActions("BlackboardTemplateAssetMenuBar");
    ezEditActions::MapActions("BlackboardTemplateAssetMenuBar", false, false);
  }

  // Tool Bar
  {
    ezActionMapManager::RegisterActionMap("BlackboardTemplateAssetToolBar").AssertSuccess();
    ezDocumentActions::MapToolbarActions("BlackboardTemplateAssetToolBar");
    ezCommandHistoryActions::MapActions("BlackboardTemplateAssetToolBar", "");
    ezAssetActions::MapToolBarActions("BlackboardTemplateAssetToolBar", true);
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
  ConfigureVisualScriptAsset();
  ConfigureDecalAsset();
  ConfigureAnimationClipAsset();
  ConfigureSkeletonAsset();
  ConfigureAnimatedMeshAsset();
  ConfigureImageDataAsset();
  ConfigureStateMachineAsset();
  ConfigureBlackboardTemplateAsset();

  ezDocumentManager::s_CustomActions["CustomAction_CreateShaderFromTemplate"] = CustomAction_CreateShaderFromTemplate;
}

void OnUnloadPlugin()
{
  ezTextureAssetActions::UnregisterActions();
  ezLUTAssetActions::UnregisterActions();
  ezVisualShaderActions::UnregisterActions();
  ezVisualScriptActions::UnregisterActions();
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
