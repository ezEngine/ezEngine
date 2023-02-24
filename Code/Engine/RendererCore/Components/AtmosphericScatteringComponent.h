#pragma once

#include <Core/World/World.h>
#include <Foundation/Math/Angle.h>
#include <RendererCore/Components/RenderComponent.h>

struct ezAtmosphereConstants;
struct ezMsgExtractRenderData;

using ezAtmosphericScatteringComponentManager = ezComponentManager<class ezAtmosphericScatteringComponent, ezBlockStorageType::Compact>;

/// Renders a procedural sky from a physically based model of a planetary atmosphere (Rayleigh and Mie scattering, ozone absorption, multiple scattering).
///
/// Like ezSkyBoxComponent this is rendered in the Sky render data category, as a fullscreen triangle at the far plane.
///
/// The sun direction is taken from the owner's orientation: the light travels along the owner's positive X axis, the same
/// convention as ezDirectionalLightComponent. Placing both components on the same object keeps the sky and the scene lighting
/// in sync. The color and intensity of the directional light are not adjusted automatically.
///
/// The scene's world-space Z axis is treated as 'up'. The camera's height above the planet surface is the camera's Z
/// position minus the ground height property. Heights above the top of the atmosphere are clamped, so the sky can't be
/// viewed from space. The ground below the horizon is rendered as a uniform diffuse surface (see SetGroundAlbedo()), which is
/// only visible where no geometry covers it.
///
/// For ambient light from the atmosphere, add an ezSkyLightComponent and give this object the tag 'SkyLight', which the sky
/// light captures by default. A sky light in 'Static' mode only captures once, so with a moving sun it has to use 'Dynamic' mode.
///
/// The scattering is precomputed into lookup tables on the GPU (see ezAtmosphericScatteringLUTs), rendering the sky
/// itself is a single texture lookup per pixel. The tables are recomputed whenever the sun direction or any of the
/// atmosphere properties change, which costs considerably more than rendering the sky. Rotating the sun every frame
/// (e.g. for a day-night cycle) is supported, but has that cost every frame.
class EZ_RENDERERCORE_DLL ezAtmosphericScatteringComponent : public ezRenderComponent
{
  EZ_DECLARE_COMPONENT_TYPE(ezAtmosphericScatteringComponent, ezRenderComponent, ezAtmosphericScatteringComponentManager);

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
  // ezAtmosphericScatteringComponent

public:
  ezAtmosphericScatteringComponent();
  ~ezAtmosphericScatteringComponent();

  /// Radius of the planet in kilometers.
  void SetPlanetRadius(float fRadius); // [ property ]
  float GetPlanetRadius() const { return m_fPlanetRadius; } // [ property ]

  /// Thickness of the atmosphere above the planet surface in kilometers. Scattering above this height is ignored.
  void SetAtmosphereHeight(float fHeight); // [ property ]
  float GetAtmosphereHeight() const { return m_fAtmosphereHeight; } // [ property ]

  /// Diffuse reflectance of the planet surface. Affects the color of the ground below the horizon and, through multiple
  /// scattering, the brightness of the sky close to the horizon.
  void SetGroundAlbedo(const ezColor& color);                       // [ property ]
  const ezColor& GetGroundAlbedo() const { return m_GroundAlbedo; } // [ property ]

  /// World-space Z coordinate (in meters) that corresponds to the planet surface.
  void SetGroundHeight(float fHeight); // [ property ]
  float GetGroundHeight() const { return m_fGroundHeight; } // [ property ]

  /// Rayleigh scattering coefficients per color channel, in 1/Mm (per 1000 km) at sea level. Mainly determines the sky color.
  void SetRayleighScattering(const ezVec3& vCoefficients); // [ property ]
  const ezVec3& GetRayleighScattering() const { return m_vRayleighScattering; } // [ property ]

  /// Height in kilometers at which the Rayleigh density has dropped to 1/e of its sea level value.
  void SetRayleighScaleHeight(float fHeight); // [ property ]
  float GetRayleighScaleHeight() const { return m_fRayleighScaleHeight; } // [ property ]

  /// Mie scattering coefficient in 1/Mm at sea level. Mainly determines the haze and the halo around the sun.
  void SetMieScattering(float fCoefficient); // [ property ]
  float GetMieScattering() const { return m_fMieScattering; } // [ property ]

  /// Height in kilometers at which the Mie density has dropped to 1/e of its sea level value.
  void SetMieScaleHeight(float fHeight); // [ property ]
  float GetMieScaleHeight() const { return m_fMieScaleHeight; } // [ property ]

  /// Henyey-Greenstein asymmetry of Mie scattering. Values close to 1 give a small, bright halo around the sun, 0 scatters uniformly.
  void SetMieAnisotropy(float fAnisotropy); // [ property ]
  float GetMieAnisotropy() const { return m_fMieAnisotropy; } // [ property ]

  /// Ozone absorption coefficients per color channel in 1/Mm. The ozone layer is centered at 30 km height.
  void SetOzoneAbsorption(const ezVec3& vCoefficients); // [ property ]
  const ezVec3& GetOzoneAbsorption() const { return m_vOzoneAbsorption; } // [ property ]

  /// Brightness of the sun light that gets scattered. Has no physical unit, needs to be balanced against the exposure.
  void SetSunIntensity(float fIntensity); // [ property ]
  float GetSunIntensity() const { return m_fSunIntensity; } // [ property ]

  /// Angular diameter of the visible sun disc. Zero disables the disc.
  void SetSunDiscSize(ezAngle size); // [ property ]
  ezAngle GetSunDiscSize() const { return m_SunDiscSize; } // [ property ]

  /// Brightness of the sun disc before it gets attenuated by the atmosphere.
  void SetSunDiscIntensity(float fIntensity); // [ property ]
  float GetSunDiscIntensity() const { return m_fSunDiscIntensity; } // [ property ]

  /// Whether the scene fog is applied on top of the sky, at the virtual distance.
  void SetUseFog(bool bUseFog); // [ property ]
  bool GetUseFog() const { return m_bUseFog; } // [ property ]

  /// The distance in meters at which the sky is treated to be for the fog computation.
  void SetVirtualDistance(float fVirtualDistance); // [ property ]
  float GetVirtualDistance() const { return m_fVirtualDistance; } // [ property ]

private:
  void OnMsgExtractRenderData(ezMsgExtractRenderData& msg) const;
  void FillConstants(ezAtmosphereConstants& out_constants) const;

  float m_fPlanetRadius = 6371.0f;
  float m_fAtmosphereHeight = 100.0f;
  float m_fGroundHeight = 0.0f;
  ezColor m_GroundAlbedo = ezColor(0.3f, 0.3f, 0.3f);
  ezVec3 m_vRayleighScattering = ezVec3(5.5f, 13.0f, 22.4f);
  float m_fRayleighScaleHeight = 8.0f;
  float m_fMieScattering = 21.0f;
  float m_fMieScaleHeight = 1.2f;
  float m_fMieAnisotropy = 0.7f;
  ezVec3 m_vOzoneAbsorption = ezVec3(20.4f, 49.7f, 1.95f);
  float m_fSunIntensity = 40.0f;
  ezAngle m_SunDiscSize = ezAngle::MakeFromDegree(1.0f);
  float m_fSunDiscIntensity = 3.0f;
  bool m_bUseFog = true;
  float m_fVirtualDistance = 1000.0f;
};
