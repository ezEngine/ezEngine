#pragma once

#include <Core/World/SettingsComponent.h>
#include <Core/World/SettingsComponentManager.h>
#include <RendererCore/Pipeline/RenderData.h>

class ezAmbientLightComponent;
typedef ezSettingsComponentManager<ezAmbientLightComponent> ezAmbientLightComponentManager;

/// \brief The render data object for ambient light.
class EZ_RENDERERCORE_DLL ezAmbientLightRenderData : public ezRenderData
{
  EZ_ADD_DYNAMIC_REFLECTION(ezAmbientLightRenderData, ezRenderData);

public:
  ezColor m_TopColor;
  ezColor m_BottomColor;
};

class EZ_RENDERERCORE_DLL ezAmbientLightComponent : public ezSettingsComponent
{
  EZ_DECLARE_COMPONENT_TYPE(ezAmbientLightComponent, ezSettingsComponent, ezAmbientLightComponentManager);

public:
  ezAmbientLightComponent();
  ~ezAmbientLightComponent();

  void SetTopColor(ezColor color);
  ezColor GetTopColor() const;

  void SetBottomColor(ezColor color);
  ezColor GetBottomColor() const;

  void SetIntensity(float fIntensity);
  float GetIntensity() const;

  void OnExtractRenderData(ezExtractRenderDataMessage& msg) const;

  virtual void SerializeComponent(ezWorldWriter& stream) const override;
  virtual void DeserializeComponent(ezWorldReader& stream) override;

private:

  ezColor m_TopColor;
  ezColor m_BottomColor;
  float m_fIntensity;
};
