#pragma once

#include <RTSPlugin/GameMode/GameMode.h>
#include <RmlUiPlugin\Components\RmlUiCanvas2DComponent.h>

class RtsSettingsMenuMode : public RtsGameMode
{
public:
  RtsSettingsMenuMode();
  ~RtsSettingsMenuMode();

protected:
  virtual void OnActivateMode() override;
  virtual void OnDeactivateMode() override;
  virtual void OnProcessInput(const RtsMouseInputState& MouseInput, bool bUiWantsInput) override;
  virtual void OnBeforeWorldUpdate() override;

  ezTypedComponentHandle<ezRmlUiCanvas2DComponent> m_hSettingsMenu;
  ezUInt32 m_uiButtonClickCount = 0;
};
