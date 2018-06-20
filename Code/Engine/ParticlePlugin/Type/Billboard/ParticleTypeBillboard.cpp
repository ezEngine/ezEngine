#include <PCH.h>
#include <ParticlePlugin/Type/Billboard/ParticleTypeBillboard.h>
#include <RendererCore/Shader/ShaderResource.h>
#include <RendererFoundation/Descriptors/Descriptors.h>
#include <RendererFoundation/Device/Device.h>
#include <RendererCore/Pipeline/Declarations.h>
#include <RendererCore/Pipeline/RenderPipelinePass.h>
#include <RendererCore/RenderContext/RenderContext.h>
#include <RendererCore/Meshes/MeshBufferResource.h>
#include <RendererCore/Textures/Texture2DResource.h>
#include <Core/World/GameObject.h>
#include <RendererCore/Pipeline/ExtractedRenderData.h>
#include <Core/World/World.h>
#include <Foundation/Profiling/Profiling.h>
#include <RendererCore/Pipeline/View.h>
#include <Foundation/Algorithm/Sorting.h>
#include <ParticlePlugin/Effect/ParticleEffectInstance.h>

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezParticleTypeBillboardFactory, 1, ezRTTIDefaultAllocator<ezParticleTypeBillboardFactory>)
{
  EZ_BEGIN_PROPERTIES
  {
    EZ_ENUM_MEMBER_PROPERTY("RenderMode", ezParticleTypeRenderMode, m_RenderMode),
    EZ_MEMBER_PROPERTY("Texture", m_sTexture)->AddAttributes(new ezAssetBrowserAttribute("Texture 2D")),
    EZ_MEMBER_PROPERTY("NumSpritesX", m_uiNumSpritesX)->AddAttributes(new ezDefaultValueAttribute(1), new ezClampValueAttribute(1, 16)),
    EZ_MEMBER_PROPERTY("NumSpritesY", m_uiNumSpritesY)->AddAttributes(new ezDefaultValueAttribute(1), new ezClampValueAttribute(1, 16)),
    EZ_MEMBER_PROPERTY("TintColorParam", m_sTintColorParameter),
  }
  EZ_END_PROPERTIES
}
EZ_END_DYNAMIC_REFLECTED_TYPE

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezParticleTypeBillboard, 1, ezRTTIDefaultAllocator<ezParticleTypeBillboard>)
EZ_END_DYNAMIC_REFLECTED_TYPE

const ezRTTI* ezParticleTypeBillboardFactory::GetTypeType() const
{
  return ezGetStaticRTTI<ezParticleTypeBillboard>();
}


void ezParticleTypeBillboardFactory::CopyTypeProperties(ezParticleType* pObject) const
{
  ezParticleTypeBillboard* pType = static_cast<ezParticleTypeBillboard*>(pObject);

  pType->m_hTexture.Invalidate();
  pType->m_RenderMode = m_RenderMode;
  pType->m_uiNumSpritesX = m_uiNumSpritesX;
  pType->m_uiNumSpritesY = m_uiNumSpritesY;
  pType->m_sTintColorParameter = ezTempHashedString(m_sTintColorParameter.GetData());

  if (!m_sTexture.IsEmpty())
    pType->m_hTexture = ezResourceManager::LoadResource<ezTexture2DResource>(m_sTexture);
}

enum class TypeBillboardVersion
{
  Version_0 = 0,
  Version_1,
  Version_2, // added texture
  Version_3, // added opacity
  Version_4, // added render mode
  Version_5, // added num sprites XY
  Version_6, // added tint color parameter

  // insert new version numbers above
  Version_Count,
  Version_Current = Version_Count - 1
};

void ezParticleTypeBillboardFactory::Save(ezStreamWriter& stream) const
{
  const ezUInt8 uiVersion = (int)TypeBillboardVersion::Version_Current;
  stream << uiVersion;

  stream << m_sTexture;
  stream << m_RenderMode;
  stream << m_uiNumSpritesX;
  stream << m_uiNumSpritesY;
  stream << m_sTintColorParameter;
}

void ezParticleTypeBillboardFactory::Load(ezStreamReader& stream)
{
  ezUInt8 uiVersion = 0;
  stream >> uiVersion;

  EZ_ASSERT_DEV(uiVersion <= (int)TypeBillboardVersion::Version_Current, "Invalid version {0}", uiVersion);

  if (uiVersion >= 2)
  {
    stream >> m_sTexture;
  }

  if (uiVersion == 3)
  {
    float fOpacity;
    stream >> fOpacity;
  }

  if (uiVersion >= 4)
  {
    stream >> m_RenderMode;
  }

  if (uiVersion >= 5)
  {
    stream >> m_uiNumSpritesX;
    stream >> m_uiNumSpritesY;
  }

  if (uiVersion >= 6)
  {
    stream >> m_sTintColorParameter;
  }
}

ezParticleTypeBillboard::ezParticleTypeBillboard()
{
}

ezParticleTypeBillboard::~ezParticleTypeBillboard()
{
}

void ezParticleTypeBillboard::CreateRequiredStreams()
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

void ezParticleTypeBillboard::ExtractTypeRenderData(const ezView& view, ezExtractedRenderData& extractedRenderData, const ezTransform& instanceTransform, ezUInt64 uiExtractedFrame) const
{
  EZ_PROFILE("PFX: Billboard");

  if (!m_hTexture.IsValid())
    return;

  const ezUInt32 numParticles = (ezUInt32)GetOwnerSystem()->GetNumActiveParticles();

  if (numParticles == 0)
    return;

  const ezTime tCur = GetOwnerSystem()->GetWorld()->GetClock().GetAccumulatedTime();

  const ezVec3 vCameraPos = view.GetCamera()->GetCenterPosition();
  const bool bNeedsSorting = m_RenderMode == ezParticleTypeRenderMode::Blended;

  // don't copy the data multiple times in the same frame, if the effect is instanced
  if ((m_uiLastExtractedFrame != uiExtractedFrame)
      /*&& !bNeedsSorting*/) // TODO: in theory every shared instance has to sort the billboards, in practice this maybe should be an option
  {
    const ezColor tintColor = GetOwnerEffect()->GetColorParameter(m_sTintColorParameter, ezColor::White);

    m_uiLastExtractedFrame = uiExtractedFrame;

    const ezVec2* pLifeTime = m_pStreamLifeTime->GetData<ezVec2>();
    const ezVec4* pPosition = m_pStreamPosition->GetData<ezVec4>();
    const float* pSize = m_pStreamSize->GetData<float>();
    const ezColor* pColor = m_pStreamColor->GetData<ezColor>();
    const float* pRotationSpeed = m_pStreamRotationSpeed->GetData<float>();

    // this will automatically be deallocated at the end of the frame
    m_ParticleData = EZ_NEW_ARRAY(ezFrameAllocator::GetCurrentAllocator(), ezBillboardParticleData, numParticles);

    ezTransform trans;
    if (bNeedsSorting)
    {
      // TODO: Using the frame allocator this way results in memory corruptions.
      // Not sure, whether this is supposed to work.
      ezHybridArray<sod, 64> sorted;// (ezFrameAllocator::GetCurrentAllocator());
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
  const ezUInt32 uiBatchId = m_hTexture.GetResourceIDHash();
  auto pRenderData = ezCreateRenderDataForThisFrame<ezParticleBillboardRenderData>(nullptr, uiBatchId);

  pRenderData->m_bApplyObjectTransform = GetOwnerEffect()->NeedsToApplyTransform();
  pRenderData->m_GlobalTransform = instanceTransform;
  pRenderData->m_RenderMode = m_RenderMode;
  pRenderData->m_hTexture = m_hTexture;
  pRenderData->m_ParticleData = m_ParticleData;
  pRenderData->m_uiNumSpritesX = m_uiNumSpritesX;
  pRenderData->m_uiNumSpritesY = m_uiNumSpritesY;

  /// \todo Generate a proper sorting key?
  const ezUInt32 uiSortingKey = 0;
  extractedRenderData.AddRenderData(pRenderData, ezDefaultRenderDataCategories::SimpleTransparent, uiSortingKey);
}




EZ_STATICLINK_FILE(ParticlePlugin, ParticlePlugin_Type_Billboard_ParticleTypeBillboard);

