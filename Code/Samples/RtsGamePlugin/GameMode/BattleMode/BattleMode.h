#pragma once

#include <RtsGamePlugin/GameMode/GameMode.h>

class RtsBattleMode : public RtsGameMode
{
public:
  RtsBattleMode();
  ~RtsBattleMode();

protected:
  virtual void OnActivateMode() override;
  virtual void OnDeactivateMode() override;
  virtual void RegisterInputActions() override;
  virtual void OnProcessInput(const RtsMouseInputState& MouseInput) override;
  virtual void OnBeforeWorldUpdate() override;
};
