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
  void FillShadowDataOffsetAndFadeOut(ezUInt32 uiDataOffset, float fFadeOut);

  ezColorLinearUB m_LightColor;
  float m_fIntensity;
  float m_fSpecularMultiplier;
  ezUInt32 m_uiShadowDataOffsetAndFadeOut;
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

  /// \brief Used to enable kelvin color values. This is a physical representation of light color using.
  /// for more detail: https://wikipedia.org/wiki/Color_temperature
  void SetUsingColorTemperature(bool bUseColorTemperature);
  bool GetUsingColorTemperature() const;

  void SetTemperature(ezUInt32 uiTemperature);   // [ property ]
  ezUInt32 GetTemperature() const;               // [ property ]

  void SetLightColor(ezColorGammaUB lightColor); // [ property ]
  ezColorGammaUB GetLightColor() const;          // [ property ]

  ezColorGammaUB GetEffectiveColor() const;

  /// \brief Sets the brightness of the light source.
  void SetIntensity(float fIntensity);                   // [ property ]
  float GetIntensity() const;                            // [ property ]

  void SetSpecularMultiplier(float fSpecularMultiplier); // [ property ]
  float GetSpecularMultiplier() const;                   // [ property ]

  /// \brief Sets whether the light source shall cast dynamic shadows.
  void SetCastShadows(bool bCastShadows); // [ property ]
  bool GetCastShadows() const;            // [ property ]

  /// \brief Sets whether transparent objects should cast shadows (emulated through dithering).
  void SetTransparentShadows(bool bShadows); // [ property ]
  bool GetTransparentShadows() const;        // [ property ]

  /// \brief Sets the fuzziness of the shadow edges.
  void SetPenumbraSize(float fPenumbraSize); // [ property ]
  float GetPenumbraSize() const;             // [ property ]

  /// \brief Allows to tweak how dynamic shadows are applied to reduce artifacts.
  void SetSlopeBias(float fShadowBias); // [ property ]
  float GetSlopeBias() const;           // [ property ]

  /// \brief Allows to tweak how dynamic shadows are applied to reduce artifacts.
  void SetConstantBias(float fShadowBias);    // [ property ]
  float GetConstantBias() const;              // [ property ]

  void OnMsgSetColor(ezMsgSetColor& ref_msg); // [ msg handler ]

  /// \brief Calculates how far a light source would shine given the specified range and intensity.
  ///
  /// If fRange is zero, the range needed for the given intensity is returned.
  /// Otherwise the smaller value of that and fRange is returned.
  static float CalculateEffectiveRange(float fRange, float fIntensity);

  /// \brief Calculates how large on screen (relative height) the light source would be.
  static float CalculateScreenSpaceSize(const ezBoundingSphere& sphere, const ezCamera& camera);

protected:
  float CalculateShadowFadeOut(const ezBoundingSphere& sphere, float fShadowFadeOutRange, const ezCamera& camera, float& out_fShadowScreenSize) const;

#if EZ_ENABLED(EZ_COMPILE_FOR_DEVELOPMENT)
  void VisualizeScreenSpaceSize(ezViewHandle hView, const ezBoundingSphere& sphere, float fScreenSize, float fShadowScreenSize, float fShadowFadeOut) const;
#endif

  ezColorGammaUB m_LightColor = ezColor::White;
  ezUInt32 m_uiTemperature = 6550;
  float m_fIntensity = 10.0f;
  float m_fSpecularMultiplier = 1.0f;
  float m_fPenumbraSize = 0.05f;
  float m_fSlopeBias = 0.25f;
  float m_fConstantBias = 0.1f;
  bool m_bCastShadows = false;
  bool m_bTransparentShadows = false;
  bool m_bUseColorTemperature = false;
};
