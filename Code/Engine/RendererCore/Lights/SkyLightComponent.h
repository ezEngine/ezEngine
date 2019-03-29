#pragma once

#include <Core/World/SettingsComponent.h>
#include <Core/World/SettingsComponentManager.h>
#include <RendererCore/Lights/Implementation/ReflectionProbeData.h>

struct ezMsgUpdateLocalBounds;
struct ezMsgExtractRenderData;

typedef ezSettingsComponentManager<class ezSkyLightComponent> ezSkyLightComponentManager;

class EZ_RENDERERCORE_DLL ezSkyLightComponent : public ezSettingsComponent
{
  EZ_DECLARE_COMPONENT_TYPE(ezSkyLightComponent, ezSettingsComponent, ezSkyLightComponentManager);

public:
  ezSkyLightComponent();
  ~ezSkyLightComponent();

  virtual void OnActivated() override;
  virtual void OnDeactivated() override;

  void SetIntensity(float fIntensity);
  float GetIntensity() const;

  void SetSaturation(float fSaturation);
  float GetSaturation() const;

  void OnUpdateLocalBounds(ezMsgUpdateLocalBounds& msg);
  void OnExtractRenderData(ezMsgExtractRenderData& msg) const;

  virtual void SerializeComponent(ezWorldWriter& stream) const override;
  virtual void DeserializeComponent(ezWorldReader& stream) override;

private:
  ezReflectionProbeData m_ReflectionProbeData;
};
