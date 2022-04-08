#include <EditorPluginJolt/EditorPluginJoltPCH.h>

#include <EditorFramework/Actions/AssetActions.h>
#include <EditorFramework/Actions/ProjectActions.h>
#include <GuiFoundation/Action/ActionMapManager.h>
#include <GuiFoundation/Action/CommandHistoryActions.h>
#include <GuiFoundation/Action/DocumentActions.h>
#include <GuiFoundation/Action/StandardMenus.h>

#include <EditorFramework/Actions/CommonAssetActions.h>
#include <EditorPluginJolt/Actions/JoltActions.h>
#include <EditorPluginJolt/CollisionMeshAsset/JoltCollisionMeshAssetObjects.h>
#include <GameEngine/Physics/CollisionFilter.h>
#include <GuiFoundation/PropertyGrid/PropertyMetaState.h>
#include <GuiFoundation/UIServices/DynamicEnums.h>

void UpdateCollisionLayerDynamicEnumValues();

static void ToolsProjectEventHandler(const ezToolsProjectEvent& e);

void OnLoadPlugin()
{
  ezQtEditorApp::GetSingleton()->AddRuntimePluginDependency("EditorPluginJolt", "ezJoltPlugin");
  ezQtEditorApp::GetSingleton()->AddRuntimePluginDependency("EditorPluginJolt", "ezEnginePluginJolt");

  ezToolsProject::GetSingleton()->s_Events.AddEventHandler(ToolsProjectEventHandler);

  // Collision Mesh
  {
    ezPropertyMetaState::GetSingleton()->m_Events.AddEventHandler(ezJoltCollisionMeshAssetProperties::PropertyMetaStateEventHandler);

    // Menu Bar
    {
      ezActionMapManager::RegisterActionMap("JoltCollisionMeshAssetMenuBar").IgnoreResult();
      ezStandardMenus::MapActions("JoltCollisionMeshAssetMenuBar", ezStandardMenuTypes::File | ezStandardMenuTypes::Edit | ezStandardMenuTypes::Panels | ezStandardMenuTypes::Help);
      ezProjectActions::MapActions("JoltCollisionMeshAssetMenuBar");
      ezDocumentActions::MapActions("JoltCollisionMeshAssetMenuBar", "Menu.File", false);
      ezCommandHistoryActions::MapActions("JoltCollisionMeshAssetMenuBar", "Menu.Edit");
    }

    // Tool Bar
    {
      ezActionMapManager::RegisterActionMap("JoltCollisionMeshAssetToolBar").IgnoreResult();
      ezDocumentActions::MapActions("JoltCollisionMeshAssetToolBar", "", true);
      ezCommandHistoryActions::MapActions("JoltCollisionMeshAssetToolBar", "");
      ezAssetActions::MapActions("JoltCollisionMeshAssetToolBar", true);
      ezCommonAssetActions::MapActions("JoltCollisionMeshAssetToolBar", "", ezCommonAssetUiState::Grid);
    }
  }

  // Scene
  {
    // Menu Bar
    {
      ezJoltActions::RegisterActions();
      ezJoltActions::MapMenuActions();
    }

    // Tool Bar
    {
    }
  }
}

void OnUnloadPlugin()
{
  ezJoltActions::UnregisterActions();
  ezToolsProject::GetSingleton()->s_Events.RemoveEventHandler(ToolsProjectEventHandler);
  ezPropertyMetaState::GetSingleton()->m_Events.RemoveEventHandler(ezJoltCollisionMeshAssetProperties::PropertyMetaStateEventHandler);
}

EZ_PLUGIN_DEPENDENCY(ezEditorPluginScene);

EZ_PLUGIN_ON_LOADED()
{
  OnLoadPlugin();
}

EZ_PLUGIN_ON_UNLOADED()
{
  OnUnloadPlugin();
}

void UpdateCollisionLayerDynamicEnumValues()
{
  auto& cfe = ezDynamicEnum::GetDynamicEnum("PhysicsCollisionLayer");
  cfe.Clear();

  ezStringBuilder sPath = ezToolsProject::GetSingleton()->GetProjectDirectory();
  sPath.AppendPath("RuntimeConfigs/CollisionLayers.cfg");

  ezCollisionFilterConfig cfg;
  if (cfg.Load(sPath).Failed())
    return;

  // add all names and values that are valid (non-empty)
  for (ezInt32 i = 0; i < 32; ++i)
  {
    if (!ezStringUtils::IsNullOrEmpty(cfg.GetGroupName(i)))
    {
      cfe.SetValueAndName(i, cfg.GetGroupName(i));
    }
  }
}

static void ToolsProjectEventHandler(const ezToolsProjectEvent& e)
{
  if (e.m_Type == ezToolsProjectEvent::Type::ProjectOpened)
  {
    UpdateCollisionLayerDynamicEnumValues();
  }
}
