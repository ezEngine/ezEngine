#pragma once

#include <GameFoundation/Basics.h>
#include <GameFoundation/GameState.h>

#include <Foundation/Time/DefaultTimeStepSmoothing.h>
#include <Foundation/Threading/DelegateTask.h>

#include <Core/Application/Application.h>

#include <RendererFoundation/Device/SwapChain.h>

class ezWindowBase;
class ezWorld;

class EZ_GAMEFOUNDATION_DLL ezGameApplication : public ezApplication
{
public:
  ezGameApplication(ezGameState& initialGameState);
  ~ezGameApplication();

  // public ezApplication implementation:
  virtual ezApplication::ApplicationExecution Run() override;

  ezGALSwapChainHandle AddWindow(ezWindowBase* pWindow);
  void RemoveWindow(ezWindowBase* pWindow);

  ezGALSwapChainHandle GetSwapChain(const ezWindowBase* pWindow) const;

  void SetCurrentGameState(ezGameState& currentGameState);
  EZ_FORCE_INLINE ezGameState& GetCurrentGameState() const
  {
    return *m_pCurrentGameState;
  }

  void RequestQuit();
  EZ_FORCE_INLINE bool WasQuitRequested() const
  {
    return m_bWasQuitRequested;
  }

  /// \brief Checks all parent directories of the scene file and tries to find an 'ezProject' file which marks the project directory.
  /// Returns an empty string, if no such directory could be found.
  ezString FindProjectDirectoryForScene(const char* szScene) const;

  void SetupProject(const char* szProjectDir);

protected:

  void UpdateWorldsAndRender();  

  // private ezApplication implementation: these methods must not be overridden by derived classes from ezGameApplication
  virtual void AfterEngineInit() override;
  virtual void BeforeEngineShutdown() override;

private:

  void SetupDefaultResources();
  void UpdateInput();
  void UpdateWorldsAndExtractViews();
  ezDelegateTask<void> m_UpdateTask;

  struct WindowContext
  {
    ezWindowBase* m_pWindow;
    ezGALSwapChainHandle m_hSwapChain;
    bool m_bFirstFrame;
  };

  ezDynamicArray<WindowContext> m_Windows;

  ezGameState* m_pCurrentGameState;

  bool m_bWasQuitRequested;
};
