#pragma once

#include <Foundation/Basics.h>
#include <GuiFoundation/Action/ActionMap.h>

/// \brief A central place for creating and retrieving action maps.
class EZ_GUIFOUNDATION_DLL ezActionMapManager
{
public:
  /// \brief Adds a new action map with the given name and an optional inherited parent mapping. Asserts if the name was already used before.
  /// \param sMapping Name of the new mapping. This string has to be used when adding actions to the map.
  /// \param sParentMapping If set, the new mapping will inherit all actions of this mapping. The name must exist and resolve to a valid mapping once ezActionMap::BuildActionTree is called to generate the action tree.
  static void RegisterActionMap(ezStringView sActionMapName, ezStringView sParentActionMapName = {});

  /// \brief Deletes the action map with the given name. Asserts, if no such map exists.
  static void UnregisterActionMap(ezStringView sActionMapName);

  /// \brief Returns the action map with the given name, or nullptr, if it doesn't exist.
  static ezActionMap* GetActionMap(ezStringView sActionMapName);

private:
  EZ_MAKE_SUBSYSTEM_STARTUP_FRIEND(GuiFoundation, ActionMapManager);

  static void Startup();
  static void Shutdown();

private:
  static ezMap<ezString, ezActionMap*> s_Mappings;
};
