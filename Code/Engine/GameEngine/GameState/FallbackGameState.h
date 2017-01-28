#pragma once

#include <GameEngine/GameState/GameState.h>
#include <Core/Graphics/Camera.h>

class ezCameraComponent;

/// \brief ezFallbackGameState is an ezGameState that can handle existing worlds when no other game state is available.
///
/// This game state returns a priority of 0 in CanHandleThis() and therefore only takes over when
/// no other game state is available.
/// It implements a simple first person camera to fly around a scene.
///
/// This game state cannot be used in stand-alone applications that require the game state to create
/// a new world. It is mainly for ezEditor and ezPlayer which make sure that a world already exists.
class EZ_GAMEENGINE_DLL ezFallbackGameState : public ezGameState
{
  EZ_ADD_DYNAMIC_REFLECTION(ezFallbackGameState, ezGameState)

public:
  ezFallbackGameState();

  virtual void ProcessInput() override;
  virtual void AfterWorldUpdate() override;

  /// \brief Returns -1 if pWorld == nullptr, 0 otherwise.
  virtual float CanHandleThis(ezGameApplicationType AppType, ezWorld* pWorld) const override;

protected:
  virtual void ConfigureInputActions() override;

  virtual const ezCameraComponent* FindActiveCameraComponent();

  ezInt32 m_iActiveCameraComponentIndex;

};
