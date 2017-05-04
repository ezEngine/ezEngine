#pragma once

#include <GameEngine/Basics.h>
#include <GameEngine/Declarations.h>

#include <Foundation/Reflection/Reflection.h>
#include <RendererFoundation/Basics.h>
#include <RendererCore/Pipeline/Declarations.h>
#include <Core/Graphics/Camera.h>
#include <Core/ResourceManager/ResourceHandle.h>

class ezWindow;
class ezView;
typedef ezTypedResourceHandle<class ezRenderPipelineResource> ezRenderPipelineResourceHandle;

/// \brief ezGameState is the base class to build custom game logic upon. It works closely together with ezGameApplication.
///
/// In a typical game there is always exactly one instance of an ezGameState derived class active.
/// The game state handles custom game logic, which must be handled outside ezWorld, custom components and scripts.
///
/// For example a custom implementation of ezGameState may handle how to show a menu, when to switch to
/// another level, how multi-player works, or which player information is transitioned from one level to the next.
/// It's main purpose is to implement high-level game logic.
///
/// ezGameApplication will loop through all available ezGameState implementations and ask each available one
/// whether it can handle a certain level. Each game state returns a 'score' how well it can handle the game.
///
/// In a typical game you only have one game state linked into the binary, so in that case there is no reason for
/// such a system. However, in an editor you might have multiple game states available through plugins, but
/// only one can take control.
/// In such a case, each game state may inspect the given world and check whether it is e.g. a single-player
/// or multi-player level, or whether it uses it's own game specific components, and then decide whether
/// it is the best fit for that level.
///
/// \note Do not forget to reflect your derived class, otherwise ezGameApplication may not find it.
class EZ_GAMEENGINE_DLL ezGameState : public ezReflectedClass
{
  EZ_ADD_DYNAMIC_REFLECTION(ezGameState, ezReflectedClass)

protected:
  /// \brief This class cannot be instantiated directly.
  ezGameState() {}

public:
  virtual ~ezGameState() {}

  /// \brief When a game state was chosen, it gets activated through this function.
  ///
  /// \param pWorld
  /// The game state is supposed to operate on the given world.
  /// In a stand-alone application pWorld will always be nullptr and the game state is expected
  /// to create worlds itself.
  /// When run inside the editor, pWorld will already exist and the game state is expected to work on it.
  virtual void OnActivation(ezWorld* pWorld);

  /// \brief Called when the game state is being shut down.
  virtual void OnDeactivation();

  /// \brief Called once per game update. Should handle input updates here.
  virtual void ProcessInput() { }

  /// \brief Called once each frame before the worlds are updated.
  virtual void BeforeWorldUpdate() { }

  /// \brief Called once each frame after the worlds have been updated.
  virtual void AfterWorldUpdate() { }

  /// \brief Returns the ezGameApplication through which this state was created.
  EZ_FORCE_INLINE ezGameApplication* GetApplication() const { return m_pApplication; }

  /// \brief Called by ezGameApplication to determine which game state is best suited to handle a situation.
  ///
  /// The application type is passed along, such that game states that cannot run inside the editor can bail out (or vice versa).
  /// If the application already has a world that should be shown, the game state can inspect it.
  /// If the game state is expected to create a new world, pWorld will be nullptr.
  virtual float CanHandleThis(ezGameApplicationType AppType, ezWorld* pWorld) const = 0;

  /// \brief Has to call ezRenderLoop::AddMainView for all views that need to be rendered
  virtual void AddAllMainViews();

  /// \brief Call this to signal that a game state requested the application to quit.
  ///
  /// ezGameApplication will shut down when this happens. ezEditor will stop play-the-game mode when it is running.
  virtual void RequestQuit() { m_bStateWantsToQuit = true; }

  /// \brief Returns whether the game state wants to quit the application.
  bool WasQuitRequested() const { return m_bStateWantsToQuit; }

protected:
  /// \brief Creates a default window (ezGameStateWindow) adds it to the application and fills out m_pMainWindow and m_hMainSwapChain
  virtual void CreateMainWindow();

  /// \brief Destroys the m_pMainWindow.
  /// Unless overridden Deactivate() will call this.
  virtual void DestroyMainWindow();

  /// \brief Configures available input devices, e.g. sets mouse speed, cursor clipping, etc.
  /// Unless overridden Activate() will call this.
  virtual void ConfigureInputDevices();

  /// \brief Adds custom input actions, if necessary.
  /// Unless overridden Activate() will call this.
  virtual void ConfigureInputActions();

  /// \brief Creates a default render pipeline. Unless overridden, Activate() will do this for the main window.
  virtual void SetupMainView(ezGALRenderTargetViewHandle hBackBuffer);

  /// \brief Sets m_pMainWorld and updates m_pMainView to use that new world for rendering
  void ChangeMainWorld(ezWorld* pNewMainWorld);

  /// \brief Sets up m_MainCamera for first use
  virtual void ConfigureMainCamera();

  ezWindow* m_pMainWindow = nullptr;
  ezGALSwapChainHandle m_hMainSwapChain;
  ezViewHandle m_hMainView;

  ezWorld* m_pMainWorld = nullptr;

  ezCamera m_MainCamera;
  bool m_bStateWantsToQuit = false;

private:
  friend class ezGameApplication;
  ezGameApplication* m_pApplication = nullptr;
};
