#pragma once

#include <Foundation/Basics.h>
#include <GuiFoundation/Action/ActionMap.h>


class EZ_GUIFOUNDATION_DLL ezActionMapManager
{
public:
  static ezResult RegisterActionMap(const ezHashedString& sMapping);
  static ezResult UnregisterActionMap(const ezHashedString& sMapping);
  static ezActionMap* GetActionMap(const ezHashedString& sMapping);

private:
  EZ_MAKE_SUBSYSTEM_STARTUP_FRIEND(Core, ActionMapManager);

  static void Startup();
  static void Shutdown();

private:
  static ezMap<ezHashedString, ezActionMap*> s_Mappings;
};
