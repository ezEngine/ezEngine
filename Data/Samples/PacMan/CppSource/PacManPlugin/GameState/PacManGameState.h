#pragma once

#include <Core/Input/Declarations.h>
#include <Core/World/Declarations.h>
#include <GameEngine/GameApplication/GameApplication.h>
#include <GameEngine/GameState/FallbackGameState.h>
#include <GameEngine/GameState/GameState.h>
#include <PacManPlugin/PacManPluginDLL.h>

class PacManGameState : public ezFallbackGameState
{
  EZ_ADD_DYNAMIC_REFLECTION(PacManGameState, ezFallbackGameState);

public:
  PacManGameState();
  ~PacManGameState();

  virtual ezGameStatePriority DeterminePriority(ezWorld* pWorld) const override;

  virtual void ProcessInput() override;

protected:
  virtual void ConfigureMainWindowInputDevices(ezWindow* pWindow) override;
  virtual void ConfigureInputActions() override;
  virtual void ConfigureMainCamera() override;

private:
  virtual void OnActivation(ezWorld* pWorld, const ezTransform* pStartPosition) override;
  virtual void OnDeactivation() override;
  virtual void BeforeWorldUpdate() override;
  virtual void AfterWorldUpdate() override;
  virtual ezResult SpawnPlayer(const ezTransform* pStartPosition) override;

  void ResetState();

  ezUInt32 m_uiNumCoinsTotal = 0;
};
