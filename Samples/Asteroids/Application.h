#pragma once

#include "Level.h"
#include <Foundation/Threading/TaskSystem.h>
#include <Foundation/Time/Time.h>
#include <Foundation/Time/DefaultTimeStepSmoothing.h>
#include <Core/Input/VirtualThumbStick.h>
#include <GameFoundation/GameApplication.h>

class GameWindow;

class SampleGameApp : public ezGameApplication
{
public:
  SampleGameApp();

  virtual void AfterEngineInit() override;
  virtual void BeforeEngineShutdown() override;

private:
  virtual void UpdateInput() override;

  void SetupInput();
  void CreateGameLevelAndRenderPipeline(ezGALRenderTargetConfigHandle hRTConfig);
  void DestroyLevelAndRenderPipeline();

  Level* m_pLevel;
  GameWindow* m_pWindow;

  ezVirtualThumbStick* m_pThumbstick;
  ezVirtualThumbStick* m_pThumbstick2;
};