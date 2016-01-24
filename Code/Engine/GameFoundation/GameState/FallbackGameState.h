#pragma once

#include <GameFoundation/GameState/GameState.h>
#include <CoreUtils/Graphics/Camera.h>

class EZ_GAMEFOUNDATION_DLL ezFallbackGameState : public ezGameState
{
  EZ_ADD_DYNAMIC_REFLECTION(ezFallbackGameState, ezGameState)

public:
  ezFallbackGameState();
  virtual ~ezFallbackGameState();

  virtual void ProcessInput() override;

  virtual void Activate() override;
  virtual void Deactivate() override;
  
  virtual ezGameStateCanHandleThis CanHandleThis(ezGameApplicationType AppType, ezWorld* pWorld) const = 0;

private:
  void SetupInput();
  void CreateRenderPipeline(ezGALRenderTargetViewHandle hBackBuffer, ezGALRenderTargetViewHandle hDSV);

  ezWindow* m_pWindow;

  ezRenderPipeline* m_pRenderPipeline;

  mutable ezWorld* m_pWorld;
  ezCamera m_Camera;

  ezView* m_pView;
};
