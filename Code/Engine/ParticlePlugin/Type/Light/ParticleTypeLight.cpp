#include <ParticlePlugin/PCH.h>
#include <ParticlePlugin/Type/Light/ParticleTypeLight.h>
#include <RendererCore/Shader/ShaderResource.h>
#include <RendererFoundation/Descriptors/Descriptors.h>
#include <RendererFoundation/Device/Device.h>
#include <RendererCore/Pipeline/Declarations.h>
#include <RendererCore/Pipeline/RenderPipelinePass.h>
#include <RendererCore/RenderContext/RenderContext.h>
#include <RendererCore/Meshes/MeshBufferResource.h>
#include <RendererCore/Textures/Texture2DResource.h>
#include <RendererCore/Pipeline/RenderData.h>
#include <RendererCore/Lights/PointLightComponent.h>
#include <RendererCore/Pipeline/ExtractedRenderData.h>

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezParticleTypeLightFactory, 1, ezRTTIDefaultAllocator<ezParticleTypeLightFactory>)
{
  EZ_BEGIN_PROPERTIES
  {
    EZ_MEMBER_PROPERTY("SizeFactor", m_fSizeFactor)->AddAttributes(new ezDefaultValueAttribute(5.0f), new ezClampValueAttribute(0.0f, 1000.0f)),
    EZ_MEMBER_PROPERTY("Intensity", m_fIntensity)->AddAttributes(new ezDefaultValueAttribute(10.0f), new ezClampValueAttribute(0.0f, 100000.0f)),
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

  EZ_ASSERT_DEV(uiVersion <= (int)TypeLightVersion::Version_Current, "Invalid version {0}", uiVersion);

  stream >> m_fSizeFactor;
  stream >> m_fIntensity;
  stream >> m_uiPercentage;
}

void ezParticleTypeLight::CreateRequiredStreams()
{
  m_pStreamOnOff = nullptr;

  CreateStream("Position", ezProcessingStream::DataType::Float3, &m_pStreamPosition, false);
  CreateStream("Size", ezProcessingStream::DataType::Float, &m_pStreamSize, false);
  CreateStream("Color", ezProcessingStream::DataType::Float4, &m_pStreamColor, false);

  if (m_uiPercentage < 100)
  {
    CreateStream("OnOff", ezProcessingStream::DataType::Int, &m_pStreamOnOff, false); /// \todo Initialize (instead of during extraction)
  }
}


void ezParticleTypeLight::ExtractTypeRenderData(const ezView& view, ezExtractedRenderData* pExtractedRenderData, const ezTransform& instanceTransform, ezUInt64 uiExtractedFrame) const
{
  const ezVec3* pPosition = m_pStreamPosition->GetData<ezVec3>();
  const float* pSize = m_pStreamSize->GetData<float>();
  const ezColor* pColor = m_pStreamColor->GetData<ezColor>();

  if (pPosition == nullptr || pSize == nullptr || pColor == nullptr)
    return;

  ezInt32* pOnOff = nullptr;

  if (m_pStreamOnOff)
  {
    pOnOff = m_pStreamOnOff->GetWritableData<ezInt32>();

    if (pOnOff == nullptr)
      return;
  }

  ezRandom& rng = GetRNG();

  const ezUInt32 uiNumParticles = (ezUInt32)GetOwnerSystem()->GetNumActiveParticles();

  const ezUInt32 uiBatchId = 1; // no shadows

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

    auto pRenderData = ezCreateRenderDataForThisFrame<ezPointLightRenderData>(nullptr, uiBatchId);

    pRenderData->m_GlobalTransform.SetIdentity();
    pRenderData->m_GlobalTransform.m_vPosition = instanceTransform * pPosition[i];
    pRenderData->m_LightColor = pColor[i];
    pRenderData->m_fIntensity = m_fIntensity;
    pRenderData->m_fRange = pSize[i] * m_fSizeFactor;
    pRenderData->m_bCastShadows = false;

    pExtractedRenderData->AddRenderData(pRenderData, ezDefaultRenderDataCategories::Light, uiBatchId);
  }
}
