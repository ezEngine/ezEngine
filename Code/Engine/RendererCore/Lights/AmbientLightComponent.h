#pragma once

#include <Core/World/SettingsComponent.h>
#include <Core/World/SettingsComponentManager.h>
#include <RendererCore/RendererCoreDLL.h>

struct ezMsgUpdateLocalBounds;

typedef ezSettingsComponentManager<class ezAmbientLightComponent> ezAmbientLightComponentManager;

class EZ_RENDERERCORE_DLL ezAmbientLightComponent : public ezSettingsComponent
{
  EZ_DECLARE_COMPONENT_TYPE(ezAmbientLightComponent, ezSettingsComponent, ezAmbientLightComponentManager);

public:
  ezAmbientLightComponent();
  ~ezAmbientLightComponent();

  virtual void Deinitialize() override;

  virtual void OnActivated() override;
  virtual void OnDeactivated() override;

  void SetTopColor(ezColorGammaUB color);
  ezColorGammaUB GetTopColor() const;

  void SetBottomColor(ezColorGammaUB color);
  ezColorGammaUB GetBottomColor() const;

  void SetIntensity(float fIntensity);
  float GetIntensity() const;

  void OnUpdateLocalBounds(ezMsgUpdateLocalBounds& msg);

  virtual void SerializeComponent(ezWorldWriter& stream) const override;
  virtual void DeserializeComponent(ezWorldReader& stream) override;

private:
  void UpdateSkyIrradiance();

  ezColorGammaUB m_TopColor;
  ezColorGammaUB m_BottomColor;
  float m_fIntensity;
};
