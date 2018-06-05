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
  virtual void RegisterInputActions() override;
  virtual void OnProcessInput() override;
  virtual void OnBeforeWorldUpdate() override;

private:
  void UpdateCamera();
  void DisplayMainUI();

  float m_fCameraZoom = 10.0f;
};
