#include <PCH.h>

#include <Core/World/GameObject.h>
#include <Core/World/World.h>
#include <Foundation/Algorithm/Sorting.h>
#include <Foundation/Profiling/Profiling.h>
#include <ParticlePlugin/Effect/ParticleEffectInstance.h>
#include <ParticlePlugin/Type/Distortion/ParticleTypeDistortion.h>
#include <RendererCore/Meshes/MeshBufferResource.h>
#include <RendererCore/Pipeline/Declarations.h>
#include <RendererCore/Pipeline/ExtractedRenderData.h>
#include <RendererCore/Pipeline/RenderPipelinePass.h>
#include <RendererCore/Pipeline/View.h>
#include <RendererCore/RenderContext/RenderContext.h>
#include <RendererCore/Shader/ShaderResource.h>
#include <RendererCore/Textures/Texture2DResource.h>
#include <RendererFoundation/Descriptors/Descriptors.h>
#include <RendererFoundation/Device/Device.h>

// clang-format off
EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezParticleTypeDistortionFactory, 1, ezRTTIDefaultAllocator<ezParticleTypeDistortionFactory>)
{
  EZ_BEGIN_PROPERTIES
  {
    EZ_MEMBER_PROPERTY("MaskTexture", m_sMaskTexture)->AddAttributes(new ezAssetBrowserAttribute("Texture 2D")),
    EZ_MEMBER_PROPERTY("DistortionTexture", m_sDistortionTexture)->AddAttributes(new ezAssetBrowserAttribute("Texture 2D")),
    EZ_MEMBER_PROPERTY("DistortionStrength", m_fDistortionStrength)->AddAttributes(new ezDefaultValueAttribute(100.0f), new ezClampValueAttribute(0.0f, 500.0f)),
    EZ_MEMBER_PROPERTY("NumSpritesX", m_uiNumSpritesX)->AddAttributes(new ezDefaultValueAttribute(1), new ezClampValueAttribute(1, 16)),
    EZ_MEMBER_PROPERTY("NumSpritesY", m_uiNumSpritesY)->AddAttributes(new ezDefaultValueAttribute(1), new ezClampValueAttribute(1, 16)),
    EZ_MEMBER_PROPERTY("TintColorParam", m_sTintColorParameter),
  }
  EZ_END_PROPERTIES;
}
EZ_END_DYNAMIC_REFLECTED_TYPE;

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezParticleTypeDistortion, 1, ezRTTIDefaultAllocator<ezParticleTypeDistortion>)
EZ_END_DYNAMIC_REFLECTED_TYPE;
    // clang-format on

    const ezRTTI* ezParticleTypeDistortionFactory::GetTypeType() const
{
  return ezGetStaticRTTI<ezParticleTypeDistortion>();
}


void ezParticleTypeDistortionFactory::CopyTypeProperties(ezParticleType* pObject) const
{
  ezParticleTypeDistortion* pType = static_cast<ezParticleTypeDistortion*>(pObject);

  pType->m_hMaskTexture.Invalidate();
  pType->m_hDistortionTexture.Invalidate();
  pType->m_fDistortionStrength = m_fDistortionStrength;
  pType->m_uiNumSpritesX = m_uiNumSpritesX;
  pType->m_uiNumSpritesY = m_uiNumSpritesY;
  pType->m_sTintColorParameter = ezTempHashedString(m_sTintColorParameter.GetData());

  if (!m_sMaskTexture.IsEmpty())
    pType->m_hMaskTexture = ezResourceManager::LoadResource<ezTexture2DResource>(m_sMaskTexture);

  if (!m_sDistortionTexture.IsEmpty())
    pType->m_hDistortionTexture = ezResourceManager::LoadResource<ezTexture2DResource>(m_sDistortionTexture);
}

enum class TypeDistortionVersion
{
  Version_0 = 0,
  Version_1,

  // insert new version numbers above
  Version_Count,
  Version_Current = Version_Count - 1
};

void ezParticleTypeDistortionFactory::Save(ezStreamWriter& stream) const
{
  const ezUInt8 uiVersion = (int)TypeDistortionVersion::Version_Current;
  stream << uiVersion;

  stream << m_sMaskTexture;
  stream << m_sDistortionTexture;
  stream << m_fDistortionStrength;
  stream << m_uiNumSpritesX;
  stream << m_uiNumSpritesY;
  stream << m_sTintColorParameter;
}

void ezParticleTypeDistortionFactory::Load(ezStreamReader& stream)
{
  ezUInt8 uiVersion = 0;
  stream >> uiVersion;

  EZ_ASSERT_DEV(uiVersion <= (int)TypeDistortionVersion::Version_Current, "Invalid version {0}", uiVersion);

  stream >> m_sMaskTexture;
  stream >> m_sDistortionTexture;
  stream >> m_fDistortionStrength;
  stream >> m_uiNumSpritesX;
  stream >> m_uiNumSpritesY;
  stream >> m_sTintColorParameter;
}

ezParticleTypeDistortion::ezParticleTypeDistortion() = default;
ezParticleTypeDistortion::~ezParticleTypeDistortion() = default;

void ezParticleTypeDistortion::CreateRequiredStreams()
{
  CreateStream("LifeTime", ezProcessingStream::DataType::Float2, &m_pStreamLifeTime, false);
  CreateStream("Position", ezProcessingStream::DataType::Float4, &m_pStreamPosition, false);
  CreateStream("Size", ezProcessingStream::DataType::Float, &m_pStreamSize, false);
  CreateStream("Color", ezProcessingStream::DataType::Float4, &m_pStreamColor, false);
  CreateStream("RotationSpeed", ezProcessingStream::DataType::Float, &m_pStreamRotationSpeed, false);
}

struct sod
{
  EZ_DECLARE_POD_TYPE();

  float dist;
  ezUInt32 index;
};

struct sodComparer
{
  // sort farther particles to the front, so that they get rendered first (back to front)
  EZ_ALWAYS_INLINE bool Less(const sod& a, const sod& b) const { return a.dist > b.dist; }
  EZ_ALWAYS_INLINE bool Equal(const sod& a, const sod& b) const { return a.dist == b.dist; }
};

void ezParticleTypeDistortion::ExtractTypeRenderData(const ezView& view, ezExtractedRenderData& extractedRenderData, const ezTransform& instanceTransform, ezUInt64 uiExtractedFrame) const
{
  EZ_PROFILE("PFX: Distortion");

  if (!m_hMaskTexture.IsValid() || !m_hDistortionTexture.IsValid() || m_fDistortionStrength <= 0)
    return;

  const ezUInt32 numParticles = (ezUInt32)GetOwnerSystem()->GetNumActiveParticles();

  if (numParticles == 0)
    return;

  const ezTime tCur = GetOwnerSystem()->GetWorld()->GetClock().GetAccumulatedTime();

  const ezVec3 vCameraPos = view.GetCamera()->GetCenterPosition();
  const bool bNeedsSorting = false; // not sure

  // don't copy the data multiple times in the same frame, if the effect is instanced
  if ((m_uiLastExtractedFrame != uiExtractedFrame)
      /*&& !bNeedsSorting*/) // TODO: in theory every shared instance has to sort the Distortions, in practice this maybe should be an option
  {
    const ezColor tintColor = GetOwnerEffect()->GetColorParameter(m_sTintColorParameter, ezColor::White);

    m_uiLastExtractedFrame = uiExtractedFrame;

    const ezVec2* pLifeTime = m_pStreamLifeTime->GetData<ezVec2>();
    const ezVec4* pPosition = m_pStreamPosition->GetData<ezVec4>();
    const float* pSize = m_pStreamSize->GetData<float>();
    const ezColor* pColor = m_pStreamColor->GetData<ezColor>();
    const float* pRotationSpeed = m_pStreamRotationSpeed->GetData<float>();

    // this will automatically be deallocated at the end of the frame
    m_ParticleData = EZ_NEW_ARRAY(ezFrameAllocator::GetCurrentAllocator(), ezDistortionParticleData, numParticles);

    ezTransform trans;
    if (bNeedsSorting)
    {
      // TODO: Using the frame allocator this way results in memory corruptions.
      // Not sure, whether this is supposed to work.
      ezHybridArray<sod, 64> sorted; // (ezFrameAllocator::GetCurrentAllocator());
      sorted.SetCountUninitialized(numParticles);

      for (ezUInt32 p = 0; p < numParticles; ++p)
      {
        const ezVec3 pos = pPosition[p].GetAsVec3();
        sorted[p].dist = (pos - vCameraPos).GetLengthSquared();
        sorted[p].index = p;
      }

      sorted.Sort(sodComparer());

      for (ezUInt32 p = 0; p < numParticles; ++p)
      {
        const ezUInt32 idx = sorted[p].index;

        trans.m_qRotation.SetFromAxisAndAngle(ezVec3(0, 1, 0), ezAngle::Radian((float)(tCur.GetSeconds() * pRotationSpeed[idx])));
        trans.m_vPosition = pPosition[idx].GetAsVec3();
        trans.m_vScale.Set(1.0f);
        m_ParticleData[p].Transform = trans;
        m_ParticleData[p].Size = pSize[idx];
        m_ParticleData[p].Color = pColor[idx] * tintColor;
        m_ParticleData[p].Life = pLifeTime[idx].x * pLifeTime[idx].y;
      }
    }
    else
    {
      for (ezUInt32 p = 0; p < numParticles; ++p)
      {
        const ezUInt32 idx = p;

        trans.m_qRotation.SetFromAxisAndAngle(ezVec3(0, 1, 0), ezAngle::Radian((float)(tCur.GetSeconds() * pRotationSpeed[idx])));
        trans.m_vPosition = pPosition[idx].GetAsVec3();
        trans.m_vScale.Set(1.0f);
        m_ParticleData[p].Transform = trans;
        m_ParticleData[p].Size = pSize[idx];
        m_ParticleData[p].Color = pColor[idx] * tintColor;
        m_ParticleData[p].Life = pLifeTime[idx].x * pLifeTime[idx].y;
      }
    }
  }

  /// \todo Is this batch ID correct?
  const ezUInt32 uiBatchId = m_hMaskTexture.GetResourceIDHash() + m_hDistortionTexture.GetResourceIDHash();
  auto pRenderData = ezCreateRenderDataForThisFrame<ezParticleDistortionRenderData>(nullptr, uiBatchId);

  // TODO: hack to render distortion effects first
  ezTransform adjustedTransform = instanceTransform;
  adjustedTransform.m_vPosition += view.GetCamera()->GetCenterDirForwards() * 0.1f;

  pRenderData->m_bApplyObjectTransform = GetOwnerEffect()->NeedsToApplyTransform();
  pRenderData->m_GlobalTransform = adjustedTransform;
  pRenderData->m_hMaskTexture = m_hMaskTexture;
  pRenderData->m_hDistortionTexture = m_hDistortionTexture;
  pRenderData->m_ParticleData = m_ParticleData;
  pRenderData->m_uiNumSpritesX = m_uiNumSpritesX;
  pRenderData->m_uiNumSpritesY = m_uiNumSpritesY;
  pRenderData->m_fDistortionStrength = m_fDistortionStrength;

  const ezUInt32 uiSortingKey = ComputeSortingKey(ezParticleTypeRenderMode::Opaque);
  extractedRenderData.AddRenderData(pRenderData, ezDefaultRenderDataCategories::LitTransparent, uiSortingKey);
}



EZ_STATICLINK_FILE(ParticlePlugin, ParticlePlugin_Type_Distortion_ParticleTypeDistortion);
