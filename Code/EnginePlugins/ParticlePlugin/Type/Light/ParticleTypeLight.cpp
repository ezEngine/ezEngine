#include <ParticlePlugin/ParticlePluginPCH.h>

#include <Foundation/Math/Color16f.h>
#include <Foundation/Math/Float16.h>
#include <Foundation/Profiling/Profiling.h>
#include <ParticlePlugin/Effect/ParticleEffectInstance.h>
#include <ParticlePlugin/Type/Light/ParticleTypeLight.h>
#include <RendererCore/Lights/PointLightComponent.h>
#include <RendererCore/Pipeline/RenderDataManager.h>
#include <RendererCore/Pipeline/View.h>

// clang-format off
EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezParticleTypeLight, 1, ezRTTIDefaultAllocator<ezParticleTypeLight>)
{
  EZ_BEGIN_PROPERTIES
  {
    EZ_MEMBER_PROPERTY("SizeFactor", m_fSizeFactor)->AddAttributes(new ezDefaultValueAttribute(5.0f), new ezClampValueAttribute(0.0f, 1000.0f)),
    EZ_MEMBER_PROPERTY("Intensity", m_fIntensity)->AddAttributes(new ezDefaultValueAttribute(10.0f), new ezClampValueAttribute(0.0f, 100000.0f)),
    EZ_MEMBER_PROPERTY("ScaleIntensityByAlpha", m_bScaleIntensityByAlpha),
    EZ_MEMBER_PROPERTY("Percentage", m_uiPercentage)->AddAttributes(new ezDefaultValueAttribute(50), new ezClampValueAttribute(1, 100)),
    EZ_MEMBER_PROPERTY("TintColorParam", m_sTintColorParameter),
    EZ_MEMBER_PROPERTY("IntensityScaleParam", m_sIntensityParameter),
    EZ_MEMBER_PROPERTY("SizeScaleParam", m_sSizeScaleParameter),
  }
  EZ_END_PROPERTIES;
}
EZ_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

ezParticleTypeLight::ezParticleTypeLight()
{
  m_fSizeFactor = 5.0f;
  m_fIntensity = 10.0f;
  m_uiPercentage = 50;
}


enum class TypeLightVersion
{
  Version_0 = 0,
  Version_1,
  Version_2, // added tint color and intensity parameter
  Version_3, // added scale intensity by alpha

  // insert new version numbers above
  Version_Count,
  Version_Current = Version_Count - 1
};

void ezParticleTypeLight::Save(ezStreamWriter& inout_stream) const
{
  const ezUInt8 uiVersion = (int)TypeLightVersion::Version_Current;
  inout_stream << uiVersion;

  inout_stream << m_fSizeFactor;
  inout_stream << m_fIntensity;
  inout_stream << m_uiPercentage;

  // Version 2
  inout_stream << m_sTintColorParameter;
  inout_stream << m_sIntensityParameter;
  inout_stream << m_sSizeScaleParameter;

  // Version 3
  inout_stream << m_bScaleIntensityByAlpha;
}

void ezParticleTypeLight::Load(ezStreamReader& inout_stream, const ezParticleEffectDescriptor& ownerEffectDescriptor, const ezParticleSystemDescriptor& ownerSystemDescriptor)
{
  ezUInt8 uiVersion = 0;
  inout_stream >> uiVersion;

  EZ_ASSERT_DEV(uiVersion <= (int)TypeLightVersion::Version_Current, "Invalid version {0}", uiVersion);

  inout_stream >> m_fSizeFactor;
  inout_stream >> m_fIntensity;
  inout_stream >> m_uiPercentage;

  if (uiVersion >= 2)
  {
    inout_stream >> m_sTintColorParameter;
    inout_stream >> m_sIntensityParameter;
    inout_stream >> m_sSizeScaleParameter;
  }

  if (uiVersion >= 3)
  {
    inout_stream >> m_bScaleIntensityByAlpha;
  }
}

bool ezParticleTypeLight::GetInstanceDataDesc(ezInstanceDataDesc& out_desc) const
{
  out_desc.FillFromType<InstanceData>();
  return true;
}

void ezParticleTypeLight::CreateRequiredStreams(const ezParticleModuleContext& ctx) const
{
  InstanceData& data = ctx.GetInstanceData<InstanceData>();

  data.m_pStreamOnOff = nullptr;

  ctx.CreateStream(ezParticleStreamNames::s_sPosition, ezProcessingStream::DataType::Float4, &data.m_pStreamPosition, false);
  ctx.CreateStream(ezParticleStreamNames::s_sSize, ezProcessingStream::DataType::Half, &data.m_pStreamSize, false);
  ctx.CreateStream(ezParticleStreamNames::s_sColor, ezProcessingStream::DataType::Half4, &data.m_pStreamColor, false);

  if (m_uiPercentage < 100)
  {
    ctx.CreateStream(ezParticleStreamNames::s_sOnOff, ezProcessingStream::DataType::Byte, &data.m_pStreamOnOff, false); /// \todo Initialize (instead of during extraction)
  }
}


void ezParticleTypeLight::ExtractTypeRenderData(const ezParticleModuleContext& ctx, ezMsgExtractRenderData& ref_msg, const ezTransform& instanceTransform) const
{
  InstanceData& data = ctx.GetInstanceData<InstanceData>();

  EZ_PROFILE_SCOPE("PFX: Light");

  const ezVec4* pPosition = data.m_pStreamPosition->GetData<ezVec4>();
  const ezFloat16* pSize = data.m_pStreamSize->GetData<ezFloat16>();
  const ezColorLinear16f* pColor = data.m_pStreamColor->GetData<ezColorLinear16f>();

  if (pPosition == nullptr || pSize == nullptr || pColor == nullptr)
    return;

  ezInt8* pOnOff = nullptr;

  if (data.m_pStreamOnOff)
  {
    pOnOff = data.m_pStreamOnOff->GetWritableData<ezInt8>();

    if (pOnOff == nullptr)
      return;
  }

  ezRandom& rng = ctx.GetRNG();

  const ezUInt32 uiNumParticles = (ezUInt32)ctx.m_pSystem->GetNumActiveParticles();

  const ezUInt32 uiBatchId = 1; // no shadows

  const ezColor tintColor = ctx.GetEffect()->GetColorParameter(m_sTintColorParameter, ezColor::White);
  const float intensityScale = ctx.GetEffect()->GetFloatParameter(m_sIntensityParameter, 1.0f);
  const float sizeScale = ctx.GetEffect()->GetFloatParameter(m_sSizeScaleParameter, 1.0f);

  const float sizeFactor = m_fSizeFactor * sizeScale;
  const float intensity = intensityScale * m_fIntensity;

  ezTransform transform;

  if (ctx.GetEffect()->IsSimulatedInLocalSpace())
    transform = instanceTransform;
  else
    transform.SetIdentity();

  for (ezUInt32 i = 0; i < uiNumParticles; ++i)
  {
    if (pOnOff)
    {
      if (pOnOff[i] == 0)
      {
        if ((ezUInt32)rng.IntMinMax(0, 100) <= m_uiPercentage)
          pOnOff[i] = 1;
        else
          pOnOff[i] = -1;
      }

      if (pOnOff[i] < 0)
        continue;
    }

    auto pRenderData = ref_msg.m_pRenderDataManager->CreateRenderDataForThisFrame<ezPointLightRenderData>(nullptr);

    pRenderData->m_vGlobalPosition = transform * pPosition[i].GetAsVec3();
    const ezColor color = tintColor * pColor[i].ToLinearFloat();

    pRenderData->m_LightColor = color;
    pRenderData->m_fIntensity = m_bScaleIntensityByAlpha ? intensity * ezMath::Max(0.0f, color.a) : intensity;
    pRenderData->m_fSpecularMultiplier = 1.0f;
    pRenderData->m_fRadius = 0.0f;
    pRenderData->m_fRange = ezMath::Max(0.01f, pSize[i] * sizeFactor);
    pRenderData->m_uiShadowDataOffsetAndFadeOut = 0;
    pRenderData->m_qGlobalRotation.SetIdentity();
    pRenderData->m_fLength = 0.0f;

    float fScreenSpaceSize = ezLightComponent::CalculateScreenSpaceSize(ezBoundingSphere::MakeFromCenterAndRadius(pRenderData->m_vGlobalPosition, pRenderData->m_fRange * 0.5f), *ref_msg.m_pView->GetCullingCamera());
    pRenderData->FillSortingKey(fScreenSpaceSize);

    ref_msg.AddRenderData(pRenderData, ezDefaultRenderDataCategories::Light, ezRenderData::Caching::Never);
  }
}



EZ_STATICLINK_FILE(ParticlePlugin, ParticlePlugin_Type_Light_ParticleTypeLight);
