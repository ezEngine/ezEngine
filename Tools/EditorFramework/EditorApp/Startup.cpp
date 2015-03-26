#include <PCH.h>
#include <Foundation/Configuration/Startup.h>
#include <GuiFoundation/Action/ActionManager.h>
#include <GuiFoundation/Action/ActionMapManager.h>
#include <GuiFoundation/Action/BaseActions.h>
#include <EditorFramework/Actions/ProjectActions.h>

EZ_BEGIN_SUBSYSTEM_DECLARATION(EditorFramework, EditorFrameworkMain)

  BEGIN_SUBSYSTEM_DEPENDENCIES
    "GuiFoundation"
  END_SUBSYSTEM_DEPENDENCIES

  ON_CORE_STARTUP
  {
    ezProjectActions::RegisterActions();

    ezActionMapManager::RegisterActionMap("SettingsDocument");
    ezProjectActions::MapActions("SettingsDocument");
  }

  ON_CORE_SHUTDOWN
  {
    ezProjectActions::UnregisterActions();
  }

EZ_END_SUBSYSTEM_DECLARATION


