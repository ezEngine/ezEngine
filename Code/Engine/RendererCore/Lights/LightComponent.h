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

/// \brief Base class for dynamic light components.
class EZ_RENDERERCORE_DLL ezLightComponent : public ezRenderComponent
{
  EZ_DECLARE_ABSTRACT_COMPONENT_TYPE(ezLightComponent, ezRenderComponent);

  //////////////////////////////////////////////////////////////////////////
  // ezComponent

public:
  virtual void SerializeComponent(ezWorldWriter& inout_stream) const override;
  virtual void DeserializeComponent(ezWorldReader& inout_stream) override;

  //////////////////////////////////////////////////////////////////////////
  // ezLightComponent

public:
  ezLightComponent();
  ~ezLightComponent();

  void SetLightColor(ezColorGammaUB lightColor); // [ property ]
  ezColorGammaUB GetLightColor() const;          // [ property ]

  /// \brief Sets the brightness of the lightsource.
  void SetIntensity(float fIntensity); // [ property ]
  float GetIntensity() const;          // [ property ]

  /// \brief Sets whether the lightsource shall cast dynamic shadows.
  void SetCastShadows(bool bCastShadows); // [ property ]
  bool GetCastShadows() const;            // [ property ]

  /// \brief Sets the fuzziness of the shadow edges.
  void SetPenumbraSize(float fPenumbraSize); // [ property ]
  float GetPenumbraSize() const;             // [ property ]

  /// \brief Allows to tweak how dynamic shadows are applied to reduce artifacts.
  void SetSlopeBias(float fShadowBias); // [ property ]
  float GetSlopeBias() const;           // [ property ]

  /// \brief Allows to tweak how dynamic shadows are applied to reduce artifacts.
  void SetConstantBias(float fShadowBias); // [ property ]
  float GetConstantBias() const;           // [ property ]

  void OnMsgSetColor(ezMsgSetColor& ref_msg); // [ msg handler ]

  /// \brief Calculates how far a lightsource would shine given the specified range and intensity.
  ///
  /// If fRange is zero, the range needed for the given intensity is returned.
  /// Otherwise the smaller value of that and fRange is returned.
  static float CalculateEffectiveRange(float fRange, float fIntensity);

  /// \brief Calculates how large on screen (in pixels) the lightsource would be.
  static float CalculateScreenSpaceSize(const ezBoundingSphere& sphere, const ezCamera& camera);

protected:
  ezColorGammaUB m_LightColor = ezColor::White;
  float m_fIntensity = 10.0f;
  float m_fPenumbraSize = 0.1f;
  float m_fSlopeBias = 0.25f;
  float m_fConstantBias = 0.1f;
  bool m_bCastShadows = false;
};
