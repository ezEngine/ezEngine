#pragma once

#include <GameEngine/GameState/GameStateBase.h>

#include <Core/Graphics/Camera.h>
#include <Core/ResourceManager/ResourceHandle.h>
#include <Foundation/Reflection/Reflection.h>
#include <Foundation/Types/UniquePtr.h>
#include <RendererCore/Pipeline/Declarations.h>
#include <RendererFoundation/RendererFoundationDLL.h>

class ezWindow;
class ezWindowOutputTargetBase;
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
class EZ_GAMEENGINE_DLL ezGameState : public ezGameStateBase
{
  EZ_ADD_DYNAMIC_REFLECTION(ezGameState, ezGameStateBase)

protected:
  /// \brief This class cannot be instantiated directly.
  ezGameState();

public:
  virtual ~ezGameState();

  /// \brief When a game state was chosen, it gets activated through this function.
  ///
  /// \param pWorld
  /// The game state is supposed to operate on the given world.
  /// In a stand-alone application pWorld will always be nullptr and the game state is expected
  /// to create worlds itself.
  /// When run inside the editor, pWorld will already exist and the game state is expected to work on it.
  ///
  /// \param pStartPosition
  /// An optional transform for the 'player object' to start at.
  /// Usually nullptr, but may be set by the editor to relocate or create the player object at the given destination.
  virtual void OnActivation(ezWorld* pWorld, const ezTransform* pStartPosition) override;

  /// \brief Called when the game state is being shut down.
  virtual void OnDeactivation() override;

  /// \brief Has to call ezRenderLoop::AddMainView for all views that need to be rendered
  virtual void ScheduleRendering() override;

  /// \brief Gives access to the game state's main camera object.
  ezCamera* GetMainCamera() { return &m_MainCamera; }

  /// \brief Returns whether the application is running in full mixed reality mode.
  /// This is evaluated in OnActivation(), will always return false before that call.
  bool IsMixedRealityMode() const { return m_bMixedRealityMode; }

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

  /// \brief Creates a default render view. Unless overridden, Activate() will do this for the main window.
  virtual void SetupMainView(ezWindowOutputTargetBase* pOutputTarget);

  /// \brief Overrideable function that may create a player object.
  ///
  /// By default called by OnActivation().
  /// The default implementation will search the world for ezPlayerStartComponent's and instantiate the given player prefab at one of those
  /// locations. If pStartPosition is not nullptr, it will be used as the spawn position for the player prefab, otherwise the location of
  /// the ezPlayerStartComponent will be used.
  ///
  /// Returns EZ_SUCCESS if a prefab was spawned, EZ_FAILURE if nothing was done.
  virtual ezResult SpawnPlayer(const ezTransform* pStartPosition);

  /// \brief Creates a default main view with the given render pipeline.
  void SetupMainView(ezWindowOutputTargetBase* pOutputTarget, ezTypedResourceHandle<ezRenderPipelineResource> hRenderPipeline);

  /// \brief Sets m_pMainWorld and updates m_pMainView to use that new world for rendering
  void ChangeMainWorld(ezWorld* pNewMainWorld);

  /// \brief Sets up m_MainCamera for first use
  virtual void ConfigureMainCamera();

  ezWindow* m_pMainWindow = nullptr;
  ezWindowOutputTargetBase* m_pMainOutputTarget = nullptr;
  ezViewHandle m_hMainView;

  ezWorld* m_pMainWorld = nullptr;

  ezCamera m_MainCamera;
  bool m_bStateWantsToQuit = false;
  bool m_bMixedRealityMode = false;
  bool m_bVirtualRealityMode = false;

};

