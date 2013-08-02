#pragma once

#include <Core/Application/Application.h>
#include "Level.h"

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
  void UpdateInput();
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
  bool m_bFullscreen;
  ezUInt32 m_uiResolutionX;
  ezUInt32 m_uiResolutionY;
  const char* m_szAppName;
  Level* m_pLevel;
};