#pragma once

#include <GameEngine/Basics.h>
#include <GameEngine/GameState/GameState.h>

#include <Foundation/Threading/DelegateTask.h>
#include <Foundation/Types/UniquePtr.h>
#include <GameEngine/Configuration/PlatformProfile.h>
#include <GameEngine/Console/ConsoleFunction.h>

#include <GameEngine/GameApplication/GameApplicationBase.h>

class ezDefaultTimeStepSmoothing;
class ezConsole;
struct ezWorldDesc;
class ezImage;
struct ezWindowCreationDesc;

/// Allows custom code to inject logic at specific update points.
/// The events are listed in the order in which they typically happen.
struct ezGameApplicationEvent
{
  enum class Type
  {
    BeginAppTick,
    BeforeWorldUpdates,
    AfterWorldUpdates,
    BeforeUpdatePlugins,
    AfterUpdatePlugins,
    BeforePresent,
    AfterPresent,
    EndAppTick,
    AfterWorldCreated,    // m_pData -> ezWorld*
    BeforeWorldDestroyed, // m_pData -> ezWorld*
  };

  Type m_Type;
  void* m_pData = nullptr;
};

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
/// directory configurations are read from DDL files. These can be configured by hand or using ezEditor.
///
/// You are NOT supposed to implement game functionality by deriving from ezGameApplication.
/// Instead see ezGameState.
///
/// ezGameApplication will create exactly one ezGameState by looping over all available ezGameState types
/// (through reflection) and picking the one whose DeterminePriority function returns the highest priority.
/// That game state will live throughout the entire application life-time and will be stepped every frame.
class EZ_GAMEENGINE_DLL ezGameApplication : public ezGameApplicationBase
{
public:
  /// szProjectPath may be nullptr, if FindProjectDirectory() is overridden.
  ezGameApplication(const char* szAppName, ezGameApplicationType type, const char* szProjectPath);
  ~ezGameApplication();

  /// \brief Returns the ezGameApplication singleton
  static ezGameApplication* GetGameApplicationInstance() { return s_pGameApplicationInstance; }

  /// \brief Overrides ezApplication::Run() and implements a typical game update.
  ///
  /// Calls ezWindowBase::ProcessWindowMessages() on all windows that have been added through AddWindow()
  /// As long as there are any main views added to ezRenderLoop it \n
  ///   Updates the global ezClock. \n
  ///   Calls UpdateInput() \n
  ///   Calls UpdateWorldsAndRender() \n
  virtual ezApplication::ApplicationExecution Run() override;


  /// \brief When the graphics device is created, by default the game application will pick a platform specific implementation. This
  /// function allows to override that by setting a custom function that creates a graphics device.
  static void SetOverrideDefaultDeviceCreator(ezDelegate<ezGALDevice*(const ezGALDeviceCreationDescription&)> creator);
  

  /// \brief Checks all parent directories of the scene file and tries to find a file called
  /// 'ezProject' (no extension) which marks the project directory.
  /// Returns an empty string, if no such directory could be found.
  ezString FindProjectDirectoryForScene(const char* szScene) const;

  /// \brief Tries to find the project directory by concatenating the start directory and the relative path
  /// to the project file. As long as the file is not found, the next parent of the start directory is tried.
  /// Returns an empty string, if no such directory could be found.
  ezString SearchProjectDirectory(const char* szStartDirectory, const char* szRelPathToProjectFile) const;

  /// \todo Fix this comment
  /// \brief virtual function that is called by DoProjectSetup(). The result is passed to ezFileSystem::SetProjectDirectory
  ///
  /// The default implementation relies on a valid path in m_sAppProjectPath.
  /// It passes that to SearchProjectDirectory() together with the path to the application binary,
  /// to search for a project somewhere relative to where the application is installed.
  ///
  /// Override this, if your application uses a different folder structure or way to specify the project directory.
  virtual ezString FindProjectDirectory() const;

  /// \brief Returns what was passed to the constructor.
  ezGameApplicationType GetAppType() const { return m_AppType; }

  /// \brief Used at runtime (by the editor) to reload input maps. Forwards to DoConfigureInput()
  void ReinitializeInputConfig();

  ezEvent<const ezGameApplicationEvent&> m_Events;


  const ezPlatformProfile& GetPlatformProfile() const { return m_PlatformProfile; }

protected:
  virtual ezUniquePtr<ezWindowOutputTargetBase> CreateWindowOutputTarget(ezWindowBase* pWindow) override;
  virtual void DestroyWindowOutputTarget(ezUniquePtr<ezWindowOutputTargetBase> pOutputTarget) override;

protected:

  /// \brief Calls Update on all worlds and renders all views through ezRenderLoop::Render()
  void UpdateWorldsAndRender();

  virtual void UpdateWorldsAndRender_Begin();
  virtual void UpdateWorldsAndRender_Middle();
  virtual void UpdateWorldsAndRender_End();

  virtual void BeforeCoreStartup() override;

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

  /// \brief Returns the name of the platform profile that should be used by default.
  ///
  /// If the '-profile "XYZ"' command line argument is provided, it takes precedence over this.
  virtual const char* GetPreferredPlatformProfile() const { return "PC"; }

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
  /// The default implementation reads the data directory configuration from a DDL file in the project folder.
  virtual void DoSetupDataDirectories();

  /// \brief Called by DoProjectSetup() after DoSetupDataDirectories(). Loads plugins that the application should always load.
  /// The default implementation loads 'ezInspectorPlugin' in development builds
  virtual void DoLoadCustomPlugins();

  /// \brief Called by DoProjectSetup() after DoLoadCustomPlugins().
  /// The default implementation uses ezApplicationPluginConfig to load all manual plugins.
  virtual void DoLoadPluginsFromConfig();

  virtual void DoLoadPlatformProfile();

  /// \brief Called by DoProjectSetup() after DoLoadPluginsFromConfig().
  /// The default implementation sets up some common fallbacks.
  virtual void DoSetupDefaultResources();

  /// \brief Called by DoProjectSetup() after DoSetupDefaultResources().
  /// Creates a GAL device for rendering.
  virtual void DoSetupGraphicsDevice();

  /// \brief Called by DoProjectSetup() after DoSetupGraphicsDevice().
  /// The default implementation uses ezGameAppInputConfig to read the input configuration from a DDL file in the project folder.
  /// Additionally it configures ESC, F5 and F8 to be 'GameApp::CloseApp', 'GameApp::ReloadResources' and 'GameApp::CaptureProfiling'
  /// respectively.
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

  /// \brief Does all input handling on input manager and game states.
  void UpdateInput();

  /// \brief Stores what is given to the constructor
  ezString m_sAppProjectPath;

  ezPlatformProfile m_PlatformProfile;

protected:
  static ezGameApplication* s_pGameApplicationInstance;

  void RenderFps();
  void RenderConsole();

  void UpdateWorldsAndExtractViews();
  ezDelegateTask<void> m_UpdateTask;

  static ezDelegate<ezGALDevice*(const ezGALDeviceCreationDescription&)> s_DefaultDeviceCreator;
  
  bool m_bShowConsole = false;
  
  ezGameApplicationType m_AppType;

  ezUniquePtr<ezConsole> m_pConsole;


#ifdef BUILDSYSTEM_ENABLE_MIXEDREALITY_SUPPORT
  ezUniquePtr<class ezMixedRealityFramework> m_pMixedRealityFramework;
#endif
};
