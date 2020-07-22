#pragma once

#include <Core/Graphics/Camera.h>
#include <GameEngine/GameState/GameState.h>

class ezCameraComponent;

/// \brief ezFallbackGameState is an ezGameState that can handle existing worlds when no other game state is available.
///
/// This game state returns a priority of 'Fallback' in DeterminePriority() and therefore only takes over when
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

  /// \brief Returns Priority::None if pWorld == nullptr, Priority::Fallback otherwise.
  virtual ezGameStatePriority DeterminePriority(ezWorld* pWorld) const override;

protected:
  virtual void ConfigureInputActions() override;
  virtual ezResult SpawnPlayer(const ezTransform* pStartPosition) override;

  virtual const ezCameraComponent* FindActiveCameraComponent();

  ezInt32 m_iActiveCameraComponentIndex;
};
