#include <PCH.h>

#include <ParticlePlugin/Type/Quad/ParticleTypeQuad.h>

#include <Core/World/GameObject.h>
#include <Core/World/World.h>
#include <Foundation/Math/Float16.h>
#include <Foundation/Profiling/Profiling.h>
#include <ParticlePlugin/Effect/ParticleEffectInstance.h>
#include <RendererCore/Pipeline/ExtractedRenderData.h>
#include <RendererCore/Pipeline/View.h>

// clang-format off
EZ_BEGIN_STATIC_REFLECTED_ENUM(ezQuadParticleOrientation, 1)
  EZ_ENUM_CONSTANTS(ezQuadParticleOrientation::Billboard)
  EZ_ENUM_CONSTANTS(ezQuadParticleOrientation::FragmentOrthogonalEmitterDirection, ezQuadParticleOrientation::FragmentEmitterDirection)
  EZ_ENUM_CONSTANTS(ezQuadParticleOrientation::SpriteEmitterDirection, ezQuadParticleOrientation::SpriteRandom, ezQuadParticleOrientation::SpriteWorldUp)
EZ_END_STATIC_REFLECTED_ENUM;

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezParticleTypeQuadFactory, 1, ezRTTIDefaultAllocator<ezParticleTypeQuadFactory>)
{
  EZ_BEGIN_PROPERTIES
  {
    EZ_ENUM_MEMBER_PROPERTY("Orientation", ezQuadParticleOrientation, m_Orientation),
    EZ_ENUM_MEMBER_PROPERTY("RenderMode", ezParticleTypeRenderMode, m_RenderMode),
    EZ_MEMBER_PROPERTY("Texture", m_sTexture)->AddAttributes(new ezAssetBrowserAttribute("Texture 2D")),
    EZ_MEMBER_PROPERTY("NumSpritesX", m_uiNumSpritesX)->AddAttributes(new ezDefaultValueAttribute(1), new ezClampValueAttribute(1, 16)),
    EZ_MEMBER_PROPERTY("NumSpritesY", m_uiNumSpritesY)->AddAttributes(new ezDefaultValueAttribute(1), new ezClampValueAttribute(1, 16)),
    EZ_MEMBER_PROPERTY("TintColorParam", m_sTintColorParameter),
  }
  EZ_END_PROPERTIES;
}
EZ_END_DYNAMIC_REFLECTED_TYPE;

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezParticleTypeQuad, 1, ezRTTIDefaultAllocator<ezParticleTypeQuad>)
EZ_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

const ezRTTI* ezParticleTypeQuadFactory::GetTypeType() const
{
  return ezGetStaticRTTI<ezParticleTypeQuad>();
}

void ezParticleTypeQuadFactory::CopyTypeProperties(ezParticleType* pObject) const
{
  ezParticleTypeQuad* pType = static_cast<ezParticleTypeQuad*>(pObject);

  pType->m_Orientation = m_Orientation;
  pType->m_hTexture.Invalidate();
  pType->m_RenderMode = m_RenderMode;
  pType->m_uiNumSpritesX = m_uiNumSpritesX;
  pType->m_uiNumSpritesY = m_uiNumSpritesY;
  pType->m_sTintColorParameter = ezTempHashedString(m_sTintColorParameter.GetData());

  if (!m_sTexture.IsEmpty())
    pType->m_hTexture = ezResourceManager::LoadResource<ezTexture2DResource>(m_sTexture);
}

enum class TypeQuadVersion
{
  Version_0 = 0,
  Version_1,

  // insert new version numbers above
  Version_Count,
  Version_Current = Version_Count - 1
};

void ezParticleTypeQuadFactory::Save(ezStreamWriter& stream) const
{
  const ezUInt8 uiVersion = (int)TypeQuadVersion::Version_Current;
  stream << uiVersion;

  stream << m_Orientation;
  stream << m_RenderMode;
  stream << m_sTexture;
  stream << m_uiNumSpritesX;
  stream << m_uiNumSpritesY;
  stream << m_sTintColorParameter;
}

void ezParticleTypeQuadFactory::Load(ezStreamReader& stream)
{
  ezUInt8 uiVersion = 0;
  stream >> uiVersion;

  EZ_ASSERT_DEV(uiVersion <= (int)TypeQuadVersion::Version_Current, "Invalid version {0}", uiVersion);

  stream >> m_Orientation;
  stream >> m_RenderMode;
  stream >> m_sTexture;
  stream >> m_uiNumSpritesX;
  stream >> m_uiNumSpritesY;
  stream >> m_sTintColorParameter;
}

ezParticleTypeQuad::ezParticleTypeQuad() = default;
ezParticleTypeQuad::~ezParticleTypeQuad() = default;

void ezParticleTypeQuad::CreateRequiredStreams()
{
  CreateStream("LifeTime", ezProcessingStream::DataType::Float2, &m_pStreamLifeTime, false);
  CreateStream("Position", ezProcessingStream::DataType::Float4, &m_pStreamPosition, false);
  CreateStream("Size", ezProcessingStream::DataType::Half, &m_pStreamSize, false);
  CreateStream("Color", ezProcessingStream::DataType::Float4, &m_pStreamColor, false);
  CreateStream("RotationSpeed", ezProcessingStream::DataType::Half, &m_pStreamRotationSpeed, false);
  CreateStream("RotationOffset", ezProcessingStream::DataType::Half, &m_pStreamRotationOffset, false);
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

void ezParticleTypeQuad::ExtractTypeRenderData(const ezView& view, ezExtractedRenderData& extractedRenderData,
                                               const ezTransform& instanceTransform, ezUInt64 uiExtractedFrame) const
{
  EZ_PROFILE("PFX: Quad");

  if (!m_hTexture.IsValid())
    return;

  const ezUInt32 numParticles = (ezUInt32)GetOwnerSystem()->GetNumActiveParticles();

  if (numParticles == 0)
    return;

  const bool bNeedsSorting = m_RenderMode == ezParticleTypeRenderMode::Blended;

  // don't copy the data multiple times in the same frame, if the effect is instanced
  if ((m_uiLastExtractedFrame != uiExtractedFrame)
      /*&& !bNeedsSorting*/) // TODO: in theory every shared instance has to sort the Quads, in practice this maybe should be an option
  {
    m_uiLastExtractedFrame = uiExtractedFrame;

    const bool bNeedsBillboardData = m_Orientation == ezQuadParticleOrientation::Billboard;
    const bool bNeedsTangentData = !bNeedsBillboardData;

    const ezVec3 vEmitterPos = GetOwnerSystem()->GetTransform().m_vPosition;
    const ezVec3 vEmitterDir = GetOwnerSystem()->GetTransform().m_qRotation * ezVec3(0, 0, 1); // Z axis
    const ezVec3 vEmitterDirOrtho = vEmitterDir.GetOrthogonalVector();

    const ezTime tCur = GetOwnerSystem()->GetWorld()->GetClock().GetAccumulatedTime();
    const ezColor tintColor = GetOwnerEffect()->GetColorParameter(m_sTintColorParameter, ezColor::White);

    const ezVec2* pLifeTime = m_pStreamLifeTime->GetData<ezVec2>();
    const ezVec4* pPosition = m_pStreamPosition->GetData<ezVec4>();
    const ezFloat16* pSize = m_pStreamSize->GetData<ezFloat16>();
    const ezColor* pColor = m_pStreamColor->GetData<ezColor>();
    const ezFloat16* pRotationSpeed = m_pStreamRotationSpeed->GetData<ezFloat16>();
    const ezFloat16* pRotationOffset = m_pStreamRotationOffset->GetData<ezFloat16>();

    // this will automatically be deallocated at the end of the frame
    m_BaseParticleData = EZ_NEW_ARRAY(ezFrameAllocator::GetCurrentAllocator(), ezBaseParticleShaderData, numParticles);

    AllocateParticleData(numParticles, bNeedsBillboardData, bNeedsTangentData);

    auto SetBaseData = [&](ezUInt32 dstIdx, ezUInt32 srcIdx) {

      m_BaseParticleData[dstIdx].Size = pSize[srcIdx];
      m_BaseParticleData[dstIdx].Color = pColor[srcIdx] * tintColor;
      m_BaseParticleData[dstIdx].Life = pLifeTime[srcIdx].x * pLifeTime[srcIdx].y;
    };

    auto SetBillboardData = [&](ezUInt32 dstIdx, ezUInt32 srcIdx) {

      ezTransform trans;

      trans.m_vPosition = pPosition[srcIdx].GetAsVec3();
      trans.m_qRotation.SetFromAxisAndAngle(ezVec3(0, 1, 0),
                                            ezAngle::Radian((float)(tCur.GetSeconds() * pRotationSpeed[srcIdx]) + pRotationOffset[srcIdx]));
      trans.m_vScale.Set(1.0f);

      m_BillboardParticleData[dstIdx].Transform = trans;
    };

    auto SetTangentDataEmitterDir = [&](ezUInt32 dstIdx, ezUInt32 srcIdx) {

      ezMat3 mRotation;
      mRotation.SetRotationMatrix(vEmitterDir,
                                  ezAngle::Radian((float)(tCur.GetSeconds() * pRotationSpeed[srcIdx]) + pRotationOffset[srcIdx]));

      m_TangentParticleData[dstIdx].Position = pPosition[srcIdx].GetAsVec3();
      m_TangentParticleData[dstIdx].TangentX = mRotation * vEmitterDirOrtho;
      m_TangentParticleData[dstIdx].TangentZ = vEmitterDir;
    };

    auto SetTangentDataEmitterDirOrtho = [&](ezUInt32 dstIdx, ezUInt32 srcIdx) {

      const ezVec3 vDirToParticle = (pPosition[srcIdx].GetAsVec3() - vEmitterPos);
      ezVec3 vOrthoDir = vEmitterDir.Cross(vDirToParticle);
      vOrthoDir.NormalizeIfNotZero(ezVec3(1, 0, 0));

      ezMat3 mRotation;
      mRotation.SetRotationMatrix(vOrthoDir, ezAngle::Radian((float)(tCur.GetSeconds() * pRotationSpeed[srcIdx]) + pRotationOffset[srcIdx]));

      m_TangentParticleData[dstIdx].Position = pPosition[srcIdx].GetAsVec3();
      m_TangentParticleData[dstIdx].TangentX = vOrthoDir;
      m_TangentParticleData[dstIdx].TangentZ = mRotation * vEmitterDir;
    };

    if (bNeedsSorting)
    {
      // TODO: Using the frame allocator this way results in memory corruptions.
      // Not sure, whether this is supposed to work.
      ezHybridArray<sod, 64> sorted; // (ezFrameAllocator::GetCurrentAllocator());
      sorted.SetCountUninitialized(numParticles);

      const ezVec3 vCameraPos = view.GetCamera()->GetCenterPosition();

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
        SetBaseData(p, idx);
      }

      if (bNeedsBillboardData)
      {
        for (ezUInt32 p = 0; p < numParticles; ++p)
        {
          const ezUInt32 idx = sorted[p].index;
          SetBillboardData(p, idx);
        }
      }

      if (bNeedsTangentData)
      {
        if (m_Orientation == ezQuadParticleOrientation::FragmentEmitterDirection)
        {
          for (ezUInt32 p = 0; p < numParticles; ++p)
          {
            const ezUInt32 idx = sorted[p].index;
            SetTangentDataEmitterDir(p, idx);
          }
        }
        else if (m_Orientation == ezQuadParticleOrientation::FragmentOrthogonalEmitterDirection)
        {
          for (ezUInt32 p = 0; p < numParticles; ++p)
          {
            const ezUInt32 idx = sorted[p].index;
            SetTangentDataEmitterDirOrtho(p, idx);
          }
        }
      }
    }
    else
    {
      for (ezUInt32 p = 0; p < numParticles; ++p)
      {
        const ezUInt32 idx = p;
        SetBaseData(p, idx);
      }

      if (bNeedsBillboardData)
      {
        for (ezUInt32 p = 0; p < numParticles; ++p)
        {
          const ezUInt32 idx = p;
          SetBillboardData(p, idx);
        }
      }

      if (bNeedsTangentData)
      {
        if (m_Orientation == ezQuadParticleOrientation::FragmentEmitterDirection)
        {
          for (ezUInt32 p = 0; p < numParticles; ++p)
          {
            const ezUInt32 idx = p;
            SetTangentDataEmitterDir(p, idx);
          }
        }
        else if (m_Orientation == ezQuadParticleOrientation::FragmentOrthogonalEmitterDirection)
        {
          for (ezUInt32 p = 0; p < numParticles; ++p)
          {
            const ezUInt32 idx = p;
            SetTangentDataEmitterDirOrtho(p, idx);
          }
        }
      }
    }
  }

  AddParticleRenderData(extractedRenderData, instanceTransform);
}


void ezParticleTypeQuad::AddParticleRenderData(ezExtractedRenderData& extractedRenderData, const ezTransform& instanceTransform) const
{
  /// \todo Is this batch ID correct?
  const ezUInt32 uiBatchId = m_hTexture.GetResourceIDHash();
  auto pRenderData = ezCreateRenderDataForThisFrame<ezParticleQuadRenderData>(nullptr, uiBatchId);

  pRenderData->m_bApplyObjectTransform = GetOwnerEffect()->NeedsToApplyTransform();
  pRenderData->m_GlobalTransform = instanceTransform;
  pRenderData->m_RenderMode = m_RenderMode;
  pRenderData->m_hTexture = m_hTexture;
  pRenderData->m_BaseParticleData = m_BaseParticleData;
  pRenderData->m_BillboardParticleData = m_BillboardParticleData;
  pRenderData->m_TangentParticleData = m_TangentParticleData;
  pRenderData->m_uiNumSpritesX = m_uiNumSpritesX;
  pRenderData->m_uiNumSpritesY = m_uiNumSpritesY;

  const ezUInt32 uiSortingKey = ComputeSortingKey(m_RenderMode);
  extractedRenderData.AddRenderData(pRenderData,
                                    /*m_RenderMode == ezParticleTypeRenderMode::Opaque ? ezDefaultRenderDataCategories::LitOpaque :*/
                                    ezDefaultRenderDataCategories::LitTransparent, uiSortingKey);
}

void ezParticleTypeQuad::AllocateParticleData(const ezUInt32 numParticles, const bool bNeedsBillboardData,
                                              const bool bNeedsTangentData) const
{
  m_BillboardParticleData = nullptr;
  if (bNeedsBillboardData)
  {
    m_BillboardParticleData = EZ_NEW_ARRAY(ezFrameAllocator::GetCurrentAllocator(), ezBillboardQuadParticleShaderData, numParticles);
  }

  m_TangentParticleData = nullptr;
  if (bNeedsTangentData)
  {
    m_TangentParticleData = EZ_NEW_ARRAY(ezFrameAllocator::GetCurrentAllocator(), ezTangentQuadParticleShaderData,
                                         (ezUInt32)GetOwnerSystem()->GetNumActiveParticles());
  }
}
