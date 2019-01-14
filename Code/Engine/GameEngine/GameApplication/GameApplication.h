#pragma once

#include <GameEngine/Basics.h>
#include <GameEngine/GameState/GameState.h>

#include <Foundation/Threading/DelegateTask.h>
#include <Foundation/Types/UniquePtr.h>
#include <GameEngine/Console/ConsoleFunction.h>

#include <GameEngine/GameApplication/GameApplicationBase.h>

class ezConsole;

// TODO: update comments below

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
  typedef ezGameApplicationBase SUPER;

  /// szProjectPath may be nullptr, if FindProjectDirectory() is overridden.
  ezGameApplication(const char* szAppName, const char* szProjectPath);
  ~ezGameApplication();

  /// \brief Returns the ezGameApplication singleton
  static ezGameApplication* GetGameApplicationInstance() { return s_pGameApplicationInstance; }

  /// \brief When the graphics device is created, by default the game application will pick a platform specific implementation. This
  /// function allows to override that by setting a custom function that creates a graphics device.
  static void SetOverrideDefaultDeviceCreator(ezDelegate<ezGALDevice*(const ezGALDeviceCreationDescription&)> creator);

  /// \todo Fix this comment
  /// \brief virtual function that is called by DoProjectSetup(). The result is passed to ezFileSystem::SetProjectDirectory
  ///
  /// The default implementation relies on a valid path in m_sAppProjectPath.
  /// It passes that to SearchProjectDirectory() together with the path to the application binary,
  /// to search for a project somewhere relative to where the application is installed.
  ///
  /// Override this, if your application uses a different folder structure or way to specify the project directory.
  virtual ezString FindProjectDirectory() const override;

  /// \brief Used at runtime (by the editor) to reload input maps. Forwards to Init_ConfigureInput()
  void ReinitializeInputConfig();

protected:
  virtual ezUniquePtr<ezWindowOutputTargetBase> CreateWindowOutputTarget(ezWindowBase* pWindow) override;
  virtual void DestroyWindowOutputTarget(ezUniquePtr<ezWindowOutputTargetBase> pOutputTarget) override;

  virtual void BeforeCoreSystemsStartup() override;

  virtual void Init_ConfigureInput() override;
  virtual void Init_ConfigureAssetManagement() override;
  virtual void Init_LoadRequiredPlugins() override;
  virtual void Init_SetupDefaultResources() override;
  virtual void Init_SetupGraphicsDevice() override;
  virtual void Deinit_ShutdownGraphicsDevice() override;

  virtual bool IsGameUpdateEnabled() const;

  virtual bool Run_ProcessApplicationInput();
  virtual void Run_WorldUpdateAndRender() override;

  /// \brief Stores what is given to the constructor
  ezString m_sAppProjectPath;

protected:
  static ezGameApplication* s_pGameApplicationInstance;

  void RenderFps();
  void RenderConsole();

  void UpdateWorldsAndExtractViews();
  ezDelegateTask<void> m_UpdateTask;

  static ezDelegate<ezGALDevice*(const ezGALDeviceCreationDescription&)> s_DefaultDeviceCreator;

  bool m_bShowConsole = false;
  ezUniquePtr<ezConsole> m_pConsole;

#ifdef BUILDSYSTEM_ENABLE_MIXEDREALITY_SUPPORT
  ezUniquePtr<class ezMixedRealityFramework> m_pMixedRealityFramework;
#endif
};
