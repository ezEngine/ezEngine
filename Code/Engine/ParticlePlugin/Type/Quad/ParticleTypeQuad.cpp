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
    EZ_MEMBER_PROPERTY("Deviation", m_MaxDeviation)->AddAttributes(new ezClampValueAttribute(ezAngle::Degree(0), ezAngle::Degree(90))),
    EZ_ENUM_MEMBER_PROPERTY("RenderMode", ezParticleTypeRenderMode, m_RenderMode),
    EZ_MEMBER_PROPERTY("Texture", m_sTexture)->AddAttributes(new ezAssetBrowserAttribute("Texture 2D")),
    EZ_MEMBER_PROPERTY("NumSpritesX", m_uiNumSpritesX)->AddAttributes(new ezDefaultValueAttribute(1), new ezClampValueAttribute(1, 16)),
    EZ_MEMBER_PROPERTY("NumSpritesY", m_uiNumSpritesY)->AddAttributes(new ezDefaultValueAttribute(1), new ezClampValueAttribute(1, 16)),
    EZ_MEMBER_PROPERTY("TintColorParam", m_sTintColorParameter),
    EZ_MEMBER_PROPERTY("DistortionTexture", m_sDistortionTexture)->AddAttributes(new ezAssetBrowserAttribute("Texture 2D")),
    EZ_MEMBER_PROPERTY("DistortionStrength", m_fDistortionStrength)->AddAttributes(new ezDefaultValueAttribute(100.0f), new ezClampValueAttribute(0.0f, 500.0f)),
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
  pType->m_MaxDeviation = m_MaxDeviation;
  pType->m_hTexture.Invalidate();
  pType->m_RenderMode = m_RenderMode;
  pType->m_uiNumSpritesX = m_uiNumSpritesX;
  pType->m_uiNumSpritesY = m_uiNumSpritesY;
  pType->m_sTintColorParameter = ezTempHashedString(m_sTintColorParameter.GetData());
  pType->m_hDistortionTexture.Invalidate();
  pType->m_fDistortionStrength = m_fDistortionStrength;

  if (!m_sTexture.IsEmpty())
    pType->m_hTexture = ezResourceManager::LoadResource<ezTexture2DResource>(m_sTexture);
  if (!m_sDistortionTexture.IsEmpty())
    pType->m_hDistortionTexture = ezResourceManager::LoadResource<ezTexture2DResource>(m_sDistortionTexture);
}

enum class TypeQuadVersion
{
  Version_0 = 0,
  Version_1,
  Version_2, // sprite deviation
  Version_3, // distortion

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
  stream << m_MaxDeviation;
  stream << m_sDistortionTexture;
  stream << m_fDistortionStrength;
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

  if (uiVersion >= 2)
  {
    stream >> m_MaxDeviation;
  }

  if (uiVersion >= 3)
  {
    stream >> m_sDistortionTexture;
    stream >> m_fDistortionStrength;
  }
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

  if (m_Orientation == ezQuadParticleOrientation::SpriteRandom || m_Orientation == ezQuadParticleOrientation::SpriteEmitterDirection ||
      m_Orientation == ezQuadParticleOrientation::SpriteWorldUp)
  {
    CreateStream("Axis", ezProcessingStream::DataType::Float3, &m_pStreamAxis, true);
  }
}

struct sodComparer
{
  // sort farther particles to the front, so that they get rendered first (back to front)
  EZ_ALWAYS_INLINE bool Less(const ezParticleTypeQuad::sod& a, const ezParticleTypeQuad::sod& b) const { return a.dist > b.dist; }
  EZ_ALWAYS_INLINE bool Equal(const ezParticleTypeQuad::sod& a, const ezParticleTypeQuad::sod& b) const { return a.dist == b.dist; }
};

void ezParticleTypeQuad::ExtractTypeRenderData(const ezView& view, ezExtractedRenderData& extractedRenderData,
                                               const ezTransform& instanceTransform, ezUInt64 uiExtractedFrame) const
{
  EZ_PROFILE("PFX: Quad");

  const ezUInt32 numParticles = (ezUInt32)GetOwnerSystem()->GetNumActiveParticles();
  if (!m_hTexture.IsValid() || numParticles == 0)
    return;

  const bool bNeedsSorting = m_RenderMode == ezParticleTypeRenderMode::Blended;

  // don't copy the data multiple times in the same frame, if the effect is instanced
  if ((m_uiLastExtractedFrame != uiExtractedFrame)
      /*&& !bNeedsSorting*/) // TODO: in theory every shared instance has to sort the Quads, in practice this maybe should be an option
  {
    m_uiLastExtractedFrame = uiExtractedFrame;

    if (bNeedsSorting)
    {
      // TODO: Using the frame allocator this way results in memory corruptions.
      // Not sure, whether this is supposed to work.
      ezHybridArray<sod, 64> sorted; // (ezFrameAllocator::GetCurrentAllocator());
      sorted.SetCountUninitialized(numParticles);

      const ezVec3 vCameraPos = view.GetCamera()->GetCenterPosition();
      const ezVec4* pPosition = m_pStreamPosition->GetData<ezVec4>();

      for (ezUInt32 p = 0; p < numParticles; ++p)
      {
        sorted[p].dist = (pPosition[p].GetAsVec3() - vCameraPos).GetLengthSquared();
        sorted[p].index = p;
      }

      sorted.Sort(sodComparer());

      auto redirectIdx = [&sorted](ezUInt32 idx) -> ezUInt32 { return sorted[idx].index; };

      CreateExtractedData(view, extractedRenderData, instanceTransform, uiExtractedFrame, &sorted);
    }
    else
    {
      auto redirectIdx = [](ezUInt32 idx) -> ezUInt32 { return idx; };

      CreateExtractedData(view, extractedRenderData, instanceTransform, uiExtractedFrame, nullptr);
    }
  }

  AddParticleRenderData(extractedRenderData, instanceTransform);
}

ezUInt32 noRedirect(ezUInt32 idx, const ezHybridArray<ezParticleTypeQuad::sod, 64>* pSorted)
{
  return idx;
}

ezUInt32 sortedRedirect(ezUInt32 idx, const ezHybridArray<ezParticleTypeQuad::sod, 64>* pSorted)
{
  return (*pSorted)[idx].index;
}

void ezParticleTypeQuad::CreateExtractedData(const ezView& view, ezExtractedRenderData& extractedRenderData,
                                             const ezTransform& instanceTransform, ezUInt64 uiExtractedFrame,
                                             const ezHybridArray<sod, 64>* pSorted) const
{
  auto redirect = (pSorted != nullptr) ? sortedRedirect : noRedirect;

  const ezUInt32 numParticles = (ezUInt32)GetOwnerSystem()->GetNumActiveParticles();

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
  const ezVec3* pAxis = m_pStreamAxis ? m_pStreamAxis->GetData<ezVec3>() : nullptr;

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

  auto SetTangentDataFromAxis = [&](ezUInt32 dstIdx, ezUInt32 srcIdx) {

    ezVec3 vNormal = pAxis[srcIdx];
    vNormal.Normalize();

    const ezVec3 vTangentStart = vNormal.GetOrthogonalVector().GetNormalized();

    ezMat3 mRotation;
    mRotation.SetRotationMatrix(vNormal, ezAngle::Radian((float)(tCur.GetSeconds() * pRotationSpeed[srcIdx]) + pRotationOffset[srcIdx]));

    const ezVec3 vTangentX = mRotation * vTangentStart;

    m_TangentParticleData[dstIdx].Position = pPosition[srcIdx].GetAsVec3();
    m_TangentParticleData[dstIdx].TangentX = vTangentX;
    m_TangentParticleData[dstIdx].TangentZ = vTangentX.Cross(vNormal);
  };

  for (ezUInt32 p = 0; p < numParticles; ++p)
  {
    SetBaseData(p, redirect(p, pSorted));
  }

  if (bNeedsBillboardData)
  {
    for (ezUInt32 p = 0; p < numParticles; ++p)
    {
      SetBillboardData(p, redirect(p, pSorted));
    }
  }

  if (bNeedsTangentData)
  {
    if (m_Orientation == ezQuadParticleOrientation::FragmentEmitterDirection)
    {
      for (ezUInt32 p = 0; p < numParticles; ++p)
      {
        SetTangentDataEmitterDir(p, redirect(p, pSorted));
      }
    }
    else if (m_Orientation == ezQuadParticleOrientation::FragmentOrthogonalEmitterDirection)
    {
      for (ezUInt32 p = 0; p < numParticles; ++p)
      {
        SetTangentDataEmitterDirOrtho(p, redirect(p, pSorted));
      }
    }
    else if (m_Orientation == ezQuadParticleOrientation::SpriteEmitterDirection ||
             m_Orientation == ezQuadParticleOrientation::SpriteRandom || m_Orientation == ezQuadParticleOrientation::SpriteWorldUp)
    {
      for (ezUInt32 p = 0; p < numParticles; ++p)
      {
        SetTangentDataFromAxis(p, redirect(p, pSorted));
      }
    }
    else
    {
      EZ_ASSERT_NOT_IMPLEMENTED;
    }
  }
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
  pRenderData->m_hDistortionTexture = m_hDistortionTexture;
  pRenderData->m_fDistortionStrength = m_fDistortionStrength;

  const ezUInt32 uiSortingKey = ComputeSortingKey(m_RenderMode);
  extractedRenderData.AddRenderData(pRenderData,
                                    /*m_RenderMode == ezParticleTypeRenderMode::Opaque ? ezDefaultRenderDataCategories::LitOpaque :*/
                                    ezDefaultRenderDataCategories::LitTransparent, uiSortingKey);
}

void ezParticleTypeQuad::InitializeElements(ezUInt64 uiStartIndex, ezUInt64 uiNumElements)
{
  if (m_pStreamAxis == nullptr)
    return;

  ezVec3* pAxis = m_pStreamAxis->GetWritableData<ezVec3>();
  ezRandom& rng = GetRNG();

  if (m_Orientation == ezQuadParticleOrientation::SpriteRandom)
  {
    EZ_PROFILE("PFX: Init Quad Axis Random");

    for (ezUInt32 i = 0; i < uiNumElements; ++i)
    {
      const ezUInt64 uiElementIdx = uiStartIndex + i;

      pAxis[uiElementIdx] = ezVec3::CreateRandomDirection(rng);
    }
  }
  else if (m_Orientation == ezQuadParticleOrientation::SpriteEmitterDirection || m_Orientation == ezQuadParticleOrientation::SpriteWorldUp)
  {
    EZ_PROFILE("PFX: Init Quad Axis");

    ezVec3 vNormal;

    if (m_Orientation == ezQuadParticleOrientation::SpriteEmitterDirection)
    {
      vNormal = GetOwnerSystem()->GetTransform().m_qRotation * ezVec3(0, 0, 1); // Z axis
    }
    else if (m_Orientation == ezQuadParticleOrientation::SpriteWorldUp)
    {
      ezCoordinateSystem coord;
      GetOwnerSystem()->GetWorld()->GetCoordinateSystem(GetOwnerSystem()->GetTransform().m_vPosition, coord);

      vNormal = coord.m_vUpDir;
    }

    if (m_MaxDeviation > ezAngle::Degree(1.0f))
    {
      // how to get from the X axis to the desired normal
      ezQuat qRotToDir;
      qRotToDir.SetShortestRotation(ezVec3(1, 0, 0), vNormal);

      for (ezUInt32 i = 0; i < uiNumElements; ++i)
      {
        const ezUInt64 uiElementIdx = uiStartIndex + i;
        const ezVec3 vRandomX = ezVec3::CreateRandomDeviationX(rng, m_MaxDeviation);

        pAxis[uiElementIdx] = qRotToDir * vRandomX;
      }
    }
    else
    {
      for (ezUInt32 i = 0; i < uiNumElements; ++i)
      {
        const ezUInt64 uiElementIdx = uiStartIndex + i;
        pAxis[uiElementIdx] = vNormal;
      }
    }
  }
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
