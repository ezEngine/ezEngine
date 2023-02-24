#include <RendererCore/RendererCorePCH.h>

#include <Core/WorldSerializer/WorldReader.h>
#include <Core/WorldSerializer/WorldWriter.h>
#include <RendererCore/Components/AtmosphericScatteringComponent.h>
#include <RendererCore/Components/AtmosphericScatteringRenderer.h>
#include <RendererCore/Pipeline/RenderDataManager.h>
#include <RendererCore/Pipeline/View.h>

// clang-format off
EZ_BEGIN_COMPONENT_TYPE(ezAtmosphericScatteringComponent, 1, ezComponentMode::Static)
{
  EZ_BEGIN_PROPERTIES
  {
    // Commonly adjusted properties come first and stay ungrouped.
    // Note: properties without a group attribute are added to the group of the preceding property, so the order matters.
    EZ_ACCESSOR_PROPERTY("SunIntensity", GetSunIntensity, SetSunIntensity)->AddAttributes(new ezClampValueAttribute(0.0f, ezVariant()), new ezDefaultValueAttribute(40.0f)),
    EZ_ACCESSOR_PROPERTY("SunDiscSize", GetSunDiscSize, SetSunDiscSize)->AddAttributes(new ezClampValueAttribute(ezAngle::MakeZero(), ezAngle::MakeFromDegree(20.0f)), new ezDefaultValueAttribute(ezAngle::MakeFromDegree(1.0f))),
    EZ_ACCESSOR_PROPERTY("SunDiscIntensity", GetSunDiscIntensity, SetSunDiscIntensity)->AddAttributes(new ezClampValueAttribute(0.0f, ezVariant()), new ezDefaultValueAttribute(3.0f)),
    EZ_ACCESSOR_PROPERTY("GroundHeight", GetGroundHeight, SetGroundHeight)->AddAttributes(new ezSuffixAttribute(" m")),
    EZ_ACCESSOR_PROPERTY("UseFog", GetUseFog, SetUseFog)->AddAttributes(new ezDefaultValueAttribute(true)),
    EZ_ACCESSOR_PROPERTY("VirtualDistance", GetVirtualDistance, SetVirtualDistance)->AddAttributes(new ezClampValueAttribute(0.0f, ezVariant()), new ezDefaultValueAttribute(1000.0f), new ezSuffixAttribute(" m")),

    EZ_ACCESSOR_PROPERTY("PlanetRadius", GetPlanetRadius, SetPlanetRadius)->AddAttributes(new ezGroupAttribute("Planet", "", -1.0f, true), new ezClampValueAttribute(1.0f, ezVariant()), new ezDefaultValueAttribute(6371.0f), new ezSuffixAttribute(" km")),
    EZ_ACCESSOR_PROPERTY("AtmosphereHeight", GetAtmosphereHeight, SetAtmosphereHeight)->AddAttributes(new ezClampValueAttribute(0.1f, ezVariant()), new ezDefaultValueAttribute(100.0f), new ezSuffixAttribute(" km")),
    EZ_ACCESSOR_PROPERTY("GroundAlbedo", GetGroundAlbedo, SetGroundAlbedo)->AddAttributes(new ezDefaultValueAttribute(ezColor(0.3f, 0.3f, 0.3f))),

    EZ_ACCESSOR_PROPERTY("RayleighScattering", GetRayleighScattering, SetRayleighScattering)->AddAttributes(new ezGroupAttribute("Scattering", "", -1.0f, true), new ezClampValueAttribute(ezVec3(0.0f), ezVariant()), new ezDefaultValueAttribute(ezVec3(5.5f, 13.0f, 22.4f))),
    EZ_ACCESSOR_PROPERTY("RayleighScaleHeight", GetRayleighScaleHeight, SetRayleighScaleHeight)->AddAttributes(new ezClampValueAttribute(0.01f, ezVariant()), new ezDefaultValueAttribute(8.0f), new ezSuffixAttribute(" km")),
    EZ_ACCESSOR_PROPERTY("MieScattering", GetMieScattering, SetMieScattering)->AddAttributes(new ezClampValueAttribute(0.0f, ezVariant()), new ezDefaultValueAttribute(21.0f)),
    EZ_ACCESSOR_PROPERTY("MieScaleHeight", GetMieScaleHeight, SetMieScaleHeight)->AddAttributes(new ezClampValueAttribute(0.01f, ezVariant()), new ezDefaultValueAttribute(1.2f), new ezSuffixAttribute(" km")),
    EZ_ACCESSOR_PROPERTY("MieAnisotropy", GetMieAnisotropy, SetMieAnisotropy)->AddAttributes(new ezClampValueAttribute(-0.99f, 0.99f), new ezDefaultValueAttribute(0.7f)),
    EZ_ACCESSOR_PROPERTY("OzoneAbsorption", GetOzoneAbsorption, SetOzoneAbsorption)->AddAttributes(new ezClampValueAttribute(ezVec3(0.0f), ezVariant()), new ezDefaultValueAttribute(ezVec3(20.4f, 49.7f, 1.95f))),
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

void ezAtmosphericScatteringComponent::FillConstants(ezAtmosphereConstants& out_constants) const
{
  // The shaders work in kilometers to keep the float precision of the ray-sphere intersections acceptable.
  // Scattering coefficients are exposed per Mm (1000 km) to get readable numbers.
  const float fPerMmToPerKm = 1e-3f;

  // light travels along +X, so the direction towards the sun is -X
  ezVec3 vSunDir = GetOwner()->GetGlobalRotation() * ezVec3(-1.0f, 0.0f, 0.0f);
  vSunDir.NormalizeIfNotZero(ezVec3(0.0f, 0.0f, 1.0f)).IgnoreResult();

  out_constants = {};
  out_constants.RayleighScattering = m_vRayleighScattering * fPerMmToPerKm;
  out_constants.PlanetRadius = m_fPlanetRadius;
  out_constants.OzoneAbsorption = m_vOzoneAbsorption * fPerMmToPerKm;
  out_constants.AtmosphereRadius = m_fPlanetRadius + m_fAtmosphereHeight;
  out_constants.SunDirection = vSunDir;
  out_constants.MieScattering = m_fMieScattering * fPerMmToPerKm;
  out_constants.RayleighScaleHeight = m_fRayleighScaleHeight;
  out_constants.MieScaleHeight = m_fMieScaleHeight;
  out_constants.MieAnisotropy = m_fMieAnisotropy;
  out_constants.MaxSkyViewHeight = m_fAtmosphereHeight * 0.999f; // stay inside the atmosphere
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
}

void ezAtmosphericScatteringComponent::DeserializeComponent(ezWorldReader& inout_stream)
{
  SUPER::DeserializeComponent(inout_stream);
  ezStreamReader& s = inout_stream.GetStream();

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

void ezAtmosphericScatteringComponent::SetVirtualDistance(float fVirtualDistance)
{
  m_fVirtualDistance = fVirtualDistance;
}

EZ_STATICLINK_FILE(RendererCore, RendererCore_Components_Implementation_AtmosphericScatteringComponent);
