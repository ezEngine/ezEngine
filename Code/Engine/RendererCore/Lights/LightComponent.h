#pragma once

#include <RendererCore/Components/RenderComponent.h>
#include <RendererCore/Pipeline/RenderData.h>

struct ezMsgSetColor;

/// \brief Base class for light render data objects.
class EZ_RENDERERCORE_DLL ezLightRenderData : public ezRenderData
{
  EZ_ADD_DYNAMIC_REFLECTION(ezLightRenderData, ezRenderData);

public:
  void FillBatchIdAndSortingKey(float fScreenSpaceSize);

  ezColor m_LightColor;
  float m_fIntensity;
  ezUInt32 m_uiShadowDataOffset;
};

/// \brief Base class for all ez light components containing shared properties
class EZ_RENDERERCORE_DLL ezLightComponent : public ezRenderComponent
{
  EZ_DECLARE_ABSTRACT_COMPONENT_TYPE(ezLightComponent, ezRenderComponent);

  //////////////////////////////////////////////////////////////////////////
  // ezComponent

public:
  virtual void SerializeComponent(ezWorldWriter& stream) const override;
  virtual void DeserializeComponent(ezWorldReader& stream) override;


  //////////////////////////////////////////////////////////////////////////
  // ezLightComponent

public:
  ezLightComponent();
  ~ezLightComponent();

  void SetLightColor(ezColorGammaUB LightColor); // [ property ]
  ezColorGammaUB GetLightColor() const;          // [ property ]

  void SetIntensity(float fIntensity); // [ property ]
  float GetIntensity() const;          // [ property ]

  void SetCastShadows(bool bCastShadows); // [ property ]
  bool GetCastShadows() const;            // [ property ]

  void SetPenumbraSize(float fPenumbraSize); // [ property ]
  float GetPenumbraSize() const;             // [ property ]

  void SetSlopeBias(float fShadowBias); // [ property ]
  float GetSlopeBias() const;           // [ property ]

  void SetConstantBias(float fShadowBias); // [ property ]
  float GetConstantBias() const;           // [ property ]

  void OnMsgSetColor(ezMsgSetColor& msg); // [ msg handler ]

  static float CalculateEffectiveRange(float fRange, float fIntensity);
  static float CalculateScreenSpaceSize(const ezBoundingSphere& sphere, const ezCamera& camera);

protected:
  ezColorGammaUB m_LightColor = ezColor::White;
  float m_fIntensity = 10.0f;
  float m_fPenumbraSize = 0.1f;
  float m_fSlopeBias = 0.25f;
  float m_fConstantBias = 0.1f;
  bool m_bCastShadows = false;
};
