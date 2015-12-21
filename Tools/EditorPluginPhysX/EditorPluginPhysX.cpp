#include <PCH.h>
#include <EditorPluginPhysX/EditorPluginPhysX.h>
#include <ToolsFoundation/Reflection/ToolsReflectionUtils.h>
#include <Foundation/Reflection/Reflection.h>
#include <EditorFramework/EditorApp/EditorApp.moc.h>
#include <GuiFoundation/Action/ActionMapManager.h>
#include <EditorFramework/Actions/ProjectActions.h>
#include <EditorFramework/Actions/AssetActions.h>
#include <GuiFoundation/Action/StandardMenus.h>
#include <GuiFoundation/Action/DocumentActions.h>
#include <GuiFoundation/Action/CommandHistoryActions.h>
#include <CoreUtils/Localization/TranslationLookup.h>

#include <PhysXCooking/PhysXCooking.h>
#include <EditorPluginPhysX/Actions/PhysXActions.h>
#include <GuiFoundation/UIServices/DynamicEnums.h>
#include <GameUtils/CollisionFilter/CollisionFilter.h>

void UpdateCollisionLayerDynamicEnumValues();

static void ToolsProjectEventHandler(const ezToolsProject::Event& e);

void OnLoadPlugin(bool bReloading)
{
  ezQtEditorApp::GetInstance()->AddRuntimePluginDependency("EditorPluginPhysX", "ezPhysXPlugin");

  ezQtEditorApp::GetInstance()->RegisterPluginNameForSettings("EditorPluginPhysX");
  ezTranslatorFromFiles::AddTranslationFile("PhysXPlugin.txt");
  ezToolsProject::GetInstance()->s_Events.AddEventHandler(ToolsProjectEventHandler);

  // Mesh Asset
  {
    // Menu Bar
    {
      ezActionMapManager::RegisterActionMap("CollisionMeshAssetMenuBar");
      ezProjectActions::MapActions("CollisionMeshAssetMenuBar");
      ezStandardMenus::MapActions("CollisionMeshAssetMenuBar", ezStandardMenuTypes::File | ezStandardMenuTypes::Edit | ezStandardMenuTypes::Panels | ezStandardMenuTypes::Help);
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
  ezToolsProject::GetInstance()->s_Events.RemoveEventHandler(ToolsProjectEventHandler);
}

ezPlugin g_Plugin(false, OnLoadPlugin, OnUnloadPlugin, "ezEditorPluginScene", "ezPhysXPlugin");

EZ_DYNAMIC_PLUGIN_IMPLEMENTATION(EZ_EDITORPLUGINPHYSX_DLL, ezEditorPluginPhysX);



void UpdateCollisionLayerDynamicEnumValues()
{
  auto& cfe = ezDynamicEnum::GetDynamicEnum("PhysicsCollisionLayer");
  cfe.Clear();

  ezStringBuilder sPath = ezApplicationConfig::GetProjectDirectory();
  sPath.AppendPath("Physics/CollisionLayers.cfg");

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

static void ToolsProjectEventHandler(const ezToolsProject::Event& e)
{
  if (e.m_Type == ezToolsProject::Event::Type::ProjectOpened)
  {
    UpdateCollisionLayerDynamicEnumValues();
  }
}