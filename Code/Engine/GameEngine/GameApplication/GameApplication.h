#pragma once

#include <GameEngine/GameEngineDLL.h>
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

  /// \brief Implementation of ezGameApplicationBase::FindProjectDirectory to define the 'project' special data directory.
  ///
  /// The default implementation will try to resolve m_sAppProjectPath to an absolute path. m_sAppProjectPath can be absolute itself,
  /// relative to ">sdk/" or relative to ezOSFile::GetApplicationDirectory().
  /// m_sAppProjectPath must be set either via the ezGameApplication constructor or manually set before project.
  ///
  /// Alternatively, ezGameApplication::FindProjectDirectory() must be overwritten.
  virtual ezString FindProjectDirectory() const override;

  /// \brief Used at runtime (by the editor) to reload input maps. Forwards to Init_ConfigureInput()
  void ReinitializeInputConfig();

protected:
  virtual ezResult BeforeCoreSystemsStartup() override;

  virtual void Init_ConfigureInput() override;
  virtual void Init_ConfigureAssetManagement() override;
  virtual void Init_LoadRequiredPlugins() override;
  virtual void Init_SetupDefaultResources() override;
  virtual void Init_SetupGraphicsDevice() override;
  virtual void Deinit_ShutdownGraphicsDevice() override;

  virtual bool IsGameUpdateEnabled() const override;

  virtual bool Run_ProcessApplicationInput() override;
  virtual void Run_WorldUpdateAndRender() override;
  virtual void Run_Present() override;
  virtual void Run_FinishFrame() override;

  /// \brief Stores what is given to the constructor
  ezString m_sAppProjectPath;

protected:
  static ezGameApplication* s_pGameApplicationInstance;

  void RenderFps();
  void RenderConsole();

  void UpdateWorldsAndExtractViews();
  ezSharedPtr<ezDelegateTask<void>> m_pUpdateTask;

  static ezDelegate<ezGALDevice*(const ezGALDeviceCreationDescription&)> s_DefaultDeviceCreator;

  bool m_bShowConsole = false;
  ezUniquePtr<ezConsole> m_pConsole;

#ifdef BUILDSYSTEM_ENABLE_MIXEDREALITY_SUPPORT
  ezUniquePtr<class ezMixedRealityFramework> m_pMixedRealityFramework;
#endif
};
