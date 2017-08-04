#pragma once

#include <GameEngine/Basics.h>
#include <GameEngine/GameState/FallbackGameState.h>
#include <Foundation/Types/UniquePtr.h>

class ezWindowsMixedRealityCamera;
class ezSurfaceReconstructionMeshManager;
class ezMixedRealityFramework;

/// \brief Default gamestate for games that support mixed reality devices.
///
/// Handles a single MR controlled camera.
/// TODO: Should fallback to ezFallbackGameState behavior if no MR device is available.
class EZ_GAMEENGINE_DLL ezMixedRealityGameState : public ezFallbackGameState
{
  EZ_ADD_DYNAMIC_REFLECTION(ezMixedRealityGameState, ezFallbackGameState)

public:

  ezMixedRealityGameState();
  virtual ~ezMixedRealityGameState();

#ifdef BUILDSYSTEM_ENABLE_MIXEDREALITY_SUPPORT

  virtual void OnActivation(ezWorld* pWorld) override;
  virtual void OnDeactivation() override;

  virtual void ProcessInput() override;

  /// \brief Returns -1 if pWorld == nullptr or no MR device is available, 1 otherwise.
  virtual float CanHandleThis(ezGameApplicationType AppType, ezWorld* pWorld) const override;

protected:

  virtual void SetupMainView(ezGALRenderTargetViewHandle hBackBuffer) override;

  virtual void ConfigureInputActions() override;
  virtual void OnHolographicCameraAdded(const ezWindowsMixedRealityCamera& camera);
#endif
};





