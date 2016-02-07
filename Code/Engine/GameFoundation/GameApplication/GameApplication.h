#pragma once

#include <GameFoundation/Basics.h>
#include <GameFoundation/GameState/GameState.h>

#include <Foundation/Time/DefaultTimeStepSmoothing.h>
#include <Foundation/Threading/DelegateTask.h>

#include <Core/Application/Application.h>

#include <RendererFoundation/Device/SwapChain.h>


class EZ_GAMEFOUNDATION_DLL ezGameApplication : public ezApplication
{
public:

  typedef ezInt32 WorldID;

  /// szProjectPath may be nullptr, if FindProjectDirectory() is overridden.
  ezGameApplication(ezGameApplicationType type, const char* szProjectPath);
  ~ezGameApplication();

  static ezGameApplication* GetGameApplicationInstance()
  {
    return s_pGameApplicationInstance;
  }

  virtual ezApplication::ApplicationExecution Run() override;


  ezGALSwapChainHandle AddWindow(ezWindowBase* pWindow);
  void RemoveWindow(ezWindowBase* pWindow);

  ezGALSwapChainHandle GetSwapChain(const ezWindowBase* pWindow) const;

  void ActivateGameStateForWorld(ezWorld* pWorld);
  void DeactivateGameStateForWorld(ezWorld* pWorld);

  void DestroyAllGameStates();
  void ActivateAllGameStates();
  void DeactivateAllGameStates();

  virtual void RequestQuit();
  EZ_FORCE_INLINE bool WasQuitRequested() const { return m_bWasQuitRequested; }

  /// \brief Checks all parent directories of the scene file and tries to find an 'ezProject' file which marks the project directory.
  /// Returns an empty string, if no such directory could be found.
  ezString FindProjectDirectoryForScene(const char* szScene) const;

  /// \brief Tries to find the project directory by concatenating the start directory and the relative path
  /// to the project file. As long as the file is not found, the next parent of the start directory is tried.
  /// Returns an empty string, if no such directory could be found.
  ezString FindProjectDirectory(const char* szStartDirectory, const char* szRelPathToProjectFile) const;

  ezGameApplicationType GetAppType() const { return m_AppType; }

  ezWorld* CreateWorld(const char* szWorldName, bool bCreateWorldModules);
  void DestroyWorld(ezWorld* pWorld);

  void CreateGameStateForWorld(ezWorld* pWorld);
  void DestroyGameStateForWorld(ezWorld* pWorld);

  ezGameState* GetGameStateForWorld(ezWorld* pWorld) const;

  void UpdateWorldModules(ezWorld* pWorld);
  void ReinitWorldModules(ezWorld* pWorld);

protected:

  void UpdateWorldsAndRender();

  // private ezApplication implementation: these methods must not be overridden by derived classes from ezGameApplication
  virtual void AfterCoreStartup() override;
  virtual void BeforeCoreShutdown() override;

  

protected:
  ///
  /// Project Initialization
  ///

  virtual void DoProjectSetup();
  virtual void DoSetupLogWriters();
  virtual void DoConfigureFileSystem();
  virtual void DoSetupDataDirectories();
  virtual void DoLoadCustomPlugins();
  virtual void DoLoadPluginsFromConfig();
  virtual void DoConfigureAssetManagement();
  virtual void DoSetupDefaultResources();
  virtual void DoSetupGraphicsDevice();
  virtual void DoConfigureInput();

  virtual ezString FindProjectDirectory() const;

  ///
  /// Project Shutdown
  ///

  virtual void DoUnloadPlugins();
  virtual void DoShutdownGraphicsDevice();
  virtual void DoShutdownLogWriters();

  ///
  /// Application Update
  ///
  
  virtual void ProcessApplicationInput();

  ///
  /// Data
  ///

  ezString m_sAppProjectPath;


private:
  static ezGameApplication* s_pGameApplicationInstance;

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
    void Update();
    void CreateWorldModules();
    void ReinitWorldModules();
    void DestroyWorldModules();

    ezDefaultTimeStepSmoothing m_TimeStepSmoothing;
    ezHybridArray<ezWorldModule*, 8> m_WorldModules;
    ezWorld* m_pWorld;
  };

  struct GameStateData
  {
    EZ_DECLARE_POD_TYPE();

    ezGameState* m_pState;
    //ezGameUpdateState m_UpdateState;
    ezWorld* m_pLinkedToWorld;
  };

  ezDynamicArray<WindowContext> m_Windows;
  ezHybridArray<WorldData, 4> m_Worlds;
  ezHybridArray<GameStateData, 4> m_GameStates;

  bool m_bWasQuitRequested;
  ezGameApplicationType m_AppType;

protected:
  const ezHybridArray<GameStateData, 4>& GetAllGameStates() const { return m_GameStates; }

};
