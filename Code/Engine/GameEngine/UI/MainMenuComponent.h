#pragma once

#include <Core/World/Component.h>
#include <Core/World/World.h>
#include <GameEngine/GameEngineDLL.h>

/// Interface for an in-game main menu, so that a game state can open one without depending on a UI plugin.
///
/// The implementation is provided by a UI plugin, see ezRmlUiMainMenuComponent.
/// ezFallbackGameState opens the menu when the user presses ESC, instead of quitting.
class EZ_GAMEENGINE_DLL ezMainMenuComponent : public ezComponent
{
  EZ_DECLARE_ABSTRACT_COMPONENT_TYPE(ezMainMenuComponent, ezComponent);

public:
  /// Does nothing if the menu is already open. The menu closes itself, it can't be closed from the outside.
  virtual void OpenMenu() = 0;

  virtual bool IsMenuOpen() const = 0;

  /// Returns the first main menu component in the world, or an invalid handle.
  ///
  /// This is a linear search over all game objects, so the result should be cached.
  static ezComponentHandle FindInWorld(ezWorld& ref_world);
};
