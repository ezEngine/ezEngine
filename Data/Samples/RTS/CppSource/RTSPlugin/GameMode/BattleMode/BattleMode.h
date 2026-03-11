#pragma once

#include <RTSPlugin/GameMode/GameMode.h>

class RtsBattleMode : public RtsGameMode
{
public:
  RtsBattleMode();
  ~RtsBattleMode();

protected:
  virtual void OnActivateMode() override;
  virtual void OnDeactivateMode() override;
  virtual void OnProcessInput(const RtsMouseInputState& MouseInput, bool bUiWantsInput) override;
  virtual void OnBeforeWorldUpdate() override;
};
