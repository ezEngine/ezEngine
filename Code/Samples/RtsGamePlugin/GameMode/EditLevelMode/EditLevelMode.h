#pragma once

#include <RtsGamePlugin/GameMode/GameMode.h>

class ezBlackboard;

class RtsEditLevelMode : public RtsGameMode
{
public:
  RtsEditLevelMode();
  ~RtsEditLevelMode();

protected:
  virtual void OnActivateMode() override;
  virtual void OnDeactivateMode() override;
  virtual void RegisterInputActions() override;
  virtual void OnProcessInput(const RtsMouseInputState& MouseInput) override;
  virtual void OnBeforeWorldUpdate() override;

  //////////////////////////////////////////////////////////////////////////

private:
  void SetupEditUI();
  void DisplayEditUI();

  ezInt32 m_iShipType = 0;

  ezComponentHandle m_hEditUIComponent;

  ezUniquePtr<ezBlackboard> m_pBlackboard;
};
