#pragma once

#include <GameEngine/GameState/FallbackGameState.h>

#if EZ_ENABLED(EZ_PLATFORM_WINDOWS_UWP)
#include <WindowsMixedReality/MixedRealityGameState.h>
#endif

#if EZ_ENABLED(EZ_PLATFORM_WINDOWS_UWP)
typedef ezMixedRealityGameState ezParentGameState;
#else
typedef ezFallbackGameState ezParentGameState;
#endif

class SimpleMeshRendererGameState : public ezParentGameState
{
  EZ_ADD_DYNAMIC_REFLECTION(SimpleMeshRendererGameState, ezParentGameState);

public:
  SimpleMeshRendererGameState();
  ~SimpleMeshRendererGameState();

private:
  virtual void OnActivation(ezWorld* pWorld) override;
  virtual void OnDeactivation() override;

  virtual float CanHandleThis(ezGameApplicationType AppType, ezWorld* pWorld) const override;
  
  void CreateGameLevel();
  void DestroyGameLevel();
};