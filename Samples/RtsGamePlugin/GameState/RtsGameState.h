#pragma once

#include <RtsGamePlugin/RtsGamePlugin.h>
#include <GameEngine/GameApplication/GameApplication.h>
#include <GameEngine/GameState/GameState.h>
#include <GameEngine/GameState/FallbackGameState.h>
#include <Core/World/Declarations.h>
#include <Core/Input/Declarations.h>
#include <RtsGamePlugin/GameMode/MainMenuMode/MainMenuMode.h>

class RtsGameMode;

enum class RtsActiveGameMode
{
  None,
  MainMenuMode,
  BattleMode,
};

class EZ_RTSGAMEPLUGIN_DLL RtsGameState : public ezFallbackGameState
{
  EZ_ADD_DYNAMIC_REFLECTION(RtsGameState, ezFallbackGameState);

public:
  RtsGameState();

  virtual float CanHandleThis(ezGameApplicationType AppType, ezWorld* pWorld) const override;

  virtual void ProcessInput() override;

protected:
  virtual void ConfigureMainCamera() override;

private:
  virtual void OnActivation(ezWorld* pWorld) override;
  virtual void OnDeactivation() override;
  virtual void BeforeWorldUpdate() override;

  //////////////////////////////////////////////////////////////////////////
  // Game Mode
public:
  void SwitchToGameMode(RtsActiveGameMode mode);
   
private:
  void SetActiveGameMode(RtsGameMode* pMode);

  RtsActiveGameMode m_GameModeToSwitchTo = RtsActiveGameMode::None;
  RtsGameMode* m_pActiveGameMode = nullptr;

  // all the modes that the game has
  RtsMainMenuMode m_MainMenuMode;

  //////////////////////////////////////////////////////////////////////////
  // Input Handling
private:
  virtual void ConfigureInputDevices() override;
  virtual void ConfigureInputActions() override;
  void UpdateMousePosition();

  ezUInt32 m_uiMousePosX;
  ezUInt32 m_uiMousePosY;
  ezKeyState::Enum m_LeftClickState;
  ezKeyState::Enum m_RightClickState;
};
