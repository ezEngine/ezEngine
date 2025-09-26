#pragma once

#include <Core/GameState/GameStateBase.h>
#include <Core/Graphics/Camera.h>
#include <Core/ResourceManager/ResourceHandle.h>
#include <Core/System/WindowManager.h>
#include <Foundation/Math/Size.h>
#include <Foundation/Reflection/Reflection.h>
#include <Foundation/Types/UniquePtr.h>
#include <GameEngine/GameEngineDLL.h>
#include <GameEngine/Utils/SceneLoadUtil.h>
#include <RendererCore/Pipeline/Declarations.h>
#include <RendererFoundation/RendererFoundationDLL.h>

class ezWindow;
class ezWindowOutputTargetBase;
class ezView;
class ezWindowOutputTargetGAL;
class ezDummyXR;

using ezRenderPipelineResourceHandle = ezTypedResourceHandle<class ezRenderPipelineResource>;

/// \brief ezGameState implements the ezGameStateBase interface and adds several convenience features.
///
/// For an explanation what game states are, see the online documentation:
/// https://ezengine.net/pages/docs/runtime/application/game-state.html
///
/// The ezGameState adds some default functionality:
/// * Creation of a main window and render pipeline
/// * A main view handle
/// * A main camera object
/// * A main world that is currently active
/// * Background loading of scenes
/// * A separate world used as a loading screen
/// * automatic player prefab spawning if a ezPlayerStartPointComponent is part of the scene
/// * automatically applies the state of the "Main View" ezCameraComponent in the scene
/// * Many additional hooks to customize only specific parts, such as the window creation
///
/// Typically you would derive from ezGameState and then override functions like
/// `ProcessInput()` and `ConfigureMainCamera()`. Take a look at `ezFallbackGameState` for inspiration.
class EZ_GAMEENGINE_DLL ezGameState : public ezGameStateBase
{
  EZ_ADD_DYNAMIC_REFLECTION(ezGameState, ezGameStateBase)

protected:
  /// \brief This class cannot be instantiated directly.
  ezGameState();

public:
  virtual ~ezGameState();

  /// \brief Returns the active ezGameState. Only one ezGameState is allowed to exist.
  static ezGameState* GetActiveGameState();

  /// \brief Returns the ezWorld that is currently the active one.
  ezWorld* GetMainWorld() { return m_pMainWorld; }

  /// \brief Gives access to the game state's main camera object.
  ezCamera* GetMainCamera() { return &m_MainCamera; }

  /// \brief Returns the ezView that is currently the one used for rendering the main output.
  ezView* GetMainView();

  /// \brief Whether a scene is currently being loaded.
  bool IsLoadingSceneInBackground(float* out_pProgress = nullptr) const;

  /// \brief Whether the game state currently displays a loading screen. This usually implies that a scene is being loaded as well.
  bool IsInLoadingScreen() const;

  /// \brief Called upon game startup.
  ///
  /// Calls CreateActors() to create the game's main window and setup input devices.
  /// Calls ConfigureInputActions() to setup input actions.
  /// Finally switches to pWorld (if available) or starts loading the scene that GetStartupSceneFile() returns.
  ///
  /// Override any of the above functions to customize them.
  virtual void OnActivation(ezWorld* pWorld, ezStringView sStartPosition, const ezTransform& startPositionOffset) override;

  /// \brief Cleans up the main window before the game is shut down.
  virtual void OnDeactivation() override;

  /// \brief Makes sure m_hMainView gets rendered. Mainly needed by the editor.
  virtual void AddMainViewsToRender() override;

  /// \brief Simply stores that the game should stop.
  ///
  /// Override this to add more elaborate logic, if necessary.
  virtual void RequestQuit(ezStringView sRequestedBy) override;

  /// \brief Whether RequestQuit() was called before.
  virtual bool WasQuitRequested() const override;

  /// \brief The ezGameState doesn't implement any input logic, but it forwards to UpdateBackgroundSceneLoading().
  virtual void ProcessInput() override;

  /// \brief Immediately switches to a loading screen and starts loading a level.
  ///
  /// When the level is finished loading, `OnBackgroundSceneLoadingFinished()` is called, which switches to it immediately, unless overridden.
  /// If the scene was already fully preloaded, the switch happens immediately, without showing a loading screen.
  void LoadScene(ezStringView sSceneFile, ezStringView sPreloadCollection, ezStringView sStartPosition, const ezTransform& startPositionOffset);

  /// \brief Convenience function to switch to a loading screen.
  ///
  /// Nothing actually gets loaded. Without further logic, the app will stay in the loading screen indefinitely.
  /// sTargetSceneFile is only passed in, so that the loading screen can be customized accordingly,
  /// for example it may show a screenshot of the target scene.
  void SwitchToLoadingScreen(ezStringView sTargetSceneFile);

  /// \brief Sets m_pMainWorld and updates m_pMainView to use that new world for rendering
  ///
  /// Calls OnChangedMainWorld() afterwards, so that you can follow up on a scene change as needed.
  /// Calls ConfigureMainCamera() as well.
  void ChangeMainWorld(ezWorld* pNewMainWorld, ezStringView sStartPosition, const ezTransform& startPositionOffset);

  /// \brief Starts loading a scene in the background.
  ///
  /// If available, a collection can be provided. Resources referenced in the collection will be fully preloaded first and then
  /// the scene is loaded. This is the only way to get a proper estimation of loading progress and is necessary to get a smooth
  /// start, otherwise the engine will have to load resources on-demand, many of which will be needed during the first frame.
  ///
  /// Once finished, one of these hooks is executed:
  ///   `OnBackgroundSceneLoadingFinished()`
  ///   `OnBackgroundSceneLoadingFailed()`
  ///   `OnBackgroundSceneLoadingCanceled()`
  void StartBackgroundSceneLoading(ezStringView sSceneFile, ezStringView sPreloadCollection);

  /// \brief If a scene is currently being loaded in the background, cancel the loading.
  ///
  /// Calls `OnBackgroundSceneLoadingCanceled()` if a scene was loading.
  void CancelBackgroundSceneLoading();

protected:
  /// \brief Creates an actor with a default window (ezGameStateWindow) adds it to the application
  ///
  /// The base implementation calls CreateMainWindow(), CreateMainOutputTarget() and SetupMainView() to configure the main window.
  virtual void CreateWindows();

  /// \brief Adds custom input actions, if necessary.
  /// Unless overridden OnActivation() will call this.
  virtual void ConfigureInputActions();

  /// \brief Overrideable function that may create a player object.
  ///
  /// By default called by OnChangedMainWorld() when switching to a non-loading screen world.
  /// The default implementation will search the world for ezPlayerStartComponent's and instantiate the given player prefab at one of those
  /// locations. If pStartPosition is not nullptr, it will be used as the spawn position for the player prefab, otherwise the location of
  /// the ezPlayerStartComponent will be used.
  ///
  /// sStartPosition allows to spawn the player at another named location, but there is no default implementation for such logic.
  ///
  /// Returns EZ_SUCCESS if a prefab was spawned, EZ_FAILURE if nothing was done.
  virtual ezResult SpawnPlayer(ezStringView sStartPosition, const ezTransform& startPositionOffset);

  /// \brief Creates an XR Actor, if XR is configured and available for the project.
  ezRegisteredWndHandle CreateXRWindow();

  /// \brief Creates a default main view.
  ezView* CreateMainView();

  /// \brief Executed when ChangeMainWorld() is used to switch to a new world.
  ///
  /// Override this to be informed about scene changes.
  /// This happens right at startup (both for given worlds and custom created ones)
  /// and when the game needs to switch to a new level.
  virtual void OnChangedMainWorld(ezWorld* pPrevWorld, ezWorld* pNewWorld, ezStringView sStartPosition, const ezTransform& startPositionOffset);

  /// \brief Searches for a "Main View" ezCameraComponent in the world and uses that for the camera position, if available.
  ///
  /// Override this for custom camera logic.
  virtual void ConfigureMainCamera() override;

  /// \brief Override this to modify the default window creation behavior. Called by CreateActors().
  virtual ezUniquePtr<ezWindow> CreateMainWindow();

  /// \brief Override this to modify the default output target creation behavior. Called by CreateActors().
  virtual ezUniquePtr<ezWindowOutputTargetGAL> CreateMainOutputTarget(ezWindow* pMainWindow);

  /// \brief Creates a default render view. Unless overridden, OnActivation() will do this for the main window.
  virtual void SetupMainView(ezGALSwapChainHandle hSwapChain, ezSizeU32 viewportSize);

  /// \brief Configures available input devices, e.g. sets mouse speed, cursor clipping, etc.
  /// Called by CreateActors() with the result of CreateMainWindow().
  virtual void ConfigureMainWindowInputDevices(ezWindow* pWindow);

  /// \brief Returns the path to the scene file to load at startup.
  ///
  /// By default this is taken from the command line '-scene' option.
  /// Override this function to define a custom startup scene (e.g. for the main menu) or load a saved state.
  virtual ezString GetStartupSceneFile();

  /// \brief Called by SwitchToLoadingScreen() to set up a new loading screen world.
  ///
  /// A loading screen uses a separate ezWorld. It can be fully set up in code or loaded from disk,
  /// but it should be very light-weight, so that it is quick to set up.
  virtual ezUniquePtr<ezWorld> CreateLoadingScreenWorld(ezStringView sTargetSceneFile);

  /// \brief If a scene is being loaded in the background, this advanced the loading.
  ///
  /// Upon success or failure, executes either of these:
  ///   `OnBackgroundSceneLoadingFinished()`
  ///   `OnBackgroundSceneLoadingFailed()`
  void UpdateBackgroundSceneLoading();

  /// \brief Called by `UpdateBackgroundSceneLoading()` when a scene is finished loading.
  ///
  /// May switch to the scene immediately or wait, for example for a user to confirm.
  virtual void OnBackgroundSceneLoadingFinished(ezUniquePtr<ezWorld>&& pWorld);

  /// \brief Called by `UpdateBackgroundSceneLoading()` when a scene failed to load.
  virtual void OnBackgroundSceneLoadingFailed(ezStringView sReason);

  /// \brief Called by `CancelBackgroundSceneLoading()` when scene loading gets canceled.
  virtual void OnBackgroundSceneLoadingCanceled();

  static ezGameState* s_pActiveGameState;

  ezViewHandle m_hMainView;

  ezWorld* m_pMainWorld = nullptr;

  ezCamera m_MainCamera;
  ezUniquePtr<ezDummyXR> m_pDummyXR;
  bool m_bStateWantsToQuit = false;
  bool m_bXREnabled = false;
  bool m_bXRRemotingEnabled = false;

  bool m_bTransitionWhenReady = false;
  ezUniquePtr<ezSceneLoadUtility> m_pBackgroundSceneLoad;
  ezUniquePtr<ezWorld> m_pLoadingScreenWorld;
  ezUniquePtr<ezWorld> m_pLoadedWorld;

  ezString m_sTargetSceneSpawnPoint;
  ezTransform m_TargetSceneSpawnOffset = ezTransform::MakeIdentity();
};
