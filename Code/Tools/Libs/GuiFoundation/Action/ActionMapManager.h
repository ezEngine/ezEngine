#pragma once

#include <Foundation/Basics.h>
#include <GuiFoundation/Action/ActionMap.h>


class EZ_GUIFOUNDATION_DLL ezActionMapManager
{
public:
  static ezResult RegisterActionMap(ezStringView sMapping);
  static ezResult UnregisterActionMap(ezStringView sMapping);
  static ezActionMap* GetActionMap(ezStringView sMapping);

private:
  EZ_MAKE_SUBSYSTEM_STARTUP_FRIEND(GuiFoundation, ActionMapManager);

  static void Startup();
  static void Shutdown();

private:
  static ezMap<ezString, ezActionMap*> s_Mappings;
};
