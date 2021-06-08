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

  void SetReflectionProbeMode(ezEnum<ezReflectionProbeMode> mode); // [ property ]
  ezEnum<ezReflectionProbeMode> GetReflectionProbeMode() const;    // [ property ]

  void SetCubeMapFile(const char* szFile); // [ property ]
  const char* GetCubeMapFile() const;      // [ property ]

protected:
  void OnUpdateLocalBounds(ezMsgUpdateLocalBounds& msg);
  void OnMsgExtractRenderData(ezMsgExtractRenderData& msg) const;

  ezReflectionProbeData m_ReflectionProbeData;
  // Tracks if any changes where made to the settings. Reset ezReflectionPool::ExtractReflectionProbe once a filter pass is done.
  mutable bool m_bStatesDirty = true;
};
