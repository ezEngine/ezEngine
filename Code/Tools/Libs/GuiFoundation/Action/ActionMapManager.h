#pragma once

#include <Foundation/Basics.h>
#include <GuiFoundation/Action/ActionMap.h>


class EZ_GUIFOUNDATION_DLL ezActionMapManager
{
public:
  static ezResult RegisterActionMap(const char* szMapping);
  static ezResult UnregisterActionMap(const char* szMapping);
  static ezActionMap* GetActionMap(const char* szMapping);

private:
  EZ_MAKE_SUBSYSTEM_STARTUP_FRIEND(GuiFoundation, ActionMapManager);

  static void Startup();
  static void Shutdown();

private:
  static ezMap<ezString, ezActionMap*> s_Mappings;
};
