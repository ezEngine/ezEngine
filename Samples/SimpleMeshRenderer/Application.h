#pragma once

#include <Foundation/Time/Time.h>
#include <Foundation/Time/DefaultTimeStepSmoothing.h>
#include <CoreUtils/Graphics/Camera.h>
#include <RendererCore/Pipeline/View.h>
#include <GameFoundation/GameApplication.h>

class GameWindow;

class SampleApp : public ezGameApplication
{
public:
  SampleApp();

  virtual void AfterEngineInit() override;
  virtual void BeforeEngineShutdown() override;

private:
  virtual void UpdateInput() override;
  
  void UpdateInputSystem(ezTime UpdateDiff);
  void UpdateGameInput();
  
  void SetupInput();
  void CreateGameLevelAndRenderPipeline(ezGALRenderTargetConfigHandle hRTConfig);
  void DestroyGameLevel();

  GameWindow* m_pWindow;

  ezRenderPipeline* m_pRenderPipeline;

  ezWorld* m_pWorld;
  ezCamera m_Camera;

  ezView* m_pView;
};