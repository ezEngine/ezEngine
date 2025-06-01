#pragma once

#include <Foundation/Math/Float16.h>
#include <RendererCore/Lights/LightComponent.h>
#include <RendererCore/Material/MaterialResource.h>
#include <RendererCore/Textures/Texture2DResource.h>

using ezSpotLightComponentManager = ezComponentManager<class ezSpotLightComponent, ezBlockStorageType::Compact>;

/// \brief The render data object for spot lights.
class EZ_RENDERERCORE_DLL ezSpotLightRenderData : public ezLightRenderData
{
  EZ_ADD_DYNAMIC_REFLECTION(ezSpotLightRenderData, ezLightRenderData);

public:
  float m_fRange;
  ezAngle m_InnerSpotAngle;
  ezAngle m_OuterSpotAngle;
  ezDecalId m_CookieId;
};

/// \brief Adds a spotlight to the scene, optionally casting shadows.
class EZ_RENDERERCORE_DLL ezSpotLightComponent : public ezLightComponent
{
  EZ_DECLARE_COMPONENT_TYPE(ezSpotLightComponent, ezLightComponent, ezSpotLightComponentManager);

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
  // ezSpotLightComponent

public:
  ezSpotLightComponent();
  ~ezSpotLightComponent();

  /// \brief Sets the radius (or length of the cone) of the lightsource. If zero, it is automatically determined from the intensity.
  void SetRange(float fRange); // [ property ]
  float GetRange() const;      // [ property ]

  /// \brief Returns the final radius of the lightsource.
  float GetEffectiveRange() const;

  /// \brief Sets the radius that is used to determine when to fade out shadows. If zero the radius of the lightsource is used.
  void SetShadowFadeOutRange(float fRange); // [ property ]
  float GetShadowFadeOutRange() const;      // [ property ]

  /// \brief Sets the inner angle where the spotlight has equal brightness.
  void SetInnerSpotAngle(ezAngle spotAngle); // [ property ]
  ezAngle GetInnerSpotAngle() const;         // [ property ]

  /// \brief Sets the outer angle of the spotlight's cone. The light will fade out between the inner and outer angle.
  void SetOuterSpotAngle(ezAngle spotAngle);                               // [ property ]
  ezAngle GetOuterSpotAngle() const;                                       // [ property ]

  void SetCookie(const ezTexture2DResourceHandle& hCookie);                // [ property ]
  const ezTexture2DResourceHandle& GetCookie() const { return m_hCookie; } // [ property ]

  // adds SetCookieFile() and GetCookieFile() for convenience
  EZ_ADD_RESOURCEHANDLE_ACCESSORS_WITH_SETTER(Cookie, m_hCookie, SetCookie);

  void SetMaterial(const ezMaterialResourceHandle& hMaterial);                // [ property ]
  const ezMaterialResourceHandle& GetMaterial() const { return m_hMaterial; } // [ property ]

  // adds SetMaterialFile() and GetMaterialFile() for convenience
  EZ_ADD_RESOURCEHANDLE_ACCESSORS_WITH_SETTER(Material, m_hMaterial, SetMaterial);

  void SetMaterialResolution(ezUInt32 uiResolution);                                                     // [ property ]
  ezUInt32 GetMaterialResolution() const { return m_uiMaterialResolution; }                              // [ property ]

  void SetMaterialUpdateInterval(ezTime updateInterval);                                                 // [ property ]
  ezTime GetMaterialUpdateInterval() const { return ezTime::MakeFromSeconds(m_MaterialUpdateInterval); } // [ property ]

protected:
  void OnMsgExtractRenderData(ezMsgExtractRenderData& msg) const;
  ezBoundingSphere CalculateBoundingSphere(const ezTransform& t, float fRange) const;

  float m_fRange = 0.0f;
  float m_fEffectiveRange = 0.0f;
  float m_fShadowFadeOutRange = 0.0f;

  ezAngle m_InnerSpotAngle = ezAngle::MakeFromDegree(15.0f);
  ezAngle m_OuterSpotAngle = ezAngle::MakeFromDegree(30.0f);

  ezUInt16 m_uiMaterialResolution = 512;
  ezFloat16 m_MaterialUpdateInterval = 0.0f;
  ezMaterialResourceHandle m_hMaterial;

  ezTexture2DResourceHandle m_hCookie;
};

/// \brief A special visualizer attribute for spot lights
class EZ_RENDERERCORE_DLL ezSpotLightVisualizerAttribute : public ezVisualizerAttribute
{
  EZ_ADD_DYNAMIC_REFLECTION(ezSpotLightVisualizerAttribute, ezVisualizerAttribute);

public:
  ezSpotLightVisualizerAttribute();
  ezSpotLightVisualizerAttribute(
    const char* szAngleProperty, const char* szRangeProperty, const char* szIntensityProperty, const char* szColorProperty);

  const ezUntrackedString& GetAngleProperty() const { return m_sProperty1; }
  const ezUntrackedString& GetRangeProperty() const { return m_sProperty2; }
  const ezUntrackedString& GetIntensityProperty() const { return m_sProperty3; }
  const ezUntrackedString& GetColorProperty() const { return m_sProperty4; }
};
