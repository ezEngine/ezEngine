#pragma once

#include <GameEngine/GameState/GameState.h>

#include <Core/Console/ConsoleFunction.h>
#include <Core/GameApplication/GameApplicationBase.h>
#include <Foundation/Configuration/CVar.h>
#include <Foundation/Threading/DelegateTask.h>
#include <Foundation/Types/UniquePtr.h>

class ezQuakeConsole;

/// Which input actions the ezGameApplication may register and execute (see ezGameApplication::RegisterGameApplicationInputActions())
struct ezGameApplicationInputFlags
{
  using StorageType = ezUInt32;

  enum Enum
  {
    None = 0,
    All = 0xFFFFFFFF,

    LoadInputConfig = EZ_BIT(0),           ///< Whether to load the project's ezGameAppInputConfig

    Dev_EscapeToClose = EZ_BIT(8),         ///< Register the ESC key to close the application without asking
    Dev_Console = EZ_BIT(9),               ///< Register the F1 key to open the developer console
    Dev_ReloadResources = EZ_BIT(10),      ///< Register the F4 key to reload all resources
    Dev_ShowStats = EZ_BIT(11),            ///< Register the F5 key to show stats on screen, such as FPS
    Dev_CaptureProfilingInfo = EZ_BIT(12), ///< Register the F8 key to write a profiling capture to disk
    Dev_CaptureFrame = EZ_BIT(13),         ///< Register the F11 key to make a render frame capture (if capture plugin is available)
    Dev_Screenshot = EZ_BIT(14),           ///< Register the F12 key to save a screenshot to disk

    Dev_All = 0xFFFFFF00,

    Default = All
  };

  struct Bits
  {
    StorageType LoadInputConfig : 1;
    StorageType NonDevBits : 7;

    StorageType Dev_EscapeToClose : 1;
    StorageType Dev_Console : 1;
    StorageType Dev_ReloadResources : 1;
    StorageType Dev_ShowStats : 1;
    StorageType Dev_CaptureProfilingInfo : 1;
    StorageType Dev_CaptureFrame : 1;
    StorageType Dev_Screenshot : 1;
  };
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
/// (through reflection) and picking the one that's not marked as a "fallback" gamestate.
/// If none is found, it uses the fallback gamestate instead.
/// That game state will live throughout the entire application life-time and will be stepped every frame.
class EZ_GAMEENGINE_DLL ezGameApplication : public ezGameApplicationBase
{
public:
  static ezCVarBool cvar_AppVSync;
  static ezCVarBool cvar_AppShowFPS;

public:
  using SUPER = ezGameApplicationBase;

  /// szProjectPath may be nullptr, if FindProjectDirectory() is overridden.
  ezGameApplication(const char* szAppName, const char* szProjectPath);
  ~ezGameApplication();

  /// \brief Returns the ezGameApplication singleton
  static ezGameApplication* GetGameApplicationInstance() { return s_pGameApplicationInstance; }

  /// \brief Returns the active renderer of the current app. Either the default or overridden via -render command line flag.
  static ezStringView GetActiveRenderer();

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

  /// \brief Returns the project path that was given to the constructor (or modified by an overridden implementation).
  ezStringView GetAppProjectPath() const { return m_sAppProjectPath; }

  /// Call this to configure which actions the ezGameApplication may handle
  ///
  /// Apart from loading the input configuration from disk, this mostly sets up developer options.
  /// The function is typically called by ezGameState::ConfigureInputActions().
  /// Once you need full control over the keyboard bindings, you should write your own game state
  /// and override ezGameState::ConfigureInputActions() to not register all the developer options.
  void RegisterGameApplicationInputActions(ezBitflags<ezGameApplicationInputFlags> flags);

protected:
  virtual void Init_ConfigureAssetManagement() override;
  virtual void Init_LoadRequiredPlugins() override;
  virtual void Init_SetupDefaultResources() override;
  virtual void Init_SetupGraphicsDevice() override;
  virtual void Deinit_ShutdownGraphicsDevice() override;

  virtual ezGameUpdateMode GetGameUpdateMode() const override;

  virtual bool Run_ProcessApplicationInput() override;
  virtual void Run_AcquireImage() override;
  virtual void Run_WorldUpdateAndRender() override;
  virtual void Run_PresentImage() override;
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
  ezUniquePtr<ezQuakeConsole> m_pConsole;
};
