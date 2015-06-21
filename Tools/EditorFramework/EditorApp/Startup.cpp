#include <PCH.h>
#include <Foundation/Configuration/Startup.h>
#include <GuiFoundation/Action/ActionManager.h>
#include <GuiFoundation/Action/ActionMapManager.h>
#include <GuiFoundation/Action/BaseActions.h>
#include <EditorFramework/Actions/ProjectActions.h>
#include <EditorFramework/Actions/AssetActions.h>
#include <GuiFoundation/Action/StandardMenus.h>

EZ_BEGIN_SUBSYSTEM_DECLARATION(EditorFramework, EditorFrameworkMain)

  BEGIN_SUBSYSTEM_DEPENDENCIES
    "GuiFoundation"
  END_SUBSYSTEM_DEPENDENCIES

  ON_CORE_STARTUP
  {
    ezToolsReflectionUtils::RegisterType(ezRTTI::FindTypeByName("ezAssetDocumentInfo"));

    ezProjectActions::RegisterActions();
    ezAssetActions::RegisterActions();

    ezActionMapManager::RegisterActionMap("SettingsTabMenuBar");
    ezProjectActions::MapActions("SettingsTabMenuBar");
    ezStandardMenus::MapActions("SettingsTabMenuBar", ezStandardMenuTypes::Panels);
    
    ezActionMapManager::RegisterActionMap("AssetBrowserToolBar");
    ezAssetActions::MapActions("AssetBrowserToolBar", false);
  }

  ON_CORE_SHUTDOWN
  {
    ezProjectActions::UnregisterActions();
  }

EZ_END_SUBSYSTEM_DECLARATION


