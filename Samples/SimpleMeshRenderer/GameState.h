#pragma once

#include <CoreUtils/Graphics/Camera.h>
#include <RendererCore/Pipeline/View.h>
#include <GameFoundation/GameApplication.h>

class GameWindow;

class GameState : public ezGameStateBase
{
public:
  GameState();

private:
  virtual void Activate() override;
  virtual void Deactivate() override;
  virtual void BeforeWorldUpdate() override;
  
  void UpdateInputSystem(ezTime UpdateDiff);
  void UpdateGameInput();
  
  void SetupInput();
  void CreateGameLevelAndRenderPipeline(ezGALRenderTargetViewHandle hBackBuffer, ezGALRenderTargetViewHandle hDSV);
  void DestroyGameLevel();

  GameWindow* m_pWindow;

  ezRenderPipeline* m_pRenderPipeline;

  ezWorld* m_pWorld;
  ezCamera m_Camera;

  ezView* m_pView;
};