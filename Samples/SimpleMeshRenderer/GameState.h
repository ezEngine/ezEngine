#pragma once

#include <CoreUtils/Graphics/Camera.h>
#include <RendererCore/Pipeline/View.h>
#include <GameFoundation/GameApplication/GameApplication.h>

class GameWindow;

class SimpleMeshRendererGameState : public ezGameState
{
  EZ_ADD_DYNAMIC_REFLECTION(SimpleMeshRendererGameState, ezGameState);

public:
  SimpleMeshRendererGameState();

private:
  virtual void Activate() override;
  virtual void Deactivate() override;
  
  void SetupInput();
  void CreateGameLevelAndRenderPipeline(ezGALRenderTargetViewHandle hBackBuffer, ezGALRenderTargetViewHandle hDSV);
  void DestroyGameLevel();

  virtual void ProcessInput() override;

  virtual ezGameStateCanHandleThis CanHandleThis(ezGameApplicationType AppType, ezWorld* pWorld) const override;

  GameWindow* m_pWindow;

  ezRenderPipeline* m_pRenderPipeline;

  ezWorld* m_pWorld;
  ezCamera m_Camera;

  ezView* m_pView;
};