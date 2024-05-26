#pragma once

#include <Foundation/Basics.h>
#include <GuiFoundation/Action/ActionMap.h>

/// \brief A central place for creating and retrieving action maps.
class EZ_GUIFOUNDATION_DLL ezActionMapManager
{
public:
  /// \brief Adds a new action map with the given name. Returns EZ_FAILURE if the name was already used before.
  static ezResult RegisterActionMap(ezStringView sMapping, ezStringView sParentMapping = {});

  /// \brief Deletes the action map with the given name. Returns EZ_FAILURE, if no such map exists.
  static ezResult UnregisterActionMap(ezStringView sMapping);

  /// \brief Returns the action map with the given name, or nullptr, if it doesn't exist.
  static ezActionMap* GetActionMap(ezStringView sMapping);

private:
  EZ_MAKE_SUBSYSTEM_STARTUP_FRIEND(GuiFoundation, ActionMapManager);

  static void Startup();
  static void Shutdown();

private:
  static ezMap<ezString, ezActionMap*> s_Mappings;
};
