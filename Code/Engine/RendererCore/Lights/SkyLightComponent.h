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

  //////////////////////////////////////////////////////////////////////////
  // ezComponent

public:
  virtual void SerializeComponent(ezWorldWriter& stream) const override;
  virtual void DeserializeComponent(ezWorldReader& stream) override;

protected:
  virtual void OnActivated() override;
  virtual void OnDeactivated() override;


  //////////////////////////////////////////////////////////////////////////
  // ezSkyLightComponent

public:
  ezSkyLightComponent();
  ~ezSkyLightComponent();

  void SetIntensity(float fIntensity); // [ property ]
  float GetIntensity() const;          // [ property ]

  void SetSaturation(float fSaturation); // [ property ]
  float GetSaturation() const;           // [ property ]

protected:
  void OnUpdateLocalBounds(ezMsgUpdateLocalBounds& msg);
  void OnMsgExtractRenderData(ezMsgExtractRenderData& msg) const;

  ezReflectionProbeData m_ReflectionProbeData;
};
