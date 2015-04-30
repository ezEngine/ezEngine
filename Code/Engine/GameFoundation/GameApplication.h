#pragma once

#include <GameFoundation/Basics.h>

#include <Foundation/Time/DefaultTimeStepSmoothing.h>
#include <Foundation/Threading/DelegateTask.h>

#include <Core/Application/Application.h>

#include <RendererFoundation/Device/SwapChain.h>

class ezWindowBase;
class ezWorld;

class EZ_GAMEFOUNDATION_DLL ezGameApplication : public ezApplication
{
public:
  ezGameApplication();
  ~ezGameApplication();

  /// \brief Typically called in application's AfterEngineInit()
  virtual void Initialize();

  /// \brief Typically called in application's BeforeEngineShutdown()
  virtual void Deinitialize();

  ezGALSwapChainHandle AddWindow(ezWindowBase* pWindow);
  void RemoveWindow(ezWindowBase* pWindow);

protected:

  virtual void UpdateInput();

private:

  virtual ezApplication::ApplicationExecution Run() override;

  void UpdateWorldsAndExtractViews();
  ezDelegateTask<void> m_UpdateTask;

  struct WindowContext
  {
    ezWindowBase* m_pWindow;
    ezGALSwapChainHandle m_hSwapChain;
  };

  ezDynamicArray<WindowContext> m_Windows;

  ezDefaultTimeStepSmoothing m_TimeStepSmoother;

  bool m_bShouldRun;
};
