#pragma once

#include <Core/Application/Application.h>
#include <Foundation/Time/Time.h>
#include <Foundation/Time/DefaultTimeStepSmoothing.h>
#include <CoreUtils/Graphics/Camera.h>
#include <SampleApp_RTS/Level.h>
#include <SampleApp_RTS/Rendering/Renderer.h>
#include <SampleApp_RTS/General/Window.h>
#include <GameUtils/DataStructures/ObjectSelection.h>

class SampleGameApp : public ezApplication
{
public:
  SampleGameApp();

  virtual void AfterEngineInit() EZ_OVERRIDE;
  virtual void BeforeEngineShutdown() EZ_OVERRIDE;

  virtual ezApplication::ApplicationExecution Run() EZ_OVERRIDE;

  ezVec2I32 GetPickedGridCell(ezVec3* out_vIntersection = NULL) const;

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

  ezCamera m_Camera;
  ezObjectSelection* m_pSelectedUnits;

  ezDefaultTimeStepSmoothing m_TimeStepSmoother;
};