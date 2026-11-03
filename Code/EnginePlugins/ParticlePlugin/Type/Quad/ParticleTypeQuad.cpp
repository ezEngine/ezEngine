#include <ParticlePlugin/ParticlePluginPCH.h>

#include <ParticlePlugin/Type/Quad/ParticleTypeQuad.h>

#include <Core/World/World.h>
#include <Foundation/Math/Color16f.h>
#include <Foundation/Math/Float16.h>
#include <Foundation/Profiling/Profiling.h>
#include <ParticlePlugin/Effect/ParticleEffectInstance.h>
#include <ParticlePlugin/Finalizer/ParticleFinalizer_LastPosition.h>
#include <RendererCore/Material/MaterialResource.h>
#include <RendererCore/Pipeline/RenderDataManager.h>
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

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezParticleTypeQuad, 2, ezRTTIDefaultAllocator<ezParticleTypeQuad>)
{
  EZ_BEGIN_PROPERTIES
  {
    EZ_ENUM_MEMBER_PROPERTY("Orientation", ezQuadParticleOrientation, m_Orientation),
    EZ_MEMBER_PROPERTY("Deviation", m_MaxDeviation)->AddAttributes(new ezClampValueAttribute(ezAngle::MakeFromDegree(0), ezAngle::MakeFromDegree(90))),
    EZ_ENUM_MEMBER_PROPERTY("RenderMode", ezParticleTypeRenderMode, m_RenderMode),
    EZ_ENUM_MEMBER_PROPERTY("LightingMode", ezParticleLightingMode, m_LightingMode),
    EZ_MEMBER_PROPERTY("NormalCurvature", m_fNormalCurvature)->AddAttributes(new ezDefaultValueAttribute(0.5f), new ezClampValueAttribute(0, 1)),
    EZ_MEMBER_PROPERTY("LightDirectionality", m_fLightDirectionality)->AddAttributes(new ezDefaultValueAttribute(0.5f), new ezClampValueAttribute(0, 1)),
    EZ_MEMBER_PROPERTY("UseCustomMaterial", m_bUseCustomMaterial),
    EZ_MEMBER_PROPERTY("CustomMaterial", m_sCustomMaterial)->AddAttributes(new ezAssetBrowserAttribute("CompatibleAsset_Material", "QuadParticle")),
    EZ_MEMBER_PROPERTY("Texture", m_sTexture)->AddAttributes(new ezAssetBrowserAttribute("CompatibleAsset_Texture_2D"), new ezDefaultValueAttribute(ezStringView("{ e00262e8-58f5-42f5-880d-569257047201 }"))),// wrap in ezStringView to prevent a memory leak report
    EZ_ENUM_MEMBER_PROPERTY("TextureAtlas", ezParticleTextureAtlasType, m_TextureAtlasType),
    EZ_ENUM_MEMBER_PROPERTY("TextureOrientation", ezParticleTextureAtlasOrientation, m_TextureAtlasOrientation),
    EZ_MEMBER_PROPERTY("NumSpritesX", m_uiNumSpritesX)->AddAttributes(new ezDefaultValueAttribute(1), new ezClampValueAttribute(1, 16)),
    EZ_MEMBER_PROPERTY("NumSpritesY", m_uiNumSpritesY)->AddAttributes(new ezDefaultValueAttribute(1), new ezClampValueAttribute(1, 16)),
    EZ_MEMBER_PROPERTY("TintColorParam", m_sTintColorParameter),
    EZ_MEMBER_PROPERTY("ParticleStretch", m_fStretch)->AddAttributes(new ezDefaultValueAttribute(1.0f), new ezClampValueAttribute(-100.0f, 100.0f)),
    EZ_MEMBER_PROPERTY("GeometryProximityFadeOut", m_fGeometryProximityFadeOut)->AddAttributes(new ezDefaultValueAttribute(0.1f), new ezClampValueAttribute(0, 100)),
    EZ_MEMBER_PROPERTY("CameraProximityFadeOut", m_fCameraProximityFadeOut)->AddAttributes(new ezDefaultValueAttribute(0.5f), new ezClampValueAttribute(0, 100)),
  }
  EZ_END_PROPERTIES;
}
EZ_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

enum class TypeQuadVersion
{
  Version_0 = 0,
  Version_1,
  Version_2, // sprite deviation
  Version_3, // distortion
  Version_4, // added texture atlas type
  Version_5, // added particle stretch
  Version_6, // added particle lighting
  Version_7, // added custom material support
  Version_8, // added proximity fade out parameters
  Version_9, // added texture atlas orientation

  // insert new version numbers above
  Version_Count,
  Version_Current = Version_Count - 1
};

void ezParticleTypeQuad::Save(ezStreamWriter& inout_stream) const
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

  ezString sDistortionTexture;
  float fDistortionStrength = 0;
  inout_stream << sDistortionTexture;
  inout_stream << fDistortionStrength;
  inout_stream << m_TextureAtlasType;

  // Version 5
  inout_stream << m_fStretch;

  // Version 6
  inout_stream << m_LightingMode;
  inout_stream << m_fNormalCurvature;
  inout_stream << m_fLightDirectionality;

  // Version 7
  inout_stream << m_bUseCustomMaterial;
  inout_stream << m_sCustomMaterial;

  // Version 8
  inout_stream << m_fGeometryProximityFadeOut;
  inout_stream << m_fCameraProximityFadeOut;

  // Version 9
  inout_stream << m_TextureAtlasOrientation;
}

void ezParticleTypeQuad::Load(ezStreamReader& inout_stream, const ezParticleEffectDescriptor& ownerEffectDescriptor, const ezParticleSystemDescriptor& ownerSystemDescriptor)
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
    ezString sDistortionTexture;
    float fDistortionStrength;
    inout_stream >> sDistortionTexture;
    inout_stream >> fDistortionStrength;
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

  if (uiVersion >= 7)
  {
    inout_stream >> m_bUseCustomMaterial;
    inout_stream >> m_sCustomMaterial;
  }

  if (uiVersion >= 8)
  {
    inout_stream >> m_fGeometryProximityFadeOut;
    inout_stream >> m_fCameraProximityFadeOut;
  }

  if (uiVersion >= 9)
  {
    inout_stream >> m_TextureAtlasOrientation;
  }

  if (m_bUseCustomMaterial)
  {
    if (!m_sCustomMaterial.IsEmpty())
    {
      m_hCustomMaterial = ezResourceManager::LoadResource<ezMaterialResource>(m_sCustomMaterial);
    }
  }
  else if (!m_sTexture.IsEmpty())
  {
    m_hTexture = ezResourceManager::LoadResource<ezTexture2DResource>(m_sTexture);
  }
}

void ezParticleTypeQuad::QueryFinalizerDependencies(ezSet<const ezRTTI*>& inout_finalizerDeps) const
{
  if (m_Orientation == ezQuadParticleOrientation::FixedAxis_ParticleDir)
  {
    inout_finalizerDeps.Insert(ezGetStaticRTTI<ezParticleFinalizer_LastPosition>());
  }
}

bool ezParticleTypeQuad::GetInstanceDataDesc(ezInstanceDataDesc& out_desc) const
{
  out_desc.FillFromType<InstanceData>();
  return true;
}

void ezParticleTypeQuad::CreateRequiredStreams(const ezParticleModuleContext& ctx) const
{
  InstanceData& data = ctx.GetInstanceData<InstanceData>();

  ctx.CreateStream(ezParticleStreamNames::s_sLifeTime, ezProcessingStream::DataType::Half2, &data.m_pStreamLifeTime, false);
  ctx.CreateStream(ezParticleStreamNames::s_sPosition, ezProcessingStream::DataType::Float4, &data.m_pStreamPosition, false);
  ctx.CreateStream(ezParticleStreamNames::s_sSize, ezProcessingStream::DataType::Half, &data.m_pStreamSize, false);
  ctx.CreateStream(ezParticleStreamNames::s_sColor, ezProcessingStream::DataType::Half4, &data.m_pStreamColor, false);
  ctx.CreateStream(ezParticleStreamNames::s_sRotationSpeed, ezProcessingStream::DataType::Half, &data.m_pStreamRotationSpeed, false);
  ctx.CreateStream(ezParticleStreamNames::s_sRotationOffset, ezProcessingStream::DataType::Half, &data.m_pStreamRotationOffset, false);

  data.m_pStreamAxis = nullptr;
  data.m_pStreamVariation = nullptr;
  data.m_pStreamLastPosition = nullptr;

  if (m_Orientation == ezQuadParticleOrientation::Fixed_RandomDir || m_Orientation == ezQuadParticleOrientation::Fixed_EmitterDir || m_Orientation == ezQuadParticleOrientation::Fixed_WorldUp)
  {
    ctx.CreateStream(ezParticleStreamNames::s_sAxis, ezProcessingStream::DataType::Float3, &data.m_pStreamAxis, true);
  }

  if (m_TextureAtlasType == ezParticleTextureAtlasType::RandomVariations || m_TextureAtlasType == ezParticleTextureAtlasType::RandomYAnimatedX || m_TextureAtlasType == ezParticleTextureAtlasType::RandomXAnimatedY)
  {
    ctx.CreateStream(ezParticleStreamNames::s_sVariation, ezProcessingStream::DataType::Int, &data.m_pStreamVariation, false);
  }

  if (m_Orientation == ezQuadParticleOrientation::FixedAxis_ParticleDir)
  {
    ctx.CreateStream(ezParticleStreamNames::s_sLastPosition, ezProcessingStream::DataType::Float3, &data.m_pStreamLastPosition, false);
  }
}

struct sodComparer
{
  // sort farther particles to the front, so that they get rendered first (back to front)
  EZ_ALWAYS_INLINE bool Less(const ezParticleTypeQuad::sod& a, const ezParticleTypeQuad::sod& b) const { return a.dist > b.dist; }
  EZ_ALWAYS_INLINE bool Equal(const ezParticleTypeQuad::sod& a, const ezParticleTypeQuad::sod& b) const { return a.dist == b.dist; }
};

void ezParticleTypeQuad::ExtractTypeRenderData(const ezParticleModuleContext& ctx, ezMsgExtractRenderData& ref_msg, const ezTransform& instanceTransform) const
{
  InstanceData& data = ctx.GetInstanceData<InstanceData>();

  if ((!m_hTexture.IsValid() && !m_hCustomMaterial))
    return;

  const ezUInt32 numParticles = (ezUInt32)ctx.m_pSystem->GetNumActiveParticles();
  if (numParticles == 0)
    return;

  const bool bNeedsSorting = (m_RenderMode == ezParticleTypeRenderMode::Blended) || (m_RenderMode == ezParticleTypeRenderMode::BlendedForeground) || (m_RenderMode == ezParticleTypeRenderMode::BlendedBackground);

  // don't copy the data multiple times in the same frame, if the effect is instanced
  if ((data.m_uiLastExtractedFrame != ezRenderWorld::GetFrameCounter())
    /*&& !bNeedsSorting*/) // TODO: in theory every shared instance has to sort the Quads, in practice this maybe should be an option
  {
    data.m_uiLastExtractedFrame = ezRenderWorld::GetFrameCounter();

    if (bNeedsSorting)
    {
      // TODO: Using the frame allocator this way results in memory corruptions.
      // Not sure, whether this is supposed to work.
      ezTempHybridArray<sod, 64> sorted; // (ezFrameAllocator::GetCurrentAllocator());
      sorted.SetCountUninitialized(numParticles);

      const ezVec3 vCameraPos = ref_msg.m_pView->GetCullingCamera()->GetCenterPosition();
      const ezVec4* pPosition = data.m_pStreamPosition->GetData<ezVec4>();

      for (ezUInt32 p = 0; p < numParticles; ++p)
      {
        sorted[p].dist = (pPosition[p].GetAsVec3() - vCameraPos).GetLengthSquared();
        sorted[p].index = p;
      }

      sorted.Sort(sodComparer());

      CreateExtractedData(ctx, &sorted);
    }
    else
    {
      CreateExtractedData(ctx, nullptr);
    }
  }

  AddParticleRenderData(ctx, ref_msg, instanceTransform);
}

EZ_ALWAYS_INLINE ezUInt32 noRedirect(ezUInt32 uiIdx, const ezHybridArray<ezParticleTypeQuad::sod, 64>* pSorted)
{
  return uiIdx;
}

EZ_ALWAYS_INLINE ezUInt32 sortedRedirect(ezUInt32 uiIdx, const ezHybridArray<ezParticleTypeQuad::sod, 64>* pSorted)
{
  return (*pSorted)[uiIdx].index;
}

void ezParticleTypeQuad::CreateExtractedData(const ezParticleModuleContext& ctx, const ezHybridArray<sod, 64>* pSorted) const
{
  InstanceData& data = ctx.GetInstanceData<InstanceData>();

  auto redirect = (pSorted != nullptr) ? sortedRedirect : noRedirect;

  const ezUInt32 numParticles = (ezUInt32)ctx.m_pSystem->GetNumActiveParticles();

  const bool bNeedsBillboardData = m_Orientation == ezQuadParticleOrientation::Billboard;
  const bool bNeedsTangentData = !bNeedsBillboardData;

  const ezVec3 vEmitterPos = ctx.m_pSystem->GetTransform().m_vPosition;
  const ezVec3 vEmitterDir = ctx.m_pSystem->GetTransform().m_qRotation * ezVec3(0, 0, 1); // Z axis
  const ezVec3 vEmitterDirOrtho = vEmitterDir.GetOrthogonalVector();

  const ezTime tCur = ctx.GetEffect()->GetTotalEffectLifeTime();
  const ezColor tintColor = ctx.GetEffect()->GetColorParameter(m_sTintColorParameter, ezColor::White);

  const ezFloat16Vec2* pLifeTime = data.m_pStreamLifeTime->GetData<ezFloat16Vec2>();
  const ezVec4* pPosition = data.m_pStreamPosition->GetData<ezVec4>();
  const ezFloat16* pSize = data.m_pStreamSize->GetData<ezFloat16>();
  const ezColorLinear16f* pColor = data.m_pStreamColor->GetData<ezColorLinear16f>();
  const ezFloat16* pRotationSpeed = data.m_pStreamRotationSpeed->GetData<ezFloat16>();
  const ezFloat16* pRotationOffset = data.m_pStreamRotationOffset->GetData<ezFloat16>();
  const ezVec3* pAxis = data.m_pStreamAxis ? data.m_pStreamAxis->GetData<ezVec3>() : nullptr;
  const ezUInt32* pVariation = data.m_pStreamVariation ? data.m_pStreamVariation->GetData<ezUInt32>() : nullptr;
  const ezVec3* pLastPosition = data.m_pStreamLastPosition ? data.m_pStreamLastPosition->GetData<ezVec3>() : nullptr;

  // this will automatically be deallocated at the end of the frame
  data.m_BaseParticleData = EZ_NEW_ARRAY(ezFrameAllocator::GetCurrentAllocator(), ezBaseParticleShaderData, numParticles);

  AllocateParticleData(data, numParticles, bNeedsBillboardData, bNeedsTangentData);

  auto SetBaseData = [&](ezUInt32 uiDstIdx, ezUInt32 uiSrcIdx)
  {
    data.m_BaseParticleData[uiDstIdx].Size = pSize[uiSrcIdx];
    data.m_BaseParticleData[uiDstIdx].Color = pColor[uiSrcIdx].ToLinearFloat() * tintColor;
    data.m_BaseParticleData[uiDstIdx].Life = pLifeTime[uiSrcIdx].x * pLifeTime[uiSrcIdx].y;
    data.m_BaseParticleData[uiDstIdx].Variation = (pVariation != nullptr) ? pVariation[uiSrcIdx] : 0;
  };

  auto SetBillboardData = [&](ezUInt32 uiDstIdx, ezUInt32 uiSrcIdx)
  {
    data.m_BillboardParticleData[uiDstIdx].Position = pPosition[uiSrcIdx].GetAsVec3();
    data.m_BillboardParticleData[uiDstIdx].RotationOffset = pRotationOffset[uiSrcIdx];
    data.m_BillboardParticleData[uiDstIdx].RotationSpeed = pRotationSpeed[uiSrcIdx];
  };

  auto SetTangentDataEmitterDir = [&](ezUInt32 uiDstIdx, ezUInt32 uiSrcIdx)
  {
    ezMat3 mRotation = ezMat3::MakeAxisRotation(vEmitterDir, ezAngle::MakeFromRadian((float)(tCur.GetSeconds() * pRotationSpeed[uiSrcIdx]) + pRotationOffset[uiSrcIdx]));

    data.m_TangentParticleData[uiDstIdx].Position = pPosition[uiSrcIdx].GetAsVec3();
    data.m_TangentParticleData[uiDstIdx].TangentX = mRotation * vEmitterDirOrtho;
    data.m_TangentParticleData[uiDstIdx].TangentZ = vEmitterDir;
  };

  auto SetTangentDataEmitterDirOrtho = [&](ezUInt32 uiDstIdx, ezUInt32 uiSrcIdx)
  {
    const ezVec3 vDirToParticle = (pPosition[uiSrcIdx].GetAsVec3() - vEmitterPos);
    ezVec3 vOrthoDir = vEmitterDir.CrossRH(vDirToParticle);
    vOrthoDir.NormalizeIfNotZero(ezVec3(1, 0, 0)).IgnoreResult();

    ezMat3 mRotation = ezMat3::MakeAxisRotation(vOrthoDir, ezAngle::MakeFromRadian((float)(tCur.GetSeconds() * pRotationSpeed[uiSrcIdx]) + pRotationOffset[uiSrcIdx]));

    data.m_TangentParticleData[uiDstIdx].Position = pPosition[uiSrcIdx].GetAsVec3();
    data.m_TangentParticleData[uiDstIdx].TangentX = vOrthoDir;
    data.m_TangentParticleData[uiDstIdx].TangentZ = mRotation * vEmitterDir;
  };

  auto SetTangentDataFromAxis = [&](ezUInt32 uiDstIdx, ezUInt32 uiSrcIdx)
  {
    EZ_ASSERT_DEBUG(pAxis != nullptr, "Axis must be valid");
    ezVec3 vNormal = pAxis[uiSrcIdx];
    vNormal.Normalize();

    const ezVec3 vTangentStart = vNormal.GetOrthogonalVector().GetNormalized();

    ezMat3 mRotation = ezMat3::MakeAxisRotation(vNormal, ezAngle::MakeFromRadian((float)(tCur.GetSeconds() * pRotationSpeed[uiSrcIdx]) + pRotationOffset[uiSrcIdx]));

    const ezVec3 vTangentX = mRotation * vTangentStart;

    data.m_TangentParticleData[uiDstIdx].Position = pPosition[uiSrcIdx].GetAsVec3();
    data.m_TangentParticleData[uiDstIdx].TangentX = vTangentX;
    data.m_TangentParticleData[uiDstIdx].TangentZ = vTangentX.CrossRH(vNormal);
  };

  auto SetTangentDataAligned_Emitter = [&](ezUInt32 uiDstIdx, ezUInt32 uiSrcIdx)
  {
    data.m_TangentParticleData[uiDstIdx].Position = pPosition[uiSrcIdx].GetAsVec3();
    data.m_TangentParticleData[uiDstIdx].TangentX = vEmitterDir;
    data.m_TangentParticleData[uiDstIdx].TangentZ.x = m_fStretch;
  };

  auto SetTangentDataAligned_ParticleDir = [&](ezUInt32 uiDstIdx, ezUInt32 uiSrcIdx)
  {
    const ezVec3 vCurPos = pPosition[uiSrcIdx].GetAsVec3();
    const ezVec3 vLastPos = pLastPosition[uiSrcIdx];
    const ezVec3 vDir = vCurPos - vLastPos;
    data.m_TangentParticleData[uiDstIdx].Position = vCurPos;
    data.m_TangentParticleData[uiDstIdx].TangentX = vDir;
    data.m_TangentParticleData[uiDstIdx].TangentZ.x = m_fStretch;
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

void ezParticleTypeQuad::AddParticleRenderData(const ezParticleModuleContext& ctx, ezMsgExtractRenderData& msg, const ezTransform& instanceTransform) const
{
  InstanceData& data = ctx.GetInstanceData<InstanceData>();

  auto pRenderData = msg.m_pRenderDataManager->CreateRenderDataForThisFrame<ezParticleQuadRenderData>(nullptr);

  if (m_hCustomMaterial.IsValid())
  {
    pRenderData->m_uiSortingKey = ComputeSortingKey(m_RenderMode, m_hCustomMaterial.GetResourceIDHash(), 0);
  }
  else
  {
    pRenderData->m_uiSortingKey = ComputeSortingKey(m_RenderMode, m_hTexture.GetResourceIDHash(), 0);
  }

  pRenderData->m_vGlobalPosition = instanceTransform.m_vPosition;

  pRenderData->m_GlobalTransform = ctx.GetEffect()->NeedsToApplyTransform() ? instanceTransform : ezTransform::MakeIdentity();
  pRenderData->m_TotalEffectLifeTime = ctx.GetEffect()->GetTotalEffectLifeTime();
  pRenderData->m_RenderMode = m_RenderMode;
  pRenderData->m_hTexture = m_hTexture;
  pRenderData->m_BaseParticleData = data.m_BaseParticleData;
  pRenderData->m_BillboardParticleData = data.m_BillboardParticleData;
  pRenderData->m_TangentParticleData = data.m_TangentParticleData;
  pRenderData->m_uiNumVariationsX = 1;
  pRenderData->m_uiNumVariationsY = 1;
  pRenderData->m_uiNumFlipbookAnimationsX = 1;
  pRenderData->m_uiNumFlipbookAnimationsY = 1;
  pRenderData->m_LightingMode = m_LightingMode;
  pRenderData->m_TextureAtlasOrientation = m_TextureAtlasOrientation;
  pRenderData->m_fNormalCurvature = m_fNormalCurvature;
  pRenderData->m_fLightDirectionality = m_fLightDirectionality;
  pRenderData->m_fGeometryProximityFadeOut = m_fGeometryProximityFadeOut;
  pRenderData->m_fCameraProximityFadeOut = m_fCameraProximityFadeOut;
  pRenderData->m_hCustomMaterial = m_hCustomMaterial;

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

    case ezParticleTextureAtlasType::RandomXAnimatedY:
      pRenderData->m_uiNumVariationsX = m_uiNumSpritesX;
      pRenderData->m_uiNumFlipbookAnimationsY = m_uiNumSpritesY;
      break;
  }

  msg.AddRenderData(pRenderData, ezDefaultRenderDataCategories::LitTransparent, ezRenderData::Caching::Never);
}

void ezParticleTypeQuad::InitializeElements(const ezParticleModuleContext& ctx, ezUInt64 uiStartIndex, ezUInt64 uiNumElements) const
{
  InstanceData& data = ctx.GetInstanceData<InstanceData>();

  if (data.m_pStreamAxis != nullptr)
  {
    ezVec3* pAxis = data.m_pStreamAxis->GetWritableData<ezVec3>();
    ezRandom& rng = ctx.GetRNG();

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
        vNormal = ctx.m_pSystem->GetTransform().m_qRotation * ezVec3(0, 0, 1); // Z axis
      }
      else if (m_Orientation == ezQuadParticleOrientation::Fixed_WorldUp)
      {
        ezCoordinateSystem coord;
        ctx.m_pSystem->GetWorld()->GetCoordinateSystem(ctx.m_pSystem->GetTransform().m_vPosition, coord);

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

void ezParticleTypeQuad::AllocateParticleData(InstanceData& data, const ezUInt32 numParticles, const bool bNeedsBillboardData, const bool bNeedsTangentData)
{
  data.m_BillboardParticleData = nullptr;
  if (bNeedsBillboardData)
  {
    data.m_BillboardParticleData = EZ_NEW_ARRAY(ezFrameAllocator::GetCurrentAllocator(), ezBillboardQuadParticleShaderData, numParticles);
  }

  data.m_TangentParticleData = nullptr;
  if (bNeedsTangentData)
  {
    data.m_TangentParticleData = EZ_NEW_ARRAY(ezFrameAllocator::GetCurrentAllocator(), ezTangentQuadParticleShaderData, numParticles);
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

class ezParticleTypeQuad_1_2 final : public ezGraphPatch
{
public:
  ezParticleTypeQuad_1_2()
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

ezParticleTypeQuad_1_2 g_ezParticleTypeQuad_1_2;


EZ_STATICLINK_FILE(ParticlePlugin, ParticlePlugin_Type_Quad_ParticleTypeQuad);
