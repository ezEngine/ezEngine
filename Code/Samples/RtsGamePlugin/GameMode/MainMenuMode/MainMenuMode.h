#pragma once

#include <RtsGamePlugin/GameMode/GameMode.h>

class RtsMainMenuMode : public RtsGameMode
{
public:
  RtsMainMenuMode();
  ~RtsMainMenuMode();

protected:
  virtual void OnActivateMode() override;
  virtual void OnDeactivateMode() override;
  virtual void OnProcessInput(const RtsMouseInputState& MouseInput) override;
  virtual void OnBeforeWorldUpdate() override;
};
