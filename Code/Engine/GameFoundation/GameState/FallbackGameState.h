#pragma once

#include <GameFoundation/GameState/GameState.h>
#include <CoreUtils/Graphics/Camera.h>

class EZ_GAMEFOUNDATION_DLL ezFallbackGameState : public ezGameState
{
  EZ_ADD_DYNAMIC_REFLECTION(ezFallbackGameState, ezGameState)

public:
  virtual void ProcessInput() override;

  virtual float CanHandleThis(ezGameApplicationType AppType, ezWorld* pWorld) const override;

protected:
  virtual void ezFallbackGameState::ConfigureInputActions() override;

private:


};
