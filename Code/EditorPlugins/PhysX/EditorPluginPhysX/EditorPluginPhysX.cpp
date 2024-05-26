#include <EditorPluginPhysX/EditorPluginPhysXPCH.h>

#include <EditorFramework/Actions/AssetActions.h>
#include <EditorFramework/Actions/ProjectActions.h>
#include <GuiFoundation/Action/ActionMapManager.h>
#include <GuiFoundation/Action/CommandHistoryActions.h>
#include <GuiFoundation/Action/DocumentActions.h>
#include <GuiFoundation/Action/StandardMenus.h>

#include <EditorFramework/Actions/CommonAssetActions.h>
#include <EditorPluginPhysX/Actions/PhysXActions.h>
#include <EditorPluginPhysX/CollisionMeshAsset/CollisionMeshAssetObjects.h>
#include <GameEngine/Physics/CollisionFilter.h>
#include <GuiFoundation/PropertyGrid/PropertyMetaState.h>
#include <GuiFoundation/UIServices/DynamicEnums.h>

void UpdateCollisionLayerDynamicEnumValues();

static void ToolsProjectEventHandler(const ezToolsProjectEvent& e);

void OnLoadPlugin()
{
  ezToolsProject::GetSingleton()->s_Events.AddEventHandler(ToolsProjectEventHandler);

  // Collision Mesh
  {
    ezPropertyMetaState::GetSingleton()->m_Events.AddEventHandler(ezCollisionMeshAssetProperties::PropertyMetaStateEventHandler);

    // Menu Bar
    {
      ezActionMapManager::RegisterActionMap("PxCollisionMeshAssetMenuBar", "AssetMenuBar").IgnoreResult();
    }

    // Tool Bar
    {
      ezActionMapManager::RegisterActionMap("PxCollisionMeshAssetToolBar", "AssetToolbar").IgnoreResult();
      ezCommonAssetActions::MapToolbarActions("PxCollisionMeshAssetToolBar", ezCommonAssetUiState::Grid);
    }
  }

  // Scene
  {
    // Menu Bar
    {
      ezPhysXActions::RegisterActions();
      ezPhysXActions::MapMenuActions();
    }

    // Tool Bar
    {
    }
  }
}

void OnUnloadPlugin()
{
  ezPhysXActions::UnregisterActions();
  ezToolsProject::GetSingleton()->s_Events.RemoveEventHandler(ToolsProjectEventHandler);
  ezPropertyMetaState::GetSingleton()->m_Events.RemoveEventHandler(ezCollisionMeshAssetProperties::PropertyMetaStateEventHandler);
}

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

  ezCollisionFilterConfig cfg;
  if (cfg.Load().Failed())
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
