#pragma once

#include <Core/CoreDLL.h>

#include <Foundation/Reflection/Reflection.h>

class ezWorld;

/// \brief ezGameStateBase is the base class for all game states. Game states are used to implement custom high level game logic.
///
/// See the online documentation for details: https://ezengine.net/pages/docs/runtime/application/game-state.html
///
/// Note that you would typically derive custom game states from ezGameState, not ezGameStateBase, since the
/// former provides much more functionality out of the box.
class EZ_CORE_DLL ezGameStateBase : public ezReflectedClass
{
  EZ_ADD_DYNAMIC_REFLECTION(ezGameStateBase, ezReflectedClass)

public:
  ezGameStateBase() = default;
  virtual ~ezGameStateBase() = default;

  /// \brief A game state gets activated through this function.
  ///
  /// \param pWorld
  /// The game state is supposed to operate on the given world.
  /// In a stand-alone application pWorld will always be nullptr and the game state is expected to create worlds itself.
  /// When run inside the editor, pWorld will already exist and the game state is expected to work on it.
  ///
  /// \param sStartPosition
  /// An optional string to identify where the player should spawn.
  /// This may, for instance, be the unique name of an object. It is up to the game state how the string is used, if at all.
  ///
  /// \param pStartPosition
  /// An optional transform for the 'player object' to start at.
  /// Usually nullptr, but may be set by the editor to relocate or create the player object at the given destination.
  virtual void OnActivation(ezWorld* pWorld, ezStringView sStartPosition, const ezTransform& startPositionOffset) = 0;

  /// \brief Called when the game state is being shut down.
  ///
  /// Override this to clean up or save data to disk.
  virtual void OnDeactivation() = 0;

  /// \brief Called once per game update, early in the frame. Should handle input updates here.
  virtual void ProcessInput() {}

  /// \brief Called once each frame before the worlds are updated.
  virtual void BeforeWorldUpdate() {}

  /// \brief Called once each frame after the worlds have been updated.
  virtual void AfterWorldUpdate() {}

  /// \brief Called once each frame to configure the main camera position and rotation.
  ///
  /// Note that ezCameraComponent may already apply set general options like field-of-view,
  /// so don't override these values, if you want to use that component.
  /// The default ezGameState implementation searches for an ezCameraComponent in the world that is set to "Main View"
  /// and uses it's transform for the main camera.
  virtual void ConfigureMainCamera() {}

  /// \brief Has to call ezRenderLoop::AddMainView for all views that need to be rendered.
  ///
  /// This will be called every frame by the editor, to ensure that only the relevant views are rendered,
  /// but during stand-alone game execution this may never be called.
  virtual void AddMainViewsToRender() = 0;

  /// \brief Call this to signal that a game state requested the application to quit.
  ///
  /// ezGameApplication will shut down when this happens. ezEditor will stop play-the-game mode when it is running.
  /// When calling this, pass a string to identify where the request comes from, e.g. "window" for when clicking
  /// the window close button, "game" when game logic (UI) decided to quite, etc.
  /// ezEditor will pass in "editor-esc" and "editor-force" when a game-state should be shut down due to
  /// the user pressing Escape or clicking the "stop" button.
  virtual void RequestQuit(ezStringView sRequestedBy) = 0;

  /// \brief Returns whether the game state wants to quit the application.
  ///
  /// ezGameApplication will shut down when this function returns true.
  /// Logic for whether to shut down should typically be handled in RequestQuit().
  virtual bool WasQuitRequested() const = 0;

  /// \brief Should be overridden by game states that are only meant as a fallback solution.
  ///
  /// See the implementation for ezFallbackGameState for details.
  virtual bool IsFallbackGameState() const { return false; }
};
