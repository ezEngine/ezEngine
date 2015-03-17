#pragma once

#include <Foundation/Threading/TaskSystem.h>
#include <Foundation/Time/Time.h>
#include <Foundation/Time/DefaultTimeStepSmoothing.h>
#include <Core/Application/Application.h>
#include <CoreUtils/Graphics/Camera.h>
#include <RendererCore/Pipeline/View.h>

class ezGALDevice;
class GameWindow;
class MainRenderPass;

class SampleApp : public ezApplication
{
public:
  SampleApp();

  virtual void AfterEngineInit() override;
  virtual void BeforeEngineShutdown() override;

  virtual ezApplication::ApplicationExecution Run() override;

private:
  friend LRESULT CALLBACK WndProc (HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

  void InitRendering();
  void DeinitRendering();

  void UpdateInputSystem(ezTime UpdateDiff);
  void UpdateGameInput();
  void UpdateGame();

  void SetupInput();
  void CreateGameLevel();
  void DestroyGameLevel();

  class WorldUpdateTask : public ezTask
  {
  public:
    WorldUpdateTask(SampleApp* pApp);

  private:
    virtual void Execute() override;

    SampleApp* m_pApp;
  };

  WorldUpdateTask m_WorldUpdateTask;

  bool m_bActiveRenderLoop;
  GameWindow* m_pWindow;

  ezDefaultTimeStepSmoothing m_TimeStepSmoother;
  ezRenderPipeline* m_pRenderPipeline;
  MainRenderPass* m_pMainRenderPass;

  ezWorld* m_pWorld;
  ezCamera m_Camera;

  ezView m_View;
};