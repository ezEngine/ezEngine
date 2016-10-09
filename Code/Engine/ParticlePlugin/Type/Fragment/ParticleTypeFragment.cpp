#include <ParticlePlugin/PCH.h>
#include <ParticlePlugin/Type/Fragment/ParticleTypeFragment.h>
#include <RendererCore/Shader/ShaderResource.h>
#include <RendererFoundation/Descriptors/Descriptors.h>
#include <RendererFoundation/Device/Device.h>
#include <RendererCore/Pipeline/Declarations.h>
#include <RendererCore/Pipeline/RenderPipelinePass.h>
#include <RendererCore/RenderContext/RenderContext.h>
#include <RendererCore/Meshes/MeshBufferResource.h>
#include <RendererCore/Textures/TextureResource.h>
#include <Core/World/GameObject.h>
#include <RendererCore/Pipeline/ExtractedRenderData.h>
#include <Core/World/World.h>

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezParticleTypeFragmentFactory, 1, ezRTTIDefaultAllocator<ezParticleTypeFragmentFactory>)
{
  EZ_BEGIN_PROPERTIES
  {
    EZ_MEMBER_PROPERTY("Texture", m_sTexture)->AddAttributes(new ezAssetBrowserAttribute("Texture 2D")),
  }
  EZ_END_PROPERTIES
}
EZ_END_DYNAMIC_REFLECTED_TYPE

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezParticleTypeFragment, 1, ezRTTIDefaultAllocator<ezParticleTypeFragment>)
EZ_END_DYNAMIC_REFLECTED_TYPE

const ezRTTI* ezParticleTypeFragmentFactory::GetTypeType() const
{
  return ezGetStaticRTTI<ezParticleTypeFragment>();
}


void ezParticleTypeFragmentFactory::CopyTypeProperties(ezParticleType* pObject) const
{
  ezParticleTypeFragment* pType = static_cast<ezParticleTypeFragment*>(pObject);

  pType->m_hTexture.Invalidate();

  if (!m_sTexture.IsEmpty())
    pType->m_hTexture = ezResourceManager::LoadResource<ezTextureResource>(m_sTexture);
}

enum class TypeFragmentVersion
{
  Version_0 = 0,
  Version_1,
  Version_2, // added texture

  // insert new version numbers above
  Version_Count,
  Version_Current = Version_Count - 1
};

void ezParticleTypeFragmentFactory::Save(ezStreamWriter& stream) const
{
  const ezUInt8 uiVersion = (int)TypeFragmentVersion::Version_Current;
  stream << uiVersion;

  stream << m_sTexture;
}

void ezParticleTypeFragmentFactory::Load(ezStreamReader& stream)
{
  ezUInt8 uiVersion = 0;
  stream >> uiVersion;

  EZ_ASSERT_DEV(uiVersion <= (int)TypeFragmentVersion::Version_Current, "Invalid version %u", uiVersion);

  if (uiVersion >= 2)
  {
    stream >> m_sTexture;
  }
}


ezParticleTypeFragment::ezParticleTypeFragment()
{
  m_uiLastExtractedFrame = 0;
}

void ezParticleTypeFragment::CreateRequiredStreams()
{
  CreateStream("Position", ezProcessingStream::DataType::Float3, &m_pStreamPosition);
  CreateStream("Size", ezProcessingStream::DataType::Float, &m_pStreamSize);
  CreateStream("Color", ezProcessingStream::DataType::Float4, &m_pStreamColor);
  CreateStream("RotationSpeed", ezProcessingStream::DataType::Float, &m_pStreamRotationSpeed);
}

void ezParticleTypeFragment::ExtractTypeRenderData(const ezView& view, ezExtractedRenderData* pExtractedRenderData, const ezTransform& instanceTransform, ezUInt64 uiExtractedFrame) const
{
  if (!m_hTexture.IsValid())
    return;

  if (GetOwnerSystem()->GetNumActiveParticles() == 0)
    return;

  const ezTime tCur = GetOwnerSystem()->GetWorld()->GetClock().GetAccumulatedTime();

  // don't copy the data multiple times in the same frame, if the effect is instanced
  if (m_uiLastExtractedFrame != uiExtractedFrame)
  {
    m_uiLastExtractedFrame = uiExtractedFrame;

    const ezVec3* pPosition = m_pStreamPosition->GetData<ezVec3>();
    const float* pSize = m_pStreamSize->GetData<float>();
    const ezColor* pColor = m_pStreamColor->GetData<ezColor>();
    const float* pRotationSpeed = m_pStreamRotationSpeed->GetData<float>();

    if (m_GpuData == nullptr)
    {
      m_GpuData = EZ_DEFAULT_NEW(ezFragmentParticleDataContainer);
      m_GpuData->m_Content.SetCountUninitialized((ezUInt32)GetOwnerSystem()->GetMaxParticles());
    }

    ezFragmentParticleData* TempData = m_GpuData->m_Content.GetData();

    ezTransform t;

    for (ezUInt32 p = 0; p < (ezUInt32)GetOwnerSystem()->GetNumActiveParticles(); ++p)
    {
      t.m_Rotation.SetRotationMatrixY(ezAngle::Radian((float)(tCur.GetSeconds() * pRotationSpeed[p])));
      t.m_vPosition = pPosition[p];
      TempData[p].Transform = t;
      TempData[p].Size = pSize[p];
      TempData[p].Color = pColor[p];
    }
  }

  /// \todo Is this batch ID correct?
  const ezUInt32 uiBatchId = m_hTexture.GetResourceIDHash();
  auto pRenderData = ezCreateRenderDataForThisFrame<ezParticleFragmentRenderData>(nullptr, uiBatchId);

  pRenderData->m_GlobalTransform = instanceTransform;
  pRenderData->m_uiNumParticles = (ezUInt32)GetOwnerSystem()->GetNumActiveParticles();
  pRenderData->m_hTexture = m_hTexture;
  pRenderData->m_GpuData = m_GpuData;

  /// \todo Generate a proper sorting key?
  const ezUInt32 uiSortingKey = 0;
  pExtractedRenderData->AddRenderData(pRenderData, ezDefaultRenderDataCategories::SimpleTransparent, uiSortingKey);
}


