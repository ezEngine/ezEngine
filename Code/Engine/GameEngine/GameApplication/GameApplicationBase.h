#pragma once

#include <GameEngine/Basics.h>

#include <GameEngine/Configuration/PlatformProfile.h>
#include <GameEngine/Console/ConsoleFunction.h>
#include <GameEngine/GameApplication/WindowOutputTargetBase.h>
#include <GameEngine/GameState/GameState.h>

#include <Core/Application/Application.h>

#include <Foundation/Types/UniquePtr.h>

class ezWindowBase;
struct ezWindowCreationDesc;
class ezWorld;

class EZ_GAMEENGINE_DLL ezGameApplicationBase : public ezApplication
{
public:
  typedef ezApplication SUPER;

  ezGameApplicationBase(const char* szAppName);
  ~ezGameApplicationBase();

  /// \name Basics
  ///@{

public:

  /// \brief Returns the ezGameApplicationBase singleton
  static ezGameApplicationBase* GetGameApplicationBaseInstance() { return s_pGameApplicationBaseInstance; }

  /// \brief Calling this function requests that the application quits after the current invocation of Run() finishes.
  ///
  /// Can be overridden to prevent quitting under certain conditions.
  virtual void RequestQuit();

  /// \brief Returns whether RequestQuit() was called.
  EZ_ALWAYS_INLINE bool WasQuitRequested() const { return m_bWasQuitRequested; }

protected:
  static ezGameApplicationBase* s_pGameApplicationBaseInstance;

  bool m_bWasQuitRequested = false;

  ///@}
  /// \name Window Management
  ///@{

public:
  /// \brief Adds a top level window to the application.
  ///
  /// An output target is created for that window. Run() will call ezWindowBase::ProcessWindowMessages()
  /// on all windows that have been added.
  /// Most applications should add exactly one such window to the game application.
  /// Only few applications will add zero or multiple windows.
  ezWindowOutputTargetBase* AddWindow(ezWindowBase* pWindow);

  /// \brief Adds a top level window to the application with a custom output target.
  void AddWindow(ezWindowBase* pWindow, ezUniquePtr<ezWindowOutputTargetBase> pOutputTarget);

  /// \brief Removes a previously added window. Destroys its output target. Should be called at application shutdown.
  void RemoveWindow(ezWindowBase* pWindow);

  /// \brief Can be called by code that creates windows (e.g. an ezGameStateBase) to adjust or override settings, such as the window title
  /// or resolution.
  virtual void AdjustWindowCreation(ezWindowCreationDesc& desc) {}

  /// \brief Calls ezWindowBase::ProcessWindowMessages() on all known windows. Returns true, if any windows are available, at all.
  ///
  /// \note This should actually never be executed manually. It is only public for very specific edge cases.
  /// Otherwise this function is automatically executed once every frame.
  bool ProcessWindowMessages();

  /// \brief Returns the ezWindowOutputTargetBase object that is associated with the given window. The window must have been added via
  /// AddWindow()
  ezWindowOutputTargetBase* GetWindowOutputTarget(const ezWindowBase* pWindow) const;

  /// \brief Sets the ezWindowOutputTargetBase for a given window. The window must have been added via AddWindow()
  ///
  /// The previous ezWindowOutputTargetBase object (if any) will be destroyed.
  void SetWindowOutputTarget(const ezWindowBase* pWindow, ezUniquePtr<ezWindowOutputTargetBase> pOutputTarget);

protected:
  virtual ezUniquePtr<ezWindowOutputTargetBase> CreateWindowOutputTarget(ezWindowBase* pWindow) = 0;
  virtual void DestroyWindowOutputTarget(ezUniquePtr<ezWindowOutputTargetBase> pOutputTarget) = 0;

  struct WindowContext
  {
    ezWindowBase* m_pWindow;
    ezUniquePtr<ezWindowOutputTargetBase> m_pOutputTarget;
    bool m_bFirstFrame = true;
  };

  ezDynamicArray<WindowContext> m_Windows;

public:
  /// \brief Does a profiling capture and writes it to disk at ':appdata'
  void TakeProfilingCapture();

  ///@}
  /// \name Screenshots
  ///@{

public:
  /// \brief Schedules a screenshot to be taken at the end of the frame.
  ///
  /// After taking a screenshot, StoreScreenshot() is executed, which may decide where to write the result to.
  void TakeScreenshot();

protected:
  /// \brief Called with the result from taking a screenshot. The default implementation writes the image to disk at ':appdata/Screenshots'
  virtual void StoreScreenshot(const ezImage& image);

  void ExecuteTakeScreenshot(ezWindowOutputTargetBase* pOutputTarget);

  bool m_bTakeScreenshot = false;

  /// expose TakeScreenshot() as a console function
  ezConsoleFunction<void()> m_ConFunc_TakeScreenshot;

  ///@}
  /// \name GameState
  ///@{
public:
  /// \brief Creates and activates the game state for this application.
  ///
  /// If the application already has a world (such as the editor), it can pass this to the newly created game state.
  /// Otherwise the game state should create its own world.
  ///
  /// In the editor case, there are cases where a 'player start position' is specified, which can be used
  /// by the game state to place the player.
  ezResult ActivateGameState(ezWorld* pWorld = nullptr, const ezTransform* pStartPosition = nullptr);

  /// \brief Deactivates and destroys the active game state.
  void DeactivateGameState();

  /// \brief Returns the currently active game state. Could be nullptr.
  ezGameState* GetActiveGameState() const { return m_pGameState.Borrow(); }

  /// \brief Returns the currently active game state IF it was created for the given world.
  ///
  /// This is mostly for editor use cases, where some documents want to handle the game state, but only
  /// it it was set up for a particular document.
  ezGameState* GetActiveGameStateLinkedToWorld(ezWorld* pWorld) const;

protected:
  /// \brief Creates a game state for the application to use.
  ///
  /// \a pWorld is typically nullptr in a stand-alone app, but may be existing already when called from the editor.
  ///
  /// The default implementation will query all available game states for the best match.
  /// By overriding this, one can also just create a specific game state directly.
  virtual ezUniquePtr<ezGameState> CreateGameState(ezWorld* pWorld);

  /// \brief Allows to override whether a game state is created and activated at application startup.
  ///
  /// The default implementation just calls ActivateGameState(), but applications that run inside the editor override this to do nothing,
  /// as they only want the game state to become active during simulation, not during editing.
  virtual void ActivateGameStateAtStartup();

  ezUniquePtr<ezGameState> m_pGameState;
  ezWorld* m_pWorldLinkedWithGameState = nullptr;

  ///@}
  /// \name Platform Profile
  ///@{
public:

  /// \brief Returns the ezPlatformProfile that has been loaded for this application
  const ezPlatformProfile& GetPlatformProfile() const { return m_PlatformProfile; }


protected:

  ezPlatformProfile m_PlatformProfile;

  ///@}
  /// \name Application Startup
  ///@{
protected:
  virtual void BeforeCoreSystemsStartup() override;
  virtual void AfterCoreSystemsStartup() override;

  virtual ezString FindProjectDirectory() const = 0;
  virtual ezString GetBaseDataDirectoryPath() const;

  virtual void ExecuteInitFunctions();

  virtual void Init_PlatformProfile_SetPreferred();
  virtual void Init_ConfigureLogging();
  virtual void Init_ConfigureTelemetry();
  virtual void Init_FileSystem_SetSpecialDirs();
  virtual void Init_FileSystem_SetDataDirFactories();
  virtual void Init_LoadRequiredPlugins();
  virtual void Init_ConfigureAssetManagement();
  virtual void Init_FileSystem_ConfigureDataDirs();
  virtual void Init_LoadProjectPlugins();
  virtual void Init_PlatformProfile_LoadForRuntime();
  virtual void Init_ConfigureInput();
  virtual void Init_ConfigureTags();
  virtual void Init_ConfigureCVars();
  virtual void Init_SetupGraphicsDevice() = 0;
  virtual void Init_SetupDefaultResources();

  ///@}
  /// \name Application Shutdown
  ///@{
protected:
  virtual void BeforeHighLevelSystemsShutdown() override;
  virtual void BeforeCoreSystemsShutdown() override;

  virtual void Deinit_ShutdownGraphicsDevice() = 0;
  virtual void Deinit_UnloadPlugins();
  virtual void Deinit_ShutdownLogging();

  ///@}
};
