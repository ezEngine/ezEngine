#include <ParticlePlugin/ParticlePluginPCH.h>

#include <ParticlePlugin/Type/Quad/ParticleTypeQuad.h>

#include <Core/World/World.h>
#include <Foundation/Math/Color16f.h>
#include <Foundation/Math/Float16.h>
#include <Foundation/Profiling/Profiling.h>
#include <ParticlePlugin/Effect/ParticleEffectInstance.h>
#include <ParticlePlugin/Finalizer/ParticleFinalizer_LastPosition.h>
#include <RendererCore/Pipeline/View.h>
#include <RendererCore/RenderWorld/RenderWorld.h>
#include <RendererFoundation/Shader/ShaderUtils.h>

// clang-format off
EZ_BEGIN_STATIC_REFLECTED_ENUM(ezQuadParticleOrientation, 2)
  EZ_ENUM_CONSTANTS(ezQuadParticleOrientation::Billboard)
  EZ_ENUM_CONSTANTS(ezQuadParticleOrientation::Rotating_OrthoEmitterDir, ezQuadParticleOrientation::Rotating_EmitterDir)
  EZ_ENUM_CONSTANTS(ezQuadParticleOrientation::Fixed_EmitterDir, ezQuadParticleOrientation::Fixed_RandomDir, ezQuadParticleOrientation::Fixed_WorldUp)
  EZ_ENUM_CONSTANTS(ezQuadParticleOrientation::FixedAxis_EmitterDir, ezQuadParticleOrientation::FixedAxis_ParticleDir)
EZ_END_STATIC_REFLECTED_ENUM;

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezParticleTypeQuadFactory, 2, ezRTTIDefaultAllocator<ezParticleTypeQuadFactory>)
{
  EZ_BEGIN_PROPERTIES
  {
    EZ_ENUM_MEMBER_PROPERTY("Orientation", ezQuadParticleOrientation, m_Orientation),
    EZ_MEMBER_PROPERTY("Deviation", m_MaxDeviation)->AddAttributes(new ezClampValueAttribute(ezAngle::MakeFromDegree(0), ezAngle::MakeFromDegree(90))),
    EZ_ENUM_MEMBER_PROPERTY("RenderMode", ezParticleTypeRenderMode, m_RenderMode),
    EZ_ENUM_MEMBER_PROPERTY("LightingMode", ezParticleLightingMode, m_LightingMode),
    EZ_MEMBER_PROPERTY("NormalCurvature", m_fNormalCurvature)->AddAttributes(new ezDefaultValueAttribute(0.5f), new ezClampValueAttribute(0, 1)),
    EZ_MEMBER_PROPERTY("LightDirectionality", m_fLightDirectionality)->AddAttributes(new ezDefaultValueAttribute(0.5f), new ezClampValueAttribute(0, 1)),
    EZ_MEMBER_PROPERTY("Texture", m_sTexture)->AddAttributes(new ezAssetBrowserAttribute("CompatibleAsset_Texture_2D"), new ezDefaultValueAttribute(ezStringView("{ e00262e8-58f5-42f5-880d-569257047201 }"))),// wrap in ezStringView to prevent a memory leak report
    EZ_ENUM_MEMBER_PROPERTY("TextureAtlas", ezParticleTextureAtlasType, m_TextureAtlasType),
    EZ_MEMBER_PROPERTY("NumSpritesX", m_uiNumSpritesX)->AddAttributes(new ezDefaultValueAttribute(1), new ezClampValueAttribute(1, 16)),
    EZ_MEMBER_PROPERTY("NumSpritesY", m_uiNumSpritesY)->AddAttributes(new ezDefaultValueAttribute(1), new ezClampValueAttribute(1, 16)),
    EZ_MEMBER_PROPERTY("TintColorParam", m_sTintColorParameter),
    EZ_MEMBER_PROPERTY("DistortionTexture", m_sDistortionTexture)->AddAttributes(new ezAssetBrowserAttribute("CompatibleAsset_Texture_2D")),
    EZ_MEMBER_PROPERTY("DistortionStrength", m_fDistortionStrength)->AddAttributes(new ezDefaultValueAttribute(100.0f), new ezClampValueAttribute(0.0f, 500.0f)),
    EZ_MEMBER_PROPERTY("ParticleStretch", m_fStretch)->AddAttributes(new ezDefaultValueAttribute(1.0f), new ezClampValueAttribute(-100.0f, 100.0f)),
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

void ezParticleTypeQuadFactory::CopyTypeProperties(ezParticleType* pObject, bool bFirstTime) const
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
  pType->m_TextureAtlasType = m_TextureAtlasType;
  pType->m_fStretch = m_fStretch;
  pType->m_LightingMode = m_LightingMode;
  pType->m_fNormalCurvature = m_fNormalCurvature;
  pType->m_fLightDirectionality = m_fLightDirectionality;

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
  Version_4, // added texture atlas type
  Version_5, // added particle stretch
  Version_6, // added particle lighting

  // insert new version numbers above
  Version_Count,
  Version_Current = Version_Count - 1
};

void ezParticleTypeQuadFactory::Save(ezStreamWriter& inout_stream) const
{
  const ezUInt8 uiVersion = (int)TypeQuadVersion::Version_Current;
  inout_stream << uiVersion;

  inout_stream << m_Orientation;
  inout_stream << m_RenderMode;
  inout_stream << m_sTexture;
  inout_stream << m_uiNumSpritesX;
  inout_stream << m_uiNumSpritesY;
  inout_stream << m_sTintColorParameter;
  inout_stream << m_MaxDeviation;
  inout_stream << m_sDistortionTexture;
  inout_stream << m_fDistortionStrength;
  inout_stream << m_TextureAtlasType;

  // Version 5
  inout_stream << m_fStretch;

  // Version 6
  inout_stream << m_LightingMode;
  inout_stream << m_fNormalCurvature;
  inout_stream << m_fLightDirectionality;
}

void ezParticleTypeQuadFactory::Load(ezStreamReader& inout_stream)
{
  ezUInt8 uiVersion = 0;
  inout_stream >> uiVersion;

  EZ_ASSERT_DEV(uiVersion <= (int)TypeQuadVersion::Version_Current, "Invalid version {0}", uiVersion);

  inout_stream >> m_Orientation;
  inout_stream >> m_RenderMode;
  inout_stream >> m_sTexture;
  inout_stream >> m_uiNumSpritesX;
  inout_stream >> m_uiNumSpritesY;
  inout_stream >> m_sTintColorParameter;

  if (uiVersion >= 2)
  {
    inout_stream >> m_MaxDeviation;
  }

  if (uiVersion >= 3)
  {
    inout_stream >> m_sDistortionTexture;
    inout_stream >> m_fDistortionStrength;
  }

  if (uiVersion >= 4)
  {
    inout_stream >> m_TextureAtlasType;

    if (m_TextureAtlasType == ezParticleTextureAtlasType::None)
    {
      m_uiNumSpritesX = 1;
      m_uiNumSpritesY = 1;
    }
  }

  if (uiVersion >= 5)
  {
    inout_stream >> m_fStretch;
  }

  if (uiVersion >= 6)
  {
    inout_stream >> m_LightingMode;
    inout_stream >> m_fNormalCurvature;
    inout_stream >> m_fLightDirectionality;
  }
}

void ezParticleTypeQuadFactory::QueryFinalizerDependencies(ezSet<const ezRTTI*>& inout_finalizerDeps) const
{
  if (m_Orientation == ezQuadParticleOrientation::FixedAxis_ParticleDir)
  {
    inout_finalizerDeps.Insert(ezGetStaticRTTI<ezParticleFinalizerFactory_LastPosition>());
  }
}

ezParticleTypeQuad::ezParticleTypeQuad() = default;
ezParticleTypeQuad::~ezParticleTypeQuad() = default;

void ezParticleTypeQuad::CreateRequiredStreams()
{
  CreateStream("LifeTime", ezProcessingStream::DataType::Half2, &m_pStreamLifeTime, false);
  CreateStream("Position", ezProcessingStream::DataType::Float4, &m_pStreamPosition, false);
  CreateStream("Size", ezProcessingStream::DataType::Half, &m_pStreamSize, false);
  CreateStream("Color", ezProcessingStream::DataType::Half4, &m_pStreamColor, false);
  CreateStream("RotationSpeed", ezProcessingStream::DataType::Half, &m_pStreamRotationSpeed, false);
  CreateStream("RotationOffset", ezProcessingStream::DataType::Half, &m_pStreamRotationOffset, false);

  m_pStreamAxis = nullptr;
  m_pStreamVariation = nullptr;
  m_pStreamLastPosition = nullptr;

  if (m_Orientation == ezQuadParticleOrientation::Fixed_RandomDir || m_Orientation == ezQuadParticleOrientation::Fixed_EmitterDir || m_Orientation == ezQuadParticleOrientation::Fixed_WorldUp)
  {
    CreateStream("Axis", ezProcessingStream::DataType::Float3, &m_pStreamAxis, true);
  }

  if (m_TextureAtlasType == ezParticleTextureAtlasType::RandomVariations || m_TextureAtlasType == ezParticleTextureAtlasType::RandomYAnimatedX)
  {
    CreateStream("Variation", ezProcessingStream::DataType::Int, &m_pStreamVariation, false);
  }

  if (m_Orientation == ezQuadParticleOrientation::FixedAxis_ParticleDir)
  {
    CreateStream("LastPosition", ezProcessingStream::DataType::Float3, &m_pStreamLastPosition, false);
  }
}

struct sodComparer
{
  // sort farther particles to the front, so that they get rendered first (back to front)
  EZ_ALWAYS_INLINE bool Less(const ezParticleTypeQuad::sod& a, const ezParticleTypeQuad::sod& b) const { return a.dist > b.dist; }
  EZ_ALWAYS_INLINE bool Equal(const ezParticleTypeQuad::sod& a, const ezParticleTypeQuad::sod& b) const { return a.dist == b.dist; }
};

void ezParticleTypeQuad::ExtractTypeRenderData(ezMsgExtractRenderData& ref_msg, const ezTransform& instanceTransform) const
{
  const ezUInt32 numParticles = (ezUInt32)GetOwnerSystem()->GetNumActiveParticles();
  if (!m_hTexture.IsValid() || numParticles == 0)
    return;

  const bool bNeedsSorting = (m_RenderMode == ezParticleTypeRenderMode::Blended) || (m_RenderMode == ezParticleTypeRenderMode::BlendedForeground) || (m_RenderMode == ezParticleTypeRenderMode::BlendedBackground) || (m_RenderMode == ezParticleTypeRenderMode::BlendAdd);

  // don't copy the data multiple times in the same frame, if the effect is instanced
  if ((m_uiLastExtractedFrame != ezRenderWorld::GetFrameCounter())
    /*&& !bNeedsSorting*/) // TODO: in theory every shared instance has to sort the Quads, in practice this maybe should be an option
  {
    m_uiLastExtractedFrame = ezRenderWorld::GetFrameCounter();

    if (bNeedsSorting)
    {
      // TODO: Using the frame allocator this way results in memory corruptions.
      // Not sure, whether this is supposed to work.
      ezHybridArray<sod, 64> sorted; // (ezFrameAllocator::GetCurrentAllocator());
      sorted.SetCountUninitialized(numParticles);

      const ezVec3 vCameraPos = ref_msg.m_pView->GetCullingCamera()->GetCenterPosition();
      const ezVec4* pPosition = m_pStreamPosition->GetData<ezVec4>();

      for (ezUInt32 p = 0; p < numParticles; ++p)
      {
        sorted[p].dist = (pPosition[p].GetAsVec3() - vCameraPos).GetLengthSquared();
        sorted[p].index = p;
      }

      sorted.Sort(sodComparer());

      CreateExtractedData(&sorted);
    }
    else
    {
      CreateExtractedData(nullptr);
    }
  }

  AddParticleRenderData(ref_msg, instanceTransform);
}

EZ_ALWAYS_INLINE ezUInt32 noRedirect(ezUInt32 uiIdx, const ezHybridArray<ezParticleTypeQuad::sod, 64>* pSorted)
{
  return uiIdx;
}

EZ_ALWAYS_INLINE ezUInt32 sortedRedirect(ezUInt32 uiIdx, const ezHybridArray<ezParticleTypeQuad::sod, 64>* pSorted)
{
  return (*pSorted)[uiIdx].index;
}

void ezParticleTypeQuad::CreateExtractedData(const ezHybridArray<sod, 64>* pSorted) const
{
  auto redirect = (pSorted != nullptr) ? sortedRedirect : noRedirect;

  const ezUInt32 numParticles = (ezUInt32)GetOwnerSystem()->GetNumActiveParticles();

  const bool bNeedsBillboardData = m_Orientation == ezQuadParticleOrientation::Billboard;
  const bool bNeedsTangentData = !bNeedsBillboardData;

  const ezVec3 vEmitterPos = GetOwnerSystem()->GetTransform().m_vPosition;
  const ezVec3 vEmitterDir = GetOwnerSystem()->GetTransform().m_qRotation * ezVec3(0, 0, 1); // Z axis
  const ezVec3 vEmitterDirOrtho = vEmitterDir.GetOrthogonalVector();

  const ezTime tCur = GetOwnerEffect()->GetTotalEffectLifeTime();
  const ezColor tintColor = GetOwnerEffect()->GetColorParameter(m_sTintColorParameter, ezColor::White);

  const ezFloat16Vec2* pLifeTime = m_pStreamLifeTime->GetData<ezFloat16Vec2>();
  const ezVec4* pPosition = m_pStreamPosition->GetData<ezVec4>();
  const ezFloat16* pSize = m_pStreamSize->GetData<ezFloat16>();
  const ezColorLinear16f* pColor = m_pStreamColor->GetData<ezColorLinear16f>();
  const ezFloat16* pRotationSpeed = m_pStreamRotationSpeed->GetData<ezFloat16>();
  const ezFloat16* pRotationOffset = m_pStreamRotationOffset->GetData<ezFloat16>();
  const ezVec3* pAxis = m_pStreamAxis ? m_pStreamAxis->GetData<ezVec3>() : nullptr;
  const ezUInt32* pVariation = m_pStreamVariation ? m_pStreamVariation->GetData<ezUInt32>() : nullptr;
  const ezVec3* pLastPosition = m_pStreamLastPosition ? m_pStreamLastPosition->GetData<ezVec3>() : nullptr;

  // this will automatically be deallocated at the end of the frame
  m_BaseParticleData = EZ_NEW_ARRAY(ezFrameAllocator::GetCurrentAllocator(), ezBaseParticleShaderData, numParticles);

  AllocateParticleData(numParticles, bNeedsBillboardData, bNeedsTangentData);

  auto SetBaseData = [&](ezUInt32 uiDstIdx, ezUInt32 uiSrcIdx)
  {
    m_BaseParticleData[uiDstIdx].Size = pSize[uiSrcIdx];
    m_BaseParticleData[uiDstIdx].Color = pColor[uiSrcIdx].ToLinearFloat() * tintColor;
    m_BaseParticleData[uiDstIdx].Life = pLifeTime[uiSrcIdx].x * pLifeTime[uiSrcIdx].y;
    m_BaseParticleData[uiDstIdx].Variation = (pVariation != nullptr) ? pVariation[uiSrcIdx] : 0;
  };

  auto SetBillboardData = [&](ezUInt32 uiDstIdx, ezUInt32 uiSrcIdx)
  {
    m_BillboardParticleData[uiDstIdx].Position = pPosition[uiSrcIdx].GetAsVec3();
    m_BillboardParticleData[uiDstIdx].RotationOffset = pRotationOffset[uiSrcIdx];
    m_BillboardParticleData[uiDstIdx].RotationSpeed = pRotationSpeed[uiSrcIdx];
  };

  auto SetTangentDataEmitterDir = [&](ezUInt32 uiDstIdx, ezUInt32 uiSrcIdx)
  {
    ezMat3 mRotation = ezMat3::MakeAxisRotation(vEmitterDir, ezAngle::MakeFromRadian((float)(tCur.GetSeconds() * pRotationSpeed[uiSrcIdx]) + pRotationOffset[uiSrcIdx]));

    m_TangentParticleData[uiDstIdx].Position = pPosition[uiSrcIdx].GetAsVec3();
    m_TangentParticleData[uiDstIdx].TangentX = mRotation * vEmitterDirOrtho;
    m_TangentParticleData[uiDstIdx].TangentZ = vEmitterDir;
  };

  auto SetTangentDataEmitterDirOrtho = [&](ezUInt32 uiDstIdx, ezUInt32 uiSrcIdx)
  {
    const ezVec3 vDirToParticle = (pPosition[uiSrcIdx].GetAsVec3() - vEmitterPos);
    ezVec3 vOrthoDir = vEmitterDir.CrossRH(vDirToParticle);
    vOrthoDir.NormalizeIfNotZero(ezVec3(1, 0, 0)).IgnoreResult();

    ezMat3 mRotation = ezMat3::MakeAxisRotation(vOrthoDir, ezAngle::MakeFromRadian((float)(tCur.GetSeconds() * pRotationSpeed[uiSrcIdx]) + pRotationOffset[uiSrcIdx]));

    m_TangentParticleData[uiDstIdx].Position = pPosition[uiSrcIdx].GetAsVec3();
    m_TangentParticleData[uiDstIdx].TangentX = vOrthoDir;
    m_TangentParticleData[uiDstIdx].TangentZ = mRotation * vEmitterDir;
  };

  auto SetTangentDataFromAxis = [&](ezUInt32 uiDstIdx, ezUInt32 uiSrcIdx)
  {
    EZ_ASSERT_DEBUG(pAxis != nullptr, "Axis must be valid");
    ezVec3 vNormal = pAxis[uiSrcIdx];
    vNormal.Normalize();

    const ezVec3 vTangentStart = vNormal.GetOrthogonalVector().GetNormalized();

    ezMat3 mRotation = ezMat3::MakeAxisRotation(vNormal, ezAngle::MakeFromRadian((float)(tCur.GetSeconds() * pRotationSpeed[uiSrcIdx]) + pRotationOffset[uiSrcIdx]));

    const ezVec3 vTangentX = mRotation * vTangentStart;

    m_TangentParticleData[uiDstIdx].Position = pPosition[uiSrcIdx].GetAsVec3();
    m_TangentParticleData[uiDstIdx].TangentX = vTangentX;
    m_TangentParticleData[uiDstIdx].TangentZ = vTangentX.CrossRH(vNormal);
  };

  auto SetTangentDataAligned_Emitter = [&](ezUInt32 uiDstIdx, ezUInt32 uiSrcIdx)
  {
    m_TangentParticleData[uiDstIdx].Position = pPosition[uiSrcIdx].GetAsVec3();
    m_TangentParticleData[uiDstIdx].TangentX = vEmitterDir;
    m_TangentParticleData[uiDstIdx].TangentZ.x = m_fStretch;
  };

  auto SetTangentDataAligned_ParticleDir = [&](ezUInt32 uiDstIdx, ezUInt32 uiSrcIdx)
  {
    const ezVec3 vCurPos = pPosition[uiSrcIdx].GetAsVec3();
    const ezVec3 vLastPos = pLastPosition[uiSrcIdx];
    const ezVec3 vDir = vCurPos - vLastPos;
    m_TangentParticleData[uiDstIdx].Position = vCurPos;
    m_TangentParticleData[uiDstIdx].TangentX = vDir;
    m_TangentParticleData[uiDstIdx].TangentZ.x = m_fStretch;
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
    if (m_Orientation == ezQuadParticleOrientation::Rotating_EmitterDir)
    {
      for (ezUInt32 p = 0; p < numParticles; ++p)
      {
        SetTangentDataEmitterDir(p, redirect(p, pSorted));
      }
    }
    else if (m_Orientation == ezQuadParticleOrientation::Rotating_OrthoEmitterDir)
    {
      for (ezUInt32 p = 0; p < numParticles; ++p)
      {
        SetTangentDataEmitterDirOrtho(p, redirect(p, pSorted));
      }
    }
    else if (m_Orientation == ezQuadParticleOrientation::Fixed_EmitterDir || m_Orientation == ezQuadParticleOrientation::Fixed_RandomDir || m_Orientation == ezQuadParticleOrientation::Fixed_WorldUp)
    {
      for (ezUInt32 p = 0; p < numParticles; ++p)
      {
        SetTangentDataFromAxis(p, redirect(p, pSorted));
      }
    }
    else if (m_Orientation == ezQuadParticleOrientation::FixedAxis_EmitterDir)
    {
      for (ezUInt32 p = 0; p < numParticles; ++p)
      {
        SetTangentDataAligned_Emitter(p, redirect(p, pSorted));
      }
    }
    else if (m_Orientation == ezQuadParticleOrientation::FixedAxis_ParticleDir)
    {
      EZ_ASSERT_DEBUG(pLastPosition != nullptr, "FixedAxis_ParticleDir needs the last position attribute");
      for (ezUInt32 p = 0; p < numParticles; ++p)
      {
        SetTangentDataAligned_ParticleDir(p, redirect(p, pSorted));
      }
    }
    else
    {
      EZ_ASSERT_NOT_IMPLEMENTED;
    }
  }
}

void ezParticleTypeQuad::AddParticleRenderData(ezMsgExtractRenderData& msg, const ezTransform& instanceTransform) const
{
  auto pRenderData = ezCreateRenderDataForThisFrame<ezParticleQuadRenderData>(nullptr);

  pRenderData->m_uiSortingKey = ComputeSortingKey(m_RenderMode, m_hTexture.GetResourceIDHash());

  pRenderData->m_bApplyObjectTransform = GetOwnerEffect()->NeedsToApplyTransform();
  pRenderData->m_GlobalTransform = instanceTransform;
  pRenderData->m_TotalEffectLifeTime = GetOwnerEffect()->GetTotalEffectLifeTime();
  pRenderData->m_RenderMode = m_RenderMode;
  pRenderData->m_hTexture = m_hTexture;
  pRenderData->m_BaseParticleData = m_BaseParticleData;
  pRenderData->m_BillboardParticleData = m_BillboardParticleData;
  pRenderData->m_TangentParticleData = m_TangentParticleData;
  pRenderData->m_uiNumVariationsX = 1;
  pRenderData->m_uiNumVariationsY = 1;
  pRenderData->m_uiNumFlipbookAnimationsX = 1;
  pRenderData->m_uiNumFlipbookAnimationsY = 1;
  pRenderData->m_hDistortionTexture = m_hDistortionTexture;
  pRenderData->m_fDistortionStrength = m_fDistortionStrength;
  pRenderData->m_LightingMode = m_LightingMode;
  pRenderData->m_fNormalCurvature = m_fNormalCurvature;
  pRenderData->m_fLightDirectionality = m_fLightDirectionality;

  switch (m_Orientation)
  {
    case ezQuadParticleOrientation::Billboard:
      pRenderData->m_QuadModePermutation = "PARTICLE_QUAD_MODE_BILLBOARD";
      break;
    case ezQuadParticleOrientation::Rotating_OrthoEmitterDir:
    case ezQuadParticleOrientation::Rotating_EmitterDir:
    case ezQuadParticleOrientation::Fixed_EmitterDir:
    case ezQuadParticleOrientation::Fixed_WorldUp:
    case ezQuadParticleOrientation::Fixed_RandomDir:
      pRenderData->m_QuadModePermutation = "PARTICLE_QUAD_MODE_TANGENTS";
      break;
    case ezQuadParticleOrientation::FixedAxis_EmitterDir:
    case ezQuadParticleOrientation::FixedAxis_ParticleDir:
      pRenderData->m_QuadModePermutation = "PARTICLE_QUAD_MODE_AXIS_ALIGNED";
      break;
  }

  switch (m_TextureAtlasType)
  {
    case ezParticleTextureAtlasType::None:
      break;

    case ezParticleTextureAtlasType::RandomVariations:
      pRenderData->m_uiNumVariationsX = m_uiNumSpritesX;
      pRenderData->m_uiNumVariationsY = m_uiNumSpritesY;
      break;

    case ezParticleTextureAtlasType::FlipbookAnimation:
      pRenderData->m_uiNumFlipbookAnimationsX = m_uiNumSpritesX;
      pRenderData->m_uiNumFlipbookAnimationsY = m_uiNumSpritesY;
      break;

    case ezParticleTextureAtlasType::RandomYAnimatedX:
      pRenderData->m_uiNumFlipbookAnimationsX = m_uiNumSpritesX;
      pRenderData->m_uiNumVariationsY = m_uiNumSpritesY;
      break;
  }

  msg.AddRenderData(pRenderData, ezDefaultRenderDataCategories::LitTransparent, ezRenderData::Caching::Never);
}

void ezParticleTypeQuad::InitializeElements(ezUInt64 uiStartIndex, ezUInt64 uiNumElements)
{
  if (m_pStreamAxis != nullptr)
  {
    ezVec3* pAxis = m_pStreamAxis->GetWritableData<ezVec3>();
    ezRandom& rng = GetRNG();

    if (m_Orientation == ezQuadParticleOrientation::Fixed_RandomDir)
    {
      EZ_PROFILE_SCOPE("PFX: Init Quad Axis Random");

      for (ezUInt32 i = 0; i < uiNumElements; ++i)
      {
        const ezUInt64 uiElementIdx = uiStartIndex + i;

        pAxis[uiElementIdx] = ezVec3::MakeRandomDirection(rng);
      }
    }
    else if (m_Orientation == ezQuadParticleOrientation::Fixed_EmitterDir || m_Orientation == ezQuadParticleOrientation::Fixed_WorldUp)
    {
      EZ_PROFILE_SCOPE("PFX: Init Quad Axis");

      ezVec3 vNormal;

      if (m_Orientation == ezQuadParticleOrientation::Fixed_EmitterDir)
      {
        vNormal = GetOwnerSystem()->GetTransform().m_qRotation * ezVec3(0, 0, 1); // Z axis
      }
      else if (m_Orientation == ezQuadParticleOrientation::Fixed_WorldUp)
      {
        ezCoordinateSystem coord;
        GetOwnerSystem()->GetWorld()->GetCoordinateSystem(GetOwnerSystem()->GetTransform().m_vPosition, coord);

        vNormal = coord.m_vUpDir;
      }

      if (m_MaxDeviation > ezAngle::MakeFromDegree(1.0f))
      {
        // how to get from the X axis to the desired normal
        ezQuat qRotToDir = ezQuat::MakeShortestRotation(ezVec3(1, 0, 0), vNormal);

        for (ezUInt32 i = 0; i < uiNumElements; ++i)
        {
          const ezUInt64 uiElementIdx = uiStartIndex + i;
          const ezVec3 vRandomX = ezVec3::MakeRandomDeviationX(rng, m_MaxDeviation);

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
}

void ezParticleTypeQuad::AllocateParticleData(const ezUInt32 numParticles, const bool bNeedsBillboardData, const bool bNeedsTangentData) const
{
  m_BillboardParticleData = nullptr;
  if (bNeedsBillboardData)
  {
    m_BillboardParticleData = EZ_NEW_ARRAY(ezFrameAllocator::GetCurrentAllocator(), ezBillboardQuadParticleShaderData, numParticles);
  }

  m_TangentParticleData = nullptr;
  if (bNeedsTangentData)
  {
    m_TangentParticleData = EZ_NEW_ARRAY(ezFrameAllocator::GetCurrentAllocator(), ezTangentQuadParticleShaderData, (ezUInt32)GetOwnerSystem()->GetNumActiveParticles());
  }
}

//////////////////////////////////////////////////////////////////////////

#include <Foundation/Serialization/AbstractObjectGraph.h>
#include <Foundation/Serialization/GraphPatch.h>

class ezQuadParticleOrientationPatch_1_2 final : public ezGraphPatch
{
public:
  ezQuadParticleOrientationPatch_1_2()
    : ezGraphPatch("ezQuadParticleOrientation", 2)
  {
  }

  virtual void Patch(ezGraphPatchContext& ref_context, ezAbstractObjectGraph* pGraph, ezAbstractObjectNode* pNode) const override
  {
    // TODO: this type of patch does not work

    pNode->RenameProperty("FragmentOrthogonalEmitterDirection", "Rotating_OrthoEmitterDir");
    pNode->RenameProperty("FragmentEmitterDirection", "Rotating_EmitterDir");

    pNode->RenameProperty("SpriteEmitterDirection", "Fixed_EmitterDir");
    pNode->RenameProperty("SpriteRandom", "Fixed_RandomDir");
    pNode->RenameProperty("SpriteWorldUp", "Fixed_WorldUp");

    pNode->RenameProperty("AxisAligned_Emitter", "FixedAxis_EmitterDir");
  }
};

ezQuadParticleOrientationPatch_1_2 g_ezQuadParticleOrientationPatch_1_2;

//////////////////////////////////////////////////////////////////////////

class ezParticleTypeQuadFactory_1_2 final : public ezGraphPatch
{
public:
  ezParticleTypeQuadFactory_1_2()
    : ezGraphPatch("ezParticleTypeQuadFactory", 2)
  {
  }

  virtual void Patch(ezGraphPatchContext& ref_context, ezAbstractObjectGraph* pGraph, ezAbstractObjectNode* pNode) const override
  {
    ezAbstractObjectNode::Property* pProp = pNode->FindProperty("Orientation");
    const ezStringBuilder sOri = pProp->m_Value.Get<ezString>();

    if (sOri == "ezQuadParticleOrientation::FragmentOrthogonalEmitterDirection")
      pProp->m_Value = "ezQuadParticleOrientation::Rotating_OrthoEmitterDir";

    if (sOri == "ezQuadParticleOrientation::FragmentEmitterDirection")
      pProp->m_Value = "ezQuadParticleOrientation::Rotating_EmitterDir";

    if (sOri == "ezQuadParticleOrientation::SpriteEmitterDirection")
      pProp->m_Value = "ezQuadParticleOrientation::Fixed_EmitterDir";

    if (sOri == "ezQuadParticleOrientation::SpriteRandom")
      pProp->m_Value = "ezQuadParticleOrientation::Fixed_RandomDir";

    if (sOri == "ezQuadParticleOrientation::SpriteWorldUp")
      pProp->m_Value = "ezQuadParticleOrientation::Fixed_WorldUp";

    if (sOri == "ezQuadParticleOrientation::AxisAligned_Emitter")
      pProp->m_Value = "ezQuadParticleOrientation::FixedAxis_EmitterDir";
  }
};

ezParticleTypeQuadFactory_1_2 g_ezParticleTypeQuadFactory_1_2;


EZ_STATICLINK_FILE(ParticlePlugin, ParticlePlugin_Type_Quad_ParticleTypeQuad);
