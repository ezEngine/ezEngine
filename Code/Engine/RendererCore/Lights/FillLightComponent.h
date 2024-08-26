#pragma once

#include <RendererCore/Components/RenderComponent.h>
#include <RendererCore/Pipeline/RenderData.h>

struct ezMsgSetColor;

struct ezFillLightMode
{
  using StorageType = ezUInt8;

  enum Enum
  {
    Additive,
    ModulateIndirect,

    Default = Additive
  };
};

EZ_DECLARE_REFLECTABLE_TYPE(EZ_RENDERERCORE_DLL, ezFillLightMode);

/// \brief The render data object for fill lights.
class EZ_RENDERERCORE_DLL ezFillLightRenderData : public ezRenderData
{
  EZ_ADD_DYNAMIC_REFLECTION(ezFillLightRenderData, ezRenderData);

public:
  void FillBatchIdAndSortingKey(float fScreenSpaceSize);

  ezColorLinearUB m_LightColor;
  ezEnum<ezFillLightMode> m_LightMode;
  float m_fIntensity;
  float m_fRange;
  float m_fFalloffExponent;
  float m_fDirectionality;
};

using ezFillLightComponentManager = ezComponentManager<class ezFillLightComponent, ezBlockStorageType::Compact>;

/// \brief Adds a fill light to the scene. This can be used to simulate bounced light or to light up dark areas.
/// It can also be used to modulate the indirect lighting.
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

  /// \brief In Additive mode the fill light adds light to scene like a regular light source.
  /// In ModulateIndirect mode it acts as a multiplier to the indirect light.
  void SetLightMode(ezEnum<ezFillLightMode> mode);                     // [ property ]
  ezEnum<ezFillLightMode> GetLightMode() const { return m_LightMode; } // [ property ]

  /// \brief Used to enable kelvin color values. This is a physical representation of light color using.
  /// for more detail: https://wikipedia.org/wiki/Color_temperature
  void SetUsingColorTemperature(bool bUseColorTemperature);                // [ property ]
  bool GetUsingColorTemperature() const { return m_bUseColorTemperature; } // [ property ]

  void SetTemperature(ezUInt32 uiTemperature);                             // [ property ]
  ezUInt32 GetTemperature() const { return m_uiTemperature; }              // [ property ]

  void SetLightColor(ezColorGammaUB lightColor);                           // [ property ]
  ezColorGammaUB GetLightColor() const { return m_LightColor; }            // [ property ]

  ezColorGammaUB GetEffectiveColor() const;

  /// \brief In Additive mode this controls the brightness of the light source.
  /// In ModulateIndirect mode light color times intensity is multiplied with the indirect light,
  /// thus values below 1 darken the indirect light, values above 1 brighten the indirect light.
  void SetIntensity(float fIntensity);                // [ property ]
  float GetIntensity() const { return m_fIntensity; } // [ property ]

  /// \brief Sets the radius of the light source.
  void SetRange(float fRange);                                    // [ property ]
  float GetRange() const { return m_fRange; }                     // [ property ]

  void SetFalloffExponent(float fFalloffExponent);                // [ property ]
  float GetFalloffExponent() const { return m_fFalloffExponent; } // [ property ]

  /// \brief Controls how much the light wraps to the backside of lit objects.
  /// A directionality of 1 means no light will wrap to the backside and
  /// with a directionality of 0 light will equaly lit front and backsides.
  void SetDirectionality(float fDirectionality);                // [ property ]
  float GetDirectionality() const { return m_fDirectionality; } // [ property ]

protected:
  void OnMsgSetColor(ezMsgSetColor& ref_msg);
  void OnMsgExtractRenderData(ezMsgExtractRenderData& msg) const;

  ezColorGammaUB m_LightColor = ezColor::White;
  ezUInt32 m_uiTemperature = 6550;
  float m_fIntensity = 10.0f;
  float m_fRange = 5.0f;
  float m_fFalloffExponent = 1.0f;
  float m_fDirectionality = 1.0f;
  ezEnum<ezFillLightMode> m_LightMode;
  bool m_bUseColorTemperature = false;
};
