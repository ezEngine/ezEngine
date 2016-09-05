#include <ParticlePlugin/PCH.h>
#include <ParticlePlugin/Type/ParticleTypeLight.h>
#include <RendererCore/Shader/ShaderResource.h>
#include <RendererFoundation/Descriptors/Descriptors.h>
#include <RendererFoundation/Device/Device.h>
#include <RendererCore/Pipeline/Declarations.h>
#include <RendererCore/Pipeline/RenderPipelinePass.h>
#include <RendererCore/RenderContext/RenderContext.h>
#include <RendererCore/Meshes/MeshBufferResource.h>
#include <RendererCore/Textures/TextureResource.h>
#include <RendererCore/Pipeline/RenderData.h>
#include <RendererCore/Lights/PointLightComponent.h>
#include <RendererCore/Pipeline/ExtractedRenderData.h>

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezParticleTypeLightFactory, 1, ezRTTIDefaultAllocator<ezParticleTypeLightFactory>)
{
  EZ_BEGIN_PROPERTIES
  {
    EZ_MEMBER_PROPERTY("Size Factor", m_fSizeFactor)->AddAttributes(new ezDefaultValueAttribute(5.0f), new ezClampValueAttribute(0.0f, 1000.0f)),
    EZ_MEMBER_PROPERTY("Intensity", m_fIntensity)->AddAttributes(new ezDefaultValueAttribute(10.0f), new ezClampValueAttribute(0.0f, 1000.0f)),
    EZ_MEMBER_PROPERTY("Percentage", m_uiPercentage)->AddAttributes(new ezDefaultValueAttribute(50), new ezClampValueAttribute(1, 100)),
  }
  EZ_END_PROPERTIES
}
EZ_END_DYNAMIC_REFLECTED_TYPE

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezParticleTypeLight, 1, ezRTTIDefaultAllocator<ezParticleTypeLight>)
EZ_END_DYNAMIC_REFLECTED_TYPE

ezParticleTypeLightFactory::ezParticleTypeLightFactory()
{
  m_fSizeFactor = 5.0f;
  m_fIntensity = 10.0f;
  m_uiPercentage = 50;
}


const ezRTTI* ezParticleTypeLightFactory::GetTypeType() const
{
  return ezGetStaticRTTI<ezParticleTypeLight>();
}

void ezParticleTypeLightFactory::CopyTypeProperties(ezParticleType* pObject) const
{
  ezParticleTypeLight* pType = static_cast<ezParticleTypeLight*>(pObject);

  pType->m_fSizeFactor = m_fSizeFactor;
  pType->m_fIntensity = m_fIntensity;
  pType->m_uiPercentage = m_uiPercentage;
}

enum class TypeLightVersion
{
  Version_0 = 0,
  Version_1,

  // insert new version numbers above
  Version_Count,
  Version_Current = Version_Count - 1
};

void ezParticleTypeLightFactory::Save(ezStreamWriter& stream) const
{
  const ezUInt8 uiVersion = (int)TypeLightVersion::Version_Current;
  stream << uiVersion;

  stream << m_fSizeFactor;
  stream << m_fIntensity;
  stream << m_uiPercentage;
}

void ezParticleTypeLightFactory::Load(ezStreamReader& stream)
{
  ezUInt8 uiVersion = 0;
  stream >> uiVersion;

  EZ_ASSERT_DEV(uiVersion <= (int)TypeLightVersion::Version_Current, "Invalid version %u", uiVersion);

  stream >> m_fSizeFactor;
  stream >> m_fIntensity;
  stream >> m_uiPercentage;
}

void ezParticleTypeLight::CreateRequiredStreams()
{
  CreateStream("Position", ezStream::DataType::Float3, &m_pStreamPosition);
  CreateStream("Size", ezStream::DataType::Float, &m_pStreamSize);
  CreateStream("Color", ezStream::DataType::Float4, &m_pStreamColor);

  if (m_uiPercentage < 100)
  {
    CreateStream("OnOff", ezStream::DataType::Int, &m_pStreamOnOff);
  }
}


void ezParticleTypeLight::ExtractRenderData(const ezView& view, ezExtractedRenderData* pExtractedRenderData) const
{
  const ezVec3* pPosition = m_pStreamPosition->GetData<ezVec3>();
  const float* pSize = m_pStreamSize->GetData<float>();
  const ezColor* pColor = m_pStreamColor->GetData<ezColor>();
  ezInt32* pIndex = m_pStreamOnOff->GetWritableData<ezInt32>();

  if (pPosition == nullptr || pSize == nullptr || pColor == nullptr || pIndex == nullptr)
    return;

  ezRandom& rng = GetRNG();

  const ezUInt32 uiNumParticles = (ezUInt32)GetOwnerSystem()->GetNumActiveParticles();

  const ezUInt32 uiBatchId = 1; // no shadows

  for (ezUInt32 i = 0; i < uiNumParticles; ++i)
  {
    if (pIndex[i] == 0)
    {
      if ((ezUInt32)rng.IntMinMax(0, 100) <= m_uiPercentage)
        pIndex[i] = 1;
      else
        pIndex[i] = -1;
    }

    if (pIndex[i] < 0)
      continue;

    auto pRenderData = ezCreateRenderDataForThisFrame<ezPointLightRenderData>(nullptr, uiBatchId);

    pRenderData->m_GlobalTransform.SetIdentity();
    pRenderData->m_GlobalTransform.m_vPosition = pPosition[i];
    pRenderData->m_LightColor = pColor[i];
    pRenderData->m_fIntensity = m_fIntensity;
    pRenderData->m_fRange = pSize[i] * m_fSizeFactor;
    pRenderData->m_bCastShadows = false;

    pExtractedRenderData->AddRenderData(pRenderData, ezDefaultRenderDataCategories::Light, uiBatchId);
  }
}
