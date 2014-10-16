#pragma once

#include "Level.h"
#include <Foundation/Threading/TaskSystem.h>
#include <Foundation/Time/Time.h>
#include <Foundation/Time/DefaultTimeStepSmoothing.h>
#include <Core/Application/Application.h>
#include <Core/Input/VirtualThumbStick.h>
#include <RendererCore/Pipeline/View.h>

class ezGALDevice;
class GameWindow;

class SampleGameApp : public ezApplication
{
public:
  SampleGameApp();

  virtual void AfterEngineInit() override;
  virtual void BeforeEngineShutdown() override;

  virtual ezApplication::ApplicationExecution Run() override;

  static ezGALDevice* GetDevice()
  {
    return s_pDevice;
  }

private:
  friend LRESULT CALLBACK WndProc (HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

  void InitRendering();

  void RenderSingleFrame();
  void UpdateInput(ezTime UpdateDiff);

  void GameLoop();
  void SetupInput();
  void CreateGameLevel();
  void DestroyGameLevel();

  class WorldUpdateTask : public ezTask
  {
  public:
    WorldUpdateTask(SampleGameApp* pApp);

  private:
    virtual void Execute() override;

    SampleGameApp* m_pApp;
  };

  WorldUpdateTask m_WorldUpdateTask;

  bool m_bActiveRenderLoop;
  Level* m_pLevel;
  GameWindow* m_pWindow;

  ezDefaultTimeStepSmoothing m_TimeStepSmoother;
  ezVirtualThumbStick* m_pThumbstick;
  ezVirtualThumbStick* m_pThumbstick2;

  static ezGALDevice* s_pDevice;
  ezView m_View;
};