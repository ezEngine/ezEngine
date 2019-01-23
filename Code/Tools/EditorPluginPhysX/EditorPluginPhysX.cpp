#include <PCH.h>

#include <EditorFramework/Actions/AssetActions.h>
#include <EditorFramework/Actions/ProjectActions.h>
#include <EditorFramework/EditorApp/EditorApp.moc.h>
#include <Foundation/Reflection/Reflection.h>
#include <Foundation/Strings/TranslationLookup.h>
#include <GuiFoundation/Action/ActionMapManager.h>
#include <GuiFoundation/Action/CommandHistoryActions.h>
#include <GuiFoundation/Action/DocumentActions.h>
#include <GuiFoundation/Action/StandardMenus.h>
#include <ToolsFoundation/Reflection/ToolsReflectionUtils.h>

#include <EditorPluginPhysX/Actions/PhysXActions.h>
#include <EditorPluginPhysX/CollisionMeshAsset/CollisionMeshAssetObjects.h>
#include <GameEngine/CollisionFilter/CollisionFilter.h>
#include <GuiFoundation/PropertyGrid/PropertyMetaState.h>
#include <GuiFoundation/UIServices/DynamicEnums.h>
#include <PhysXCooking/PhysXCooking.h>

void UpdateCollisionLayerDynamicEnumValues();

static void ToolsProjectEventHandler(const ezToolsProjectEvent& e);

void OnLoadPlugin(bool bReloading)
{
  ezQtEditorApp::GetSingleton()->AddRuntimePluginDependency("EditorPluginPhysX", "ezPhysXPlugin");
  ezQtEditorApp::GetSingleton()->AddRuntimePluginDependency("EditorPluginPhysX", "ezEnginePluginPhysX");


  ezToolsProject::GetSingleton()->s_Events.AddEventHandler(ToolsProjectEventHandler);

  // Collision Mesh Asset
  {
    ezPropertyMetaState::GetSingleton()->m_Events.AddEventHandler(ezCollisionMeshAssetProperties::PropertyMetaStateEventHandler);

    // Menu Bar
    {
      ezActionMapManager::RegisterActionMap("CollisionMeshAssetMenuBar");
      ezProjectActions::MapActions("CollisionMeshAssetMenuBar");
      ezStandardMenus::MapActions("CollisionMeshAssetMenuBar", ezStandardMenuTypes::File | ezStandardMenuTypes::Edit |
                                                                   ezStandardMenuTypes::Panels | ezStandardMenuTypes::Help);
      ezDocumentActions::MapActions("CollisionMeshAssetMenuBar", "Menu.File", false);
      ezCommandHistoryActions::MapActions("CollisionMeshAssetMenuBar", "Menu.Edit");
    }

    // Tool Bar
    {
      ezActionMapManager::RegisterActionMap("CollisionMeshAssetToolBar");
      ezDocumentActions::MapActions("CollisionMeshAssetToolBar", "", true);
      ezCommandHistoryActions::MapActions("CollisionMeshAssetToolBar", "");
      ezAssetActions::MapActions("CollisionMeshAssetToolBar", true);
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

void OnUnloadPlugin(bool bReloading)
{
  ezPhysXActions::UnregisterActions();
  ezToolsProject::GetSingleton()->s_Events.RemoveEventHandler(ToolsProjectEventHandler);
}

ezPlugin g_Plugin(false, OnLoadPlugin, OnUnloadPlugin, "ezEditorPluginScene", "ezPhysXPlugin");

EZ_DYNAMIC_PLUGIN_IMPLEMENTATION(EZ_EDITORPLUGINPHYSX_DLL, ezEditorPluginPhysX);



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
