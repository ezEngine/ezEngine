#pragma once

#include <Core/World/SettingsComponent.h>
#include <Core/World/SettingsComponentManager.h>
#include <RendererCore/RendererCoreDLL.h>

struct ezMsgUpdateLocalBounds;

using ezAmbientLightComponentManager = ezSettingsComponentManager<class ezAmbientLightComponent>;

class EZ_RENDERERCORE_DLL ezAmbientLightComponent : public ezSettingsComponent
{
  EZ_DECLARE_COMPONENT_TYPE(ezAmbientLightComponent, ezSettingsComponent, ezAmbientLightComponentManager);

  //////////////////////////////////////////////////////////////////////////
  // ezComponent

public:
  virtual void SerializeComponent(ezWorldWriter& inout_stream) const override;
  virtual void DeserializeComponent(ezWorldReader& inout_stream) override;

protected:
  virtual void Deinitialize() override;
  virtual void OnActivated() override;
  virtual void OnDeactivated() override;


  //////////////////////////////////////////////////////////////////////////
  // ezAmbientLightComponent

public:
  ezAmbientLightComponent();
  ~ezAmbientLightComponent();

  void SetTopColor(ezColorGammaUB color);    // [ property ]
  ezColorGammaUB GetTopColor() const;        // [ property ]

  void SetBottomColor(ezColorGammaUB color); // [ property ]
  ezColorGammaUB GetBottomColor() const;     // [ property ]

  void SetIntensity(float fIntensity);       // [ property ]
  float GetIntensity() const;                // [ property ]

private:
  void OnUpdateLocalBounds(ezMsgUpdateLocalBounds& msg);
  void UpdateSkyIrradiance();

  ezColorGammaUB m_TopColor = ezColor(0.2f, 0.2f, 0.3f);
  ezColorGammaUB m_BottomColor = ezColor(0.1f, 0.1f, 0.15f);
  float m_fIntensity = 1.0f;
};
