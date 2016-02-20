#pragma once

#include <GameFoundation/GameState/GameState.h>
#include <CoreUtils/Graphics/Camera.h>

/// \brief ezFallbackGameState is an ezGameState that can handle existing worlds when no other game state is available.
///
/// This game state returns a priority of 0 in CanHandleThis() and therefore only takes over when
/// no other game state is available.
/// It implements a simple first person camera to fly around a scene.
///
/// This game state cannot be used in stand-alone applications that require the game state to create
/// a new world. It is mainly for ezEditor and ezPlayer which make sure that a world already exists.
class EZ_GAMEFOUNDATION_DLL ezFallbackGameState : public ezGameState
{
  EZ_ADD_DYNAMIC_REFLECTION(ezFallbackGameState, ezGameState)

public:
  virtual void ProcessInput() override;

  /// \brief Returns -1 if pWorld == nullptr, 0 otherwise.
  virtual float CanHandleThis(ezGameApplicationType AppType, ezWorld* pWorld) const override;

protected:
  virtual void ConfigureInputActions() override;

};
