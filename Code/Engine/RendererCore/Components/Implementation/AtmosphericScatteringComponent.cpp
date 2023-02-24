#include <RendererCore/RendererCorePCH.h>

#include <Core/Messages/SetColorMessage.h>
#include <Core/WorldSerializer/WorldReader.h>
#include <Core/WorldSerializer/WorldWriter.h>
#include <RendererCore/Components/AtmosphericScatteringComponent.h>
#include <RendererCore/Components/AtmosphericScatteringRenderer.h>
#include <RendererCore/Pipeline/RenderDataManager.h>
#include <RendererCore/Pipeline/View.h>

// clang-format off
EZ_BEGIN_STATIC_REFLECTED_ENUM(ezAtmospherePreset, 1)
  EZ_ENUM_CONSTANTS(ezAtmospherePreset::Custom, ezAtmospherePreset::Earth, ezAtmospherePreset::EarthHazy, ezAtmospherePreset::EarthClear, ezAtmospherePreset::DustyPlanet)
EZ_END_STATIC_REFLECTED_ENUM;

EZ_BEGIN_COMPONENT_TYPE(ezAtmosphericScatteringComponent, 1, ezComponentMode::Static)
{
  EZ_BEGIN_PROPERTIES
  {
    // Commonly adjusted properties come first and stay ungrouped.
    // Note: properties without a group attribute are added to the group of the preceding property, so the order matters.
    EZ_ACCESSOR_PROPERTY("SunIntensity", GetSunIntensity, SetSunIntensity)->AddAttributes(new ezClampValueAttribute(0.0f, ezVariant()), new ezDefaultValueAttribute(40.0f)),
    EZ_ACCESSOR_PROPERTY("SunDiscSize", GetSunDiscSize, SetSunDiscSize)->AddAttributes(new ezClampValueAttribute(ezAngle::MakeZero(), ezAngle::MakeFromDegree(20.0f)), new ezDefaultValueAttribute(ezAngle::MakeFromDegree(1.0f))),
    EZ_ACCESSOR_PROPERTY("SunDiscIntensity", GetSunDiscIntensity, SetSunDiscIntensity)->AddAttributes(new ezClampValueAttribute(0.0f, ezVariant()), new ezDefaultValueAttribute(3.0f)),
    EZ_ACCESSOR_PROPERTY("SetLightColor", GetSetLightColor, SetSetLightColor)->AddAttributes(new ezDefaultValueAttribute(true)),
    EZ_ACCESSOR_PROPERTY("GroundHeight", GetGroundHeight, SetGroundHeight)->AddAttributes(new ezSuffixAttribute(" m")),
    EZ_ACCESSOR_PROPERTY("UseFog", GetUseFog, SetUseFog)->AddAttributes(new ezDefaultValueAttribute(true)),
    EZ_ACCESSOR_PROPERTY("VirtualDistance", GetVirtualDistance, SetVirtualDistance)->AddAttributes(new ezClampValueAttribute(0.0f, ezVariant()), new ezDefaultValueAttribute(1000.0f), new ezSuffixAttribute(" m")),

    EZ_ENUM_ACCESSOR_PROPERTY("Preset", ezAtmospherePreset, GetPreset, SetPreset)->AddAttributes(new ezGroupAttribute("Atmosphere")),
    EZ_ACCESSOR_PROPERTY("AirDensity", GetAirDensity, SetAirDensity)->AddAttributes(new ezClampValueAttribute(0.0f, 10.0f), new ezDefaultValueAttribute(1.0f)),
    EZ_ACCESSOR_PROPERTY("Haze", GetHaze, SetHaze)->AddAttributes(new ezClampValueAttribute(0.0f, 20.0f), new ezDefaultValueAttribute(1.0f)),
    EZ_ACCESSOR_PROPERTY("GroundAlbedo", GetGroundAlbedo, SetGroundAlbedo)->AddAttributes(new ezDefaultValueAttribute(ezColor(0.3f, 0.3f, 0.3f))),

    EZ_ACCESSOR_PROPERTY("PlanetRadius", GetPlanetRadius, SetPlanetRadius)->AddAttributes(new ezGroupAttribute("Planet", "", -1.0f, true), new ezClampValueAttribute(1.0f, ezVariant()), new ezDefaultValueAttribute(6360.0f), new ezSuffixAttribute(" km")),
    EZ_ACCESSOR_PROPERTY("AtmosphereHeight", GetAtmosphereHeight, SetAtmosphereHeight)->AddAttributes(new ezClampValueAttribute(0.1f, ezVariant()), new ezDefaultValueAttribute(100.0f), new ezSuffixAttribute(" km")),

    EZ_ACCESSOR_PROPERTY("RayleighScattering", GetRayleighScattering, SetRayleighScattering)->AddAttributes(new ezGroupAttribute("Scattering", "", -1.0f, true), new ezClampValueAttribute(ezVec3(0.0f), ezVariant()), new ezDefaultValueAttribute(ezVec3(5.802f, 13.558f, 33.1f))),
    EZ_ACCESSOR_PROPERTY("RayleighScaleHeight", GetRayleighScaleHeight, SetRayleighScaleHeight)->AddAttributes(new ezClampValueAttribute(0.01f, ezVariant()), new ezDefaultValueAttribute(8.0f), new ezSuffixAttribute(" km")),
    EZ_ACCESSOR_PROPERTY("MieScattering", GetMieScattering, SetMieScattering)->AddAttributes(new ezClampValueAttribute(0.0f, ezVariant()), new ezDefaultValueAttribute(3.996f)),
    EZ_ACCESSOR_PROPERTY("MieAbsorption", GetMieAbsorption, SetMieAbsorption)->AddAttributes(new ezClampValueAttribute(0.0f, ezVariant()), new ezDefaultValueAttribute(0.444f)),
    EZ_ACCESSOR_PROPERTY("MieScaleHeight", GetMieScaleHeight, SetMieScaleHeight)->AddAttributes(new ezClampValueAttribute(0.01f, ezVariant()), new ezDefaultValueAttribute(1.2f), new ezSuffixAttribute(" km")),
    EZ_ACCESSOR_PROPERTY("MieAnisotropy", GetMieAnisotropy, SetMieAnisotropy)->AddAttributes(new ezClampValueAttribute(-0.99f, 0.99f), new ezDefaultValueAttribute(0.8f)),
    EZ_ACCESSOR_PROPERTY("OzoneAbsorption", GetOzoneAbsorption, SetOzoneAbsorption)->AddAttributes(new ezClampValueAttribute(ezVec3(0.0f), ezVariant()), new ezDefaultValueAttribute(ezVec3(0.65f, 1.881f, 0.085f))),
  }
  EZ_END_PROPERTIES;
  EZ_BEGIN_ATTRIBUTES
  {
    new ezCategoryAttribute("Rendering"),
    new ezDirectionVisualizerAttribute(ezBasisAxis::PositiveX, 1.0f, ezColor::Yellow),
  }
  EZ_END_ATTRIBUTES;
  EZ_BEGIN_MESSAGEHANDLERS
  {
    EZ_MESSAGE_HANDLER(ezMsgExtractRenderData, OnMsgExtractRenderData),
  }
  EZ_END_MESSAGEHANDLERS;
}
EZ_END_COMPONENT_TYPE;
// clang-format on

namespace
{
  struct ezAtmospherePresetValues
  {
    float m_fPlanetRadius;
    float m_fAtmosphereHeight;
    ezVec3 m_vRayleighScattering;
    float m_fRayleighScaleHeight;
    float m_fMieScattering;
    float m_fMieAbsorption;
    float m_fMieScaleHeight;
    float m_fMieAnisotropy;
    ezVec3 m_vOzoneAbsorption;
  };

  // Indexed by ezAtmospherePreset::Enum - 1, 'Custom' has no entry.
  // Units match the component properties: km for distances, 1/Mm for coefficients.
  static const ezAtmospherePresetValues s_Presets[] = {
    // Earth: same as the property defaults, Hillaire 2020 and the defaults of Unreal Engine's sky atmosphere
    {6360.0f, 100.0f, ezVec3(5.802f, 13.558f, 33.1f), 8.0f, 3.996f, 0.444f, 1.2f, 0.8f, ezVec3(0.65f, 1.881f, 0.085f)},
    // EarthHazy: twenty times the aerosols, mixed higher up, less forward scattering
    {6360.0f, 100.0f, ezVec3(5.802f, 13.558f, 33.1f), 8.0f, 80.0f, 8.9f, 2.0f, 0.7f, ezVec3(0.65f, 1.881f, 0.085f)},
    // EarthClear: less than half the aerosols, more forward scattering
    {6360.0f, 100.0f, ezVec3(5.802f, 13.558f, 33.1f), 8.0f, 1.5f, 0.167f, 1.2f, 0.85f, ezVec3(0.65f, 1.881f, 0.085f)},
    // DustyPlanet: Mars-sized, the Rayleigh colors are inverted to fake the dust tint, dust mixed up to 6 km, no ozone
    {3390.0f, 80.0f, ezVec3(20.0f, 10.0f, 4.0f), 11.0f, 6.0f, 0.0f, 6.0f, 0.65f, ezVec3(0.0f)},
  };

  static_assert(EZ_ARRAY_SIZE(s_Presets) == ezAtmospherePreset::DustyPlanet);
} // namespace

ezAtmosphericScatteringComponent::ezAtmosphericScatteringComponent() = default;
ezAtmosphericScatteringComponent::~ezAtmosphericScatteringComponent() = default;

ezResult ezAtmosphericScatteringComponent::GetLocalBounds(ezBoundingBoxSphere& ref_bounds, bool& ref_bAlwaysVisible, ezMsgUpdateLocalBounds& ref_msg)
{
  ref_bAlwaysVisible = true;
  return EZ_SUCCESS;
}

void ezAtmosphericScatteringComponent::OnMsgExtractRenderData(ezMsgExtractRenderData& msg) const
{
  // Don't extract sky render data for selection or in orthographic views.
  if (msg.m_OverrideCategory != ezInvalidRenderDataCategory || msg.m_pView->GetCamera()->IsOrthographic())
    return;

  ezAtmosphericScatteringRenderData* pRenderData = msg.m_pRenderDataManager->CreateRenderDataForThisFrame<ezAtmosphericScatteringRenderData>(GetOwner());
  FillConstants(pRenderData->m_Constants);

  // Requesting the tables every frame keeps them alive and schedules an update when the sun or the parameters changed.
  // Therefore the render data must not be cached.
  const ezAtmosphericScatteringLUTs::LUTs luts = ezAtmosphericScatteringLUTs::Request(reinterpret_cast<ezUInt64>(this), pRenderData->m_Constants);
  pRenderData->m_hTransmittanceLUT = luts.m_hTransmittance;
  pRenderData->m_hSkyViewLUT = luts.m_hSkyView;

  msg.AddRenderData(pRenderData, ezDefaultRenderDataCategories::Sky, ezRenderData::Caching::Never);
  msg.AddDependency(luts.m_hTransmittance, ezDefaultRenderDataCategories::Sky, ezGALResourceState::ShaderResource);
  msg.AddDependency(luts.m_hSkyView, ezDefaultRenderDataCategories::Sky, ezGALResourceState::ShaderResource);
}

void ezAtmosphericScatteringComponent::Update()
{
  if (!m_bSetLightColor)
    return;

  // Sent every frame, so that the light gets the color back after its properties were reset, e.g. by the editor.
  // Light components only invalidate their render data when the color actually changes.
  ezMsgSetColor msg;
  msg.m_Color = ComputeSunLightColor();
  msg.m_Mode = ezSetColorMode::SetRGB;
  GetOwner()->SendMessage(msg);
}

ezColor ezAtmosphericScatteringComponent::ComputeSunLightColor() const
{
  // Mirrors AtmosphereSampleMedium() in AtmosphereCommon.h. All distances in km.
  ezAtmosphereConstants c;
  FillConstants(c);

  // Fade out while the sun disc sinks below the horizon, instead of switching off when its center does.
  // Without a visible disc, the size of the real sun is used.
  const float fSinDiscRadius = ezMath::Sin(ezMath::Max(m_SunDiscSize, ezAngle::MakeFromDegree(0.5f)) * 0.5f);
  const float fVisible = ezMath::Saturate((c.SunDirection.z + fSinDiscRadius) / (2.0f * fSinDiscRadius));
  if (fVisible <= 0.0f)
    return ezColor::Black;

  ezVec3 vDir = c.SunDirection;
  vDir.z = ezMath::Max(vDir.z, 0.0f);
  vDir.NormalizeIfNotZero(ezVec3(1.0f, 0.0f, 0.0f)).IgnoreResult();

  const ezVec3 vStart(0.0f, 0.0f, c.PlanetRadius + 0.001f);

  // distance to the top of the atmosphere
  const float b = vDir.Dot(vStart);
  const float cc = vStart.GetLengthSquared() - c.AtmosphereRadius * c.AtmosphereRadius;
  const float fTopDistance = -b + ezMath::Sqrt(ezMath::Max(b * b - cc, 0.0f));

  // Quadratically distributed samples, the density is highest at the start of the ray.
  constexpr ezUInt32 uiNumSteps = 64;
  ezVec3 vOpticalDepth = ezVec3::MakeZero();
  float fPrevT = 0.0f;

  for (ezUInt32 i = 1; i <= uiNumSteps; ++i)
  {
    const float fNorm = (float)i / uiNumSteps;
    const float t = fTopDistance * fNorm * fNorm;
    const float fMidT = 0.5f * (fPrevT + t);
    const float fHeight = ezMath::Max((vStart + vDir * fMidT).GetLength() - c.PlanetRadius, 0.0f);

    const float fDensityRayleigh = ezMath::Exp(-fHeight / c.RayleighScaleHeight);
    const float fDensityMie = ezMath::Exp(-fHeight / c.MieScaleHeight);
    const float fDensityOzone = ezMath::Saturate(1.0f - ezMath::Abs(fHeight - 25.0f) / 15.0f); // ATMOSPHERE_OZONE_HEIGHT, ATMOSPHERE_OZONE_HALF_WIDTH

    const ezVec3 vExtinction = c.RayleighScattering * fDensityRayleigh + ezVec3((c.MieScattering + c.MieAbsorption) * fDensityMie) + c.OzoneAbsorption * fDensityOzone;
    vOpticalDepth += vExtinction * (t - fPrevT);
    fPrevT = t;
  }

  return ezColor(ezMath::Exp(-vOpticalDepth.x), ezMath::Exp(-vOpticalDepth.y), ezMath::Exp(-vOpticalDepth.z)) * fVisible;
}

void ezAtmosphericScatteringComponent::FillConstants(ezAtmosphereConstants& out_constants) const
{
  // The shaders work in kilometers to keep the float precision of the ray-sphere intersections acceptable.
  // Scattering coefficients are exposed per Mm (1000 km) to get readable numbers.
  const float fPerMmToPerKm = 1e-3f;

  // light travels along +X, so the direction towards the sun is -X
  ezVec3 vSunDir = GetOwner()->GetGlobalRotation() * ezVec3(-1.0f, 0.0f, 0.0f);
  vSunDir.NormalizeIfNotZero(ezVec3(0.0f, 0.0f, 1.0f)).IgnoreResult();

  ezAtmospherePresetValues p = {m_fPlanetRadius, m_fAtmosphereHeight, m_vRayleighScattering, m_fRayleighScaleHeight, m_fMieScattering, m_fMieAbsorption, m_fMieScaleHeight, m_fMieAnisotropy, m_vOzoneAbsorption};

  if (m_Preset != ezAtmospherePreset::Custom && m_Preset.GetValue() <= ezAtmospherePreset::DustyPlanet)
  {
    p = s_Presets[m_Preset.GetValue() - 1];
  }

  out_constants = {};
  out_constants.RayleighScattering = p.m_vRayleighScattering * (m_fAirDensity * fPerMmToPerKm);
  out_constants.PlanetRadius = p.m_fPlanetRadius;
  out_constants.OzoneAbsorption = p.m_vOzoneAbsorption * (m_fAirDensity * fPerMmToPerKm);
  out_constants.AtmosphereRadius = p.m_fPlanetRadius + p.m_fAtmosphereHeight;
  out_constants.SunDirection = vSunDir;
  out_constants.MieScattering = p.m_fMieScattering * (m_fHaze * fPerMmToPerKm);
  out_constants.MieAbsorption = p.m_fMieAbsorption * (m_fHaze * fPerMmToPerKm);
  out_constants.RayleighScaleHeight = p.m_fRayleighScaleHeight;
  out_constants.MieScaleHeight = p.m_fMieScaleHeight;
  out_constants.MieAnisotropy = p.m_fMieAnisotropy;
  out_constants.MaxSkyViewHeight = p.m_fAtmosphereHeight * 0.999f; // stay inside the atmosphere
  out_constants.SunIntensity = m_fSunIntensity;
  out_constants.SunDiscCosAngle = ezMath::Cos(m_SunDiscSize * 0.5f);
  out_constants.SunDiscIntensity = m_SunDiscSize.GetRadian() > 0.0f ? m_fSunDiscIntensity : 0.0f;
  out_constants.GroundHeight = m_fGroundHeight;
  out_constants.GroundAlbedo = ezVec3(m_GroundAlbedo.r, m_GroundAlbedo.g, m_GroundAlbedo.b);
  out_constants.VirtualDistance = m_fVirtualDistance;
  out_constants.UseFog = m_bUseFog ? 1 : 0;
}

void ezAtmosphericScatteringComponent::SerializeComponent(ezWorldWriter& inout_stream) const
{
  SUPER::SerializeComponent(inout_stream);
  ezStreamWriter& s = inout_stream.GetStream();

  s << m_fPlanetRadius;
  s << m_fAtmosphereHeight;
  s << m_fGroundHeight;
  s << m_vRayleighScattering;
  s << m_fRayleighScaleHeight;
  s << m_fMieScattering;
  s << m_fMieScaleHeight;
  s << m_fMieAnisotropy;
  s << m_vOzoneAbsorption;
  s << m_fSunIntensity;
  s << m_SunDiscSize;
  s << m_fSunDiscIntensity;
  s << m_bUseFog;
  s << m_fVirtualDistance;
  s << m_GroundAlbedo;
  s << m_Preset;
  s << m_fAirDensity;
  s << m_fHaze;
  s << m_bSetLightColor;
  s << m_fMieAbsorption;
}

void ezAtmosphericScatteringComponent::DeserializeComponent(ezWorldReader& inout_stream)
{
  SUPER::DeserializeComponent(inout_stream);
  ezStreamReader& s = inout_stream.GetStream();
  // const ezUInt32 uiVersion = inout_stream.GetComponentTypeVersion(GetStaticRTTI());

  s >> m_fPlanetRadius;
  s >> m_fAtmosphereHeight;
  s >> m_fGroundHeight;
  s >> m_vRayleighScattering;
  s >> m_fRayleighScaleHeight;
  s >> m_fMieScattering;
  s >> m_fMieScaleHeight;
  s >> m_fMieAnisotropy;
  s >> m_vOzoneAbsorption;
  s >> m_fSunIntensity;
  s >> m_SunDiscSize;
  s >> m_fSunDiscIntensity;
  s >> m_bUseFog;
  s >> m_fVirtualDistance;
  s >> m_GroundAlbedo;
  s >> m_Preset;
  s >> m_fAirDensity;
  s >> m_fHaze;
  s >> m_bSetLightColor;
  s >> m_fMieAbsorption;
}

void ezAtmosphericScatteringComponent::SetPreset(ezEnum<ezAtmospherePreset> preset)
{
  m_Preset = preset;
}

void ezAtmosphericScatteringComponent::SetAirDensity(float fFactor)
{
  m_fAirDensity = fFactor;
}

void ezAtmosphericScatteringComponent::SetHaze(float fFactor)
{
  m_fHaze = fFactor;
}

void ezAtmosphericScatteringComponent::SetPlanetRadius(float fRadius)
{
  m_fPlanetRadius = fRadius;
}

void ezAtmosphericScatteringComponent::SetAtmosphereHeight(float fHeight)
{
  m_fAtmosphereHeight = fHeight;
}

void ezAtmosphericScatteringComponent::SetGroundAlbedo(const ezColor& color)
{
  m_GroundAlbedo = color;
}

void ezAtmosphericScatteringComponent::SetGroundHeight(float fHeight)
{
  m_fGroundHeight = fHeight;
}

void ezAtmosphericScatteringComponent::SetRayleighScattering(const ezVec3& vCoefficients)
{
  m_vRayleighScattering = vCoefficients;
}

void ezAtmosphericScatteringComponent::SetRayleighScaleHeight(float fHeight)
{
  m_fRayleighScaleHeight = fHeight;
}

void ezAtmosphericScatteringComponent::SetMieScattering(float fCoefficient)
{
  m_fMieScattering = fCoefficient;
}

void ezAtmosphericScatteringComponent::SetMieAbsorption(float fCoefficient)
{
  m_fMieAbsorption = fCoefficient;
}

void ezAtmosphericScatteringComponent::SetMieScaleHeight(float fHeight)
{
  m_fMieScaleHeight = fHeight;
}

void ezAtmosphericScatteringComponent::SetMieAnisotropy(float fAnisotropy)
{
  m_fMieAnisotropy = fAnisotropy;
}

void ezAtmosphericScatteringComponent::SetOzoneAbsorption(const ezVec3& vCoefficients)
{
  m_vOzoneAbsorption = vCoefficients;
}

void ezAtmosphericScatteringComponent::SetSunIntensity(float fIntensity)
{
  m_fSunIntensity = fIntensity;
}

void ezAtmosphericScatteringComponent::SetSunDiscSize(ezAngle size)
{
  m_SunDiscSize = size;
}

void ezAtmosphericScatteringComponent::SetSunDiscIntensity(float fIntensity)
{
  m_fSunDiscIntensity = fIntensity;
}

void ezAtmosphericScatteringComponent::SetUseFog(bool bUseFog)
{
  m_bUseFog = bUseFog;
}

void ezAtmosphericScatteringComponent::SetSetLightColor(bool bEnable)
{
  m_bSetLightColor = bEnable;
}

void ezAtmosphericScatteringComponent::SetVirtualDistance(float fVirtualDistance)
{
  m_fVirtualDistance = fVirtualDistance;
}

EZ_STATICLINK_FILE(RendererCore, RendererCore_Components_Implementation_AtmosphericScatteringComponent);
