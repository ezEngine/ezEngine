#pragma once

#include <RendererCore/Components/RenderComponent.h>
#include <RendererCore/Pipeline/RenderData.h>

struct ezMsgSetColor;

using ezFillLightComponentManager = ezComponentManager<class ezFillLightComponent, ezBlockStorageType::Compact>;

/// \brief The render data object for fill lights.
class EZ_RENDERERCORE_DLL ezFillLightRenderData : public ezRenderData
{
  EZ_ADD_DYNAMIC_REFLECTION(ezFillLightRenderData, ezRenderData);

public:
  void FillBatchIdAndSortingKey(float fScreenSpaceSize);

  ezColorLinearUB m_LightColor;
  float m_fIntensity;
  float m_fRange;
  float m_fFalloffExponent;
  float m_fDirectionality;
};

/// \brief Adds a fill light to the scene.
class EZ_RENDERERCORE_DLL ezFillLightComponent : public ezRenderComponent
{
  EZ_DECLARE_COMPONENT_TYPE(ezFillLightComponent, ezRenderComponent, ezFillLightComponentManager);

  //////////////////////////////////////////////////////////////////////////
  // ezComponent

public:
  virtual void SerializeComponent(ezWorldWriter& inout_stream) const override;
  virtual void DeserializeComponent(ezWorldReader& inout_stream) override;


  //////////////////////////////////////////////////////////////////////////
  // ezRenderComponent

public:
  virtual ezResult GetLocalBounds(ezBoundingBoxSphere& ref_bounds, bool& ref_bAlwaysVisible, ezMsgUpdateLocalBounds& ref_msg) override;

  //////////////////////////////////////////////////////////////////////////
  // ezFillLightComponent

public:
  ezFillLightComponent();
  ~ezFillLightComponent();

  /// \brief Used to enable kelvin color values. This is a physical representation of light color using.
  /// for more detail: https://wikipedia.org/wiki/Color_temperature
  void SetUsingColorTemperature(bool bUseColorTemperature);                // [ property ]
  bool GetUsingColorTemperature() const { return m_bUseColorTemperature; } // [ property ]

  void SetTemperature(ezUInt32 uiTemperature);                             // [ property ]
  ezUInt32 GetTemperature() const { return m_uiTemperature; }              // [ property ]

  void SetLightColor(ezColorGammaUB lightColor);                           // [ property ]
  ezColorGammaUB GetLightColor() const { return m_LightColor; }            // [ property ]

  ezColorGammaUB GetEffectiveColor() const;

  /// \brief Sets the brightness of the light source.
  void SetIntensity(float fIntensity);                // [ property ]
  float GetIntensity() const { return m_fIntensity; } // [ property ]

  /// \brief Sets the radius of the light source.
  void SetRange(float fRange);                                    // [ property ]
  float GetRange() const { return m_fRange; }                     // [ property ]

  void SetFalloffExponent(float fFalloffExponent);                // [ property ]
  float GetFalloffExponent() const { return m_fFalloffExponent; } // [ property ]

  void SetDirectionality(float fDirectionality);                  // [ property ]
  float GetDirectionality() const { return m_fDirectionality; }   // [ property ]

protected:
  void OnMsgSetColor(ezMsgSetColor& ref_msg);
  void OnMsgExtractRenderData(ezMsgExtractRenderData& msg) const;

  ezColorGammaUB m_LightColor = ezColor::White;
  ezUInt32 m_uiTemperature = 6550;
  float m_fIntensity = 10.0f;
  float m_fRange = 5.0f;
  float m_fFalloffExponent = 1.0f;
  float m_fDirectionality = 1.0f;
  bool m_bUseColorTemperature = false;
};
