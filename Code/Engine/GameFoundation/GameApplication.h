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
  ezGameApplication(ezGameStateBase& initialGameState);
  ~ezGameApplication();

  ezGALSwapChainHandle AddWindow(ezWindowBase* pWindow);
  void RemoveWindow(ezWindowBase* pWindow);

  void SetCurrentGameState(ezGameStateBase& currentGameState);

  void RequestQuit();

private:

  virtual void AfterEngineInit() override;
  virtual void BeforeEngineShutdown() override;
  virtual ezApplication::ApplicationExecution Run() override;

  void UpdateInput();
  void UpdateWorldsAndExtractViews();
  ezDelegateTask<void> m_UpdateTask;

  struct WindowContext
  {
    ezWindowBase* m_pWindow;
    ezGALSwapChainHandle m_hSwapChain;
  };

  ezDynamicArray<WindowContext> m_Windows;

  ezDefaultTimeStepSmoothing m_TimeStepSmoother;

  ezGameStateBase* m_pCurrentGameState;

  bool m_bShouldRun;
};
