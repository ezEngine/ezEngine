#pragma once

#include <GameEngine/GameState/FallbackGameState.h>

#include <GameEngine/MixedReality/MixedRealityGameState.h>

class SimpleMeshRendererGameState : public ezMixedRealityGameState
{
  EZ_ADD_DYNAMIC_REFLECTION(SimpleMeshRendererGameState, ezMixedRealityGameState);

public:
  SimpleMeshRendererGameState();
  virtual ~SimpleMeshRendererGameState();

  virtual void ConfigureInputActions() override;
  virtual void ProcessInput() override;

private:
  virtual void OnActivation(ezWorld* pWorld) override;
  virtual void OnDeactivation() override;

  virtual float CanHandleThis(ezGameApplicationType AppType, ezWorld* pWorld) const override;
  
  void CreateGameLevel();
  void DestroyGameLevel();
  void MoveObjectToPosition(const ezVec3& pos);

  ezGameObjectHandle m_hSponza;
  ezGameObjectHandle m_hTree;
};