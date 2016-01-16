#pragma once

#include <Core/World/World.h>
#include <RendererCore/Pipeline/Declarations.h>

class EZ_RENDERERCORE_DLL ezLightComponent : public ezComponent
{
  EZ_ADD_DYNAMIC_REFLECTION(ezLightComponent, ezComponent);

public:
  ezLightComponent();

  void SetLightColor(ezColor LightColor);
  ezColor GetLightColor() const;

  void SetIntensity(float fIntensity);
  float GetIntensity() const;

  void SetCastShadows(bool bCastShadows);
  bool GetCastShadows();

  virtual void SerializeComponent(ezWorldWriter& stream) const override;
  virtual void DeserializeComponent(ezWorldReader& stream) override;

protected:

  ezColor m_LightColor;
  float m_fIntensity;
  bool m_bCastShadows;

};
