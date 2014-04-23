#pragma once

#include <Core/Application/Application.h>
#include <Foundation/Time/Time.h>
#include <Foundation/Time/DefaultTimeStepSmoothing.h>
#include <CoreUtils/Graphics/Camera.h>
#include <RTS/Level.h>
#include <RTS/Rendering/Renderer.h>
#include <RTS/General/Window.h>
#include <GameUtils/DataStructures/ObjectSelection.h>
#include <CoreUtils/Debugging/DataTransfer.h>
#include <CoreUtils/Console/Console.h>

class SampleGameApp : public ezApplication
{
public:
  SampleGameApp();

  virtual void AfterEngineInit() override;
  virtual void BeforeEngineShutdown() override;

  virtual ezApplication::ApplicationExecution Run() override;

  ezVec2I32 GetPickedGridCell(ezVec3* out_vIntersection = nullptr) const;

private:
  friend LRESULT CALLBACK WndProc (HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

  void UpdateInput(ezTime UpdateDiff);
  void SetupInput();

  void SelectUnit();
  void SendUnit();

  bool m_bActiveRenderLoop;
  Level* m_pLevel;
  GameRenderer* m_pRenderer;
  GameWindow* m_pWindow;
  ezDataTransfer m_ScreenshotTransfer;
  ezDataTransfer m_StatsTransfer;
  ezConsole m_Console;
  bool m_bConsoleActive;

  ezCamera m_Camera;
  ezObjectSelection* m_pSelectedUnits;

  ezDefaultTimeStepSmoothing m_TimeStepSmoother;
};