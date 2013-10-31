#pragma once

#include <Core/Application/Application.h>
#include "Level.h"
#include <Core/Input/VirtualThumbStick.h>
#include <Foundation/Time/Time.h>
#include <Foundation/Time/DefaultTimeStepSmoothing.h>

class SampleGameApp : public ezApplication
{
public:
  SampleGameApp();

  virtual void AfterEngineInit() EZ_OVERRIDE;
  virtual void BeforeEngineShutdown() EZ_OVERRIDE;

  virtual ezApplication::ApplicationExecution Run() EZ_OVERRIDE;



private:
  friend LRESULT CALLBACK WndProc (HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

  void RenderSingleFrame();
  void UpdateInput(ezTime UpdateDiff);
  void RenderProjectiles();
  void RenderAsteroids();
  void RenderPlayerShips();

  void CreateAppWindow();
  void DestroyAppWindow();
  void GameLoop();
  void SetupInput();
  void CreateGameLevel();
  void DestroyGameLevel();

  bool m_bActiveRenderLoop;
  Level* m_pLevel;
  class GameWindow* m_pWindow;

  ezDefaultTimeStepSmoothing m_TimeStepSmoother;
  ezVirtualThumbStick* m_pThumbstick;
  ezVirtualThumbStick* m_pThumbstick2;
};