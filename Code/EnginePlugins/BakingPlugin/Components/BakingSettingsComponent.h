#pragma once

#include <BakingPlugin/BakingPluginDLL.h>
#include <Core/World/SettingsComponent.h>
#include <Core/World/SettingsComponentManager.h>

struct ezMsgUpdateLocalBounds;

typedef ezSettingsComponentManager<class ezBakingSettingsComponent> ezBakingSettingsComponentManager;

class EZ_BAKINGPLUGIN_DLL ezBakingSettingsComponent : public ezSettingsComponent
{
  EZ_DECLARE_COMPONENT_TYPE(ezBakingSettingsComponent, ezSettingsComponent, ezBakingSettingsComponentManager);

public:
  ezBakingSettingsComponent();
  ~ezBakingSettingsComponent();

  virtual void OnActivated() override;
  virtual void OnDeactivated() override;

  void OnUpdateLocalBounds(ezMsgUpdateLocalBounds& msg);

  virtual void SerializeComponent(ezWorldWriter& stream) const override;
  virtual void DeserializeComponent(ezWorldReader& stream) override;

private:
  bool m_bShowDebugOverlay = false;
  bool m_bShowDebugProbes = false;
};
