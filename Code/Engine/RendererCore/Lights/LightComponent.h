#pragma once

#include <RendererCore/Components/RenderComponent.h>
#include <RendererCore/Pipeline/RenderData.h>

/// \brief Base class for light render data objects.
class EZ_RENDERERCORE_DLL ezLightRenderData : public ezRenderData
{
  EZ_ADD_DYNAMIC_REFLECTION(ezLightRenderData, ezRenderData);

public:
  ezColor m_LightColor;
  float m_fIntensity;
  ezUInt32 m_uiShadowDataOffset;
};

/// \brief Base class for all ez light components containing shared properties
class EZ_RENDERERCORE_DLL ezLightComponent : public ezRenderComponent
{
  EZ_DECLARE_ABSTRACT_COMPONENT_TYPE(ezLightComponent, ezRenderComponent);

public:
  ezLightComponent();
  ~ezLightComponent();

  void SetLightColor(ezColorGammaUB LightColor);
  ezColorGammaUB GetLightColor() const;

  void SetIntensity(float fIntensity);
  float GetIntensity() const;

  void SetCastShadows(bool bCastShadows);
  bool GetCastShadows() const;

  void SetPenumbraSize(float fPenumbraSize);
  float GetPenumbraSize() const;

  void SetSlopeBias(float fShadowBias);
  float GetSlopeBias() const;

  void SetConstantBias(float fShadowBias);
  float GetConstantBias() const;

  virtual void SerializeComponent(ezWorldWriter& stream) const override;
  virtual void DeserializeComponent(ezWorldReader& stream) override;

  static float CalculateEffectiveRange(float fRange, float fIntensity);
  static float CalculateScreenSpaceSize(const ezBoundingSphere& sphere, const ezCamera& camera);

protected:

  ezColorGammaUB m_LightColor;
  float m_fIntensity;
  float m_fPenumbraSize;
  float m_fSlopeBias;
  float m_fConstantBias;
  bool m_bCastShadows;

};
