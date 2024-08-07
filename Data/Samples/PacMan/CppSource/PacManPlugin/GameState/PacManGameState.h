#pragma once

#include <Core/Input/Declarations.h>
#include <Core/World/Declarations.h>
#include <GameEngine/GameApplication/GameApplication.h>
#include <GameEngine/GameState/GameState.h>
#include <PacManPlugin/PacManPluginDLL.h>

// Every game can have a single 'game state' for high-level logic.
// For more details, see https://ezengine.net/pages/docs/runtime/application/game-state.html
class PacManGameState : public ezGameState
{
  EZ_ADD_DYNAMIC_REFLECTION(PacManGameState, ezGameState);

public:
  static ezHashedString s_sStats;
  static ezHashedString s_sCoinsEaten;
  static ezHashedString s_sPacManState;

public:
  PacManGameState();
  ~PacManGameState();

  // Called at the start of each frame. The typical per-frame decisions should be done here.
  virtual void ProcessInput() override;

protected:
  virtual void ConfigureInputActions() override;
  virtual void ConfigureMainCamera() override;

private:
  virtual void OnActivation(ezWorld* pWorld, ezStringView sStartPosition, const ezTransform* pStartPosition) override;
  virtual void OnDeactivation() override;
  virtual void AfterWorldUpdate() override;
  virtual ezResult SpawnPlayer(ezStringView sStartPosition, const ezTransform* pStartPosition) override;

  void ResetState();

  // How many coins we have in the scene, in total.
  ezUInt32 m_uiNumCoinsTotal = 0;
};
