#pragma once

#include <GameFoundation/Basics.h>
#include <GameFoundation/GameState/GameState.h>

#include <Foundation/Threading/DelegateTask.h>
#include <Foundation/Types/UniquePtr.h>

#include <Core/Application/Application.h>

#include <RendererFoundation/Device/SwapChain.h>

class ezDefaultTimeStepSmoothing;
class ezConsole;

/// \brief The base class for all typical game applications made with ezEngine
///
/// While ezApplication is an abstraction for the operating system entry point,
/// ezGameApplication extends this to implement startup and tear down functionality
/// of a typical game that uses the standard functionality of ezEngine.
///
/// ezGameApplication implements a lot of functionality needed by most games,
/// such as setting up data directories, loading plugins, configuring the input system, etc.
///
/// For every such step a virtual function is called, allowing to override steps in custom applications.
///
/// The default implementation tries to do as much of this in a data-driven way. E.g. plugin and data
/// directory configurations are read from JSON files. These can be configured by hand or using ezEditor.
///
/// You are NOT supposed to implement game functionality by deriving from ezGameApplication.
/// Instead see ezGameState.
///
/// ezGameApplication will create exactly one ezGameState by looping over all available ezGameState types
/// (through reflection) and picking the one whose ezGameState::CanHandleThis() function returns the highest score.
/// That game state will live throughout the entire application life-time and will be stepped every frame.
class EZ_GAMEFOUNDATION_DLL ezGameApplication : public ezApplication
{
public:

  /// szProjectPath may be nullptr, if FindProjectDirectory() is overridden.
  ezGameApplication(const char* szAppName, ezGameApplicationType type, const char* szProjectPath);
  ~ezGameApplication();

  /// \brief Returns the app name that was given to the constructor.
  const ezString GetAppName() const { return m_sAppName; }

  /// \brief Returns the ezGameApplication singleton
  static ezGameApplication* GetGameApplicationInstance()
  {
    return s_pGameApplicationInstance;
  }

  /// \brief Overrides ezApplication::Run() and implements a typical game update.
  ///
  /// Calls ezWindowBase::ProcessWindowMessages() on all windows that have been added through AddWindow()
  /// As long as there are any main views added to ezRenderLoop it \n
  ///   Updates the global ezClock. \n
  ///   Calls UpdateInput() \n
  ///   Calls UpdateWorldsAndRender() \n
  virtual ezApplication::ApplicationExecution Run() override;

  /// \brief Adds a top level window to the application.
  /// 
  /// An ezGALSwapChain is created for that window. Run() will call ezWindowBase::ProcessWindowMessages()
  /// on all windows that have been added.
  /// Most applications should add exactly one such window to the game application.
  /// Only few applications will add zero or multiple windows.
  ezGALSwapChainHandle AddWindow(ezWindowBase* pWindow);

  /// \brief Removes a previously added window. Destroys its swapchain. Should be called at application shutdown.
  void RemoveWindow(ezWindowBase* pWindow);

  /// \brief Returns the swapchain for the given window. The window must have been added via AddWindow()
  ezGALSwapChainHandle GetSwapChain(const ezWindowBase* pWindow) const;

  /// \brief Activates only the game state that is linked to the given ezWorld.
  /// Not needed in a typical application. Used by the editor for selective game state handling in play-the-game mode.
  void ActivateGameStateForWorld(ezWorld* pWorld);

  /// \brief Deactivates only the game state that is linked to the given ezWorld.
  /// Not needed in a typical application. Used by the editor for selective game state handling in play-the-game mode.
  void DeactivateGameStateForWorld(ezWorld* pWorld);

  /// \brief Shuts down all known game states. Automatically called by ezGameApplication::BeforeCoreShutdown()
  void DestroyAllGameStates();

  /// \brief Activates all known game states. Automatically called by ezGameApplication::AfterCoreStartup()
  void ActivateAllGameStates();

  /// \brief Deactivates all known game states. Automatically called by ezGameApplication::BeforeCoreShutdown()
  void DeactivateAllGameStates();

  /// \brief Calling this function requests that the application quits after the current invocation of Run() finishes.
  ///
  /// Can be overridden to prevent quitting under certain conditions.
  virtual void RequestQuit();

  /// \brief Returns whether RequestQuit() was called.
  EZ_FORCE_INLINE bool WasQuitRequested() const { return m_bWasQuitRequested; }

  /// \brief Checks all parent directories of the scene file and tries to find a file called 
  /// 'ezProject' (no extension) which marks the project directory.
  /// Returns an empty string, if no such directory could be found.
  ezString FindProjectDirectoryForScene(const char* szScene) const;

  /// \brief Tries to find the project directory by concatenating the start directory and the relative path
  /// to the project file. As long as the file is not found, the next parent of the start directory is tried.
  /// Returns an empty string, if no such directory could be found.
  ezString SearchProjectDirectory(const char* szStartDirectory, const char* szRelPathToProjectFile) const;

  /// \brief virtual function that is called by DoProjectSetup(). The result is passed to ezApplicationConfig::SetProjectDirectory
  ///
  /// The default implementation relies on a valid path in m_sAppProjectPath.
  /// It passes that to SearchProjectDirectory() together with the path to the application binary,
  /// to search for a project somewhere relative to where the application is installed.
  ///
  /// Override this, if your application uses a different folder structure or way to specify the project directory.
  virtual ezString FindProjectDirectory() const;

  /// \brief Returns what was passed to the constructor.
  ezGameApplicationType GetAppType() const { return m_AppType; }

  /// \brief Creates a new world with the given name.
  ///
  /// The world is added to the array of known worlds and as such is stepped during Run().
  /// If \a bCreateWorldModules is set, an instance of every class derived from ezWorldModule
  /// is created and linked with the new ezWorld.
  /// These modules will also be stepped when the world is updated.
  ezWorld* CreateWorld(const char* szWorldName, bool bCreateWorldModules);

  /// \brief Cleanes up all data related to a world that was created through CreateWorld().
  void DestroyWorld(ezWorld* pWorld);

  /// \brief Creates a new ezGameState for the given world. Automatically called by ezGameApplication::AfterCoreStartup().
  void CreateGameStateForWorld(ezWorld* pWorld);

  /// \brief Destroys the game state for the given world. Typically not necessary to call this directly.
  void DestroyGameStateForWorld(ezWorld* pWorld);

  /// \brief Returns the ezGameState associated with the given world.
  ezGameState* GetGameStateForWorld(ezWorld* pWorld) const;

  /// \brief Calls ezWorldModule::Update() on all modules associated with this world.
  void UpdateWorldModulesBefore(ezWorld* pWorld);
  void UpdateWorldModulesAfter(ezWorld* pWorld);

  /// \brief Calls ezWorldModule::Reinit() on all modules associated with this world.
  void ReinitWorldModules(ezWorld* pWorld);

  /// \brief Used at runtime (by the editor) to reload input maps. Forwards to DoConfigureInput()
  void ReinitializeInputConfig();

protected:

  /// \brief Calls Update on all worlds and renders all views through ezRenderLoop::Render()
  void UpdateWorldsAndRender();

  /// \brief Implements all the application startup
  ///
  /// Calls DoProjectSetup() to configure everything about the project.
  /// Calls ezStartup::StartupEngine()
  /// For stand-alone applications CreateGameStateForWorld() is called with a nullptr world.
  virtual void AfterCoreStartup() override;

  /// Destroys all game states and shuts down everything that was created in AfterCoreStartup()
  virtual void BeforeCoreShutdown() override;

  

protected:
  ///
  /// Project Initialization
  ///

  /// \brief This is the main setup function. It calls various other functions in a specific order to initialize the application.
  virtual void DoProjectSetup();

  /// \brief Called by DoProjectSetup() very early to configure where all log output shall go.
  virtual void DoSetupLogWriters();

  /// \brief Called by DoProjectSetup() early on to configure the ezFileSystem. 
  ///
  /// It's main responsibility is to call ezFileSystem::RegisterDataDirectoryFactory for all data
  /// directory types that the application should use. For configuring custom data directories,
  /// prefer to use DoSetupDataDirectories()
  virtual void DoConfigureFileSystem();

  /// \brief Called by DoProjectSetup() after DoConfigureFileSystem(). Sets up everything needed to use assets.
  virtual void DoConfigureAssetManagement();

  /// \brief Called by DoProjectSetup() after DoConfigureAssetManagement(). Adds additional data directories.
  /// The default implementation reads the data directory configuration from a JSON file in the project folder.
  virtual void DoSetupDataDirectories();

  /// \brief Called by DoProjectSetup() after DoSetupDataDirectories(). Loads plugins that the application should always load.
  /// The default implementation loads 'ezInspectorPlugin' in development builds 
  virtual void DoLoadCustomPlugins();

  /// \brief Called by DoProjectSetup() after DoLoadCustomPlugins().
  /// The default implementation uses ezApplicationPluginConfig to load all manual plugins.
  virtual void DoLoadPluginsFromConfig();
  
  /// \brief Called by DoProjectSetup() after DoLoadPluginsFromConfig().
  /// The default implementation sets up some common fallbacks.
  virtual void DoSetupDefaultResources();

  /// \brief Called by DoProjectSetup() after DoSetupDefaultResources().
  /// Creates a GAL device for rendering.
  virtual void DoSetupGraphicsDevice();

  /// \brief Called by DoProjectSetup() after DoSetupGraphicsDevice().
  /// The default implementation uses ezGameAppInputConfig to read the input configuration from a JSON file in the project folder.
  /// Additionally it configures ESC, F5 and F8 to be 'GameApp::CloseApp', 'GameApp::ReloadResources' and 'GameApp::CaptureProfiling' respectively.
  virtual void DoConfigureInput(bool bReinitialize);

  /// \brief Called by DoProjectSetup() after DoConfigureInput().
  /// The default implementation loads the "Tags.ezManifest" file from the project directory.
  virtual void DoLoadTags();

  ///
  /// Project Shutdown
  ///

  /// \brief Calls ezPlugin::UnloadPlugin on all loaded plugins.
  virtual void DoUnloadPlugins();

  /// \brief Cleans up the GAL device.
  virtual void DoShutdownGraphicsDevice();

  /// Called at the very end to shut down all log writers that may need de-initialization.
  virtual void DoShutdownLogWriters();

  ///
  /// Application Update
  ///
  
  /// \brief Override to implement proper input handling.
  ///
  /// The default implementation handles ESC (close app), F5 (reload resources) and F8 (capture profiling info).
  virtual void ProcessApplicationInput();

  ///
  /// Data
  ///

  struct GameStateData
  {
    EZ_DECLARE_POD_TYPE();

    ezGameState* m_pState;
    ezWorld* m_pLinkedToWorld;
  };

  /// \brief Only few applications can have more than one game state (e.g. the editor).
  const ezHybridArray<GameStateData, 4>& GetAllGameStates() const { return m_GameStates; }

  /// \brief Stores what is given to the constructor
  ezString m_sAppProjectPath;

private:
  static ezGameApplication* s_pGameApplicationInstance;

  void RenderFps();
  void RenderConsole();

  void DestroyGameState(ezUInt32 idx);
  void UpdateInput();

  void UpdateWorldsAndExtractViews();
  ezDelegateTask<void> m_UpdateTask;

  struct WindowContext
  {
    ezWindowBase* m_pWindow;
    ezGALSwapChainHandle m_hSwapChain;
    bool m_bFirstFrame;
  };

  struct WorldData
  {
    void UpdateBefore();
    void UpdateAfter();
    void CreateWorldModules();
    void ReinitWorldModules();
    void DestroyWorldModules();

    ezDefaultTimeStepSmoothing* m_pTimeStepSmoothing;
    ezHybridArray<ezWorldModule*, 8> m_WorldModules;
    ezWorld* m_pWorld;
  };


  ezString m_sAppName;
  ezDynamicArray<WindowContext> m_Windows;
  ezHybridArray<WorldData, 4> m_Worlds;
  ezHybridArray<GameStateData, 4> m_GameStates;

  bool m_bWasQuitRequested;
  bool m_bShowFps;
  bool m_bShowConsole;
  ezGameApplicationType m_AppType;

  ezUniquePtr<ezConsole> m_pConsole;
};
