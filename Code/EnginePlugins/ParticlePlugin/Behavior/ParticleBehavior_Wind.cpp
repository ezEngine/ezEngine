#include <ParticlePlugin/ParticlePluginPCH.h>

#include <Core/Interfaces/WindWorldModule.h>
#include <Core/World/World.h>
#include <Foundation/Configuration/CVar.h>
#include <Foundation/DataProcessing/Stream/ProcessingStreamIterator.h>
#include <Foundation/Profiling/Profiling.h>
#include <Foundation/SimdMath/SimdBBox.h>
#include <Foundation/SimdMath/SimdConversion.h>
#include <ParticlePlugin/Behavior/ParticleBehavior_Wind.h>
#include <ParticlePlugin/WorldModule/ParticleWorldModule.h>
#include <RendererCore/Debug/DebugRenderer.h>

// clang-format off
EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezParticleBehaviorFactory_Wind, 1, ezRTTIDefaultAllocator<ezParticleBehaviorFactory_Wind>)
{
  EZ_BEGIN_PROPERTIES
  {
    EZ_MEMBER_PROPERTY("WindInfluence", m_fWindInfluence)->AddAttributes(new ezClampValueAttribute(0.0f, 10.0f), new ezDefaultValueAttribute(1.0f)),
  }
  EZ_END_PROPERTIES;
}
EZ_END_DYNAMIC_REFLECTED_TYPE;

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezParticleBehavior_Wind, 1, ezRTTIDefaultAllocator<ezParticleBehavior_Wind>)
EZ_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

#if EZ_ENABLED(EZ_COMPILE_FOR_DEVELOPMENT)
ezCVarBool cvar_ParticlesDebugWindSamples("Particles.DebugWindSamples", false, ezCVarFlags::Default, "Enables debug visualization for wind sampling on particle effects.");
#endif

ezParticleBehaviorFactory_Wind::ezParticleBehaviorFactory_Wind() = default;
ezParticleBehaviorFactory_Wind::~ezParticleBehaviorFactory_Wind() = default;

const ezRTTI* ezParticleBehaviorFactory_Wind::GetBehaviorType() const
{
  return ezGetStaticRTTI<ezParticleBehavior_Wind>();
}

void ezParticleBehaviorFactory_Wind::CopyBehaviorProperties(ezParticleBehavior* pObject, bool bFirstTime) const
{
  ezParticleBehavior_Wind* pBehavior = static_cast<ezParticleBehavior_Wind*>(pObject);

  pBehavior->m_fWindInfluence = m_fWindInfluence;
}

enum class BehaviorWindVersion
{
  Version_0 = 0,

  // insert new version numbers above
  Version_Count,
  Version_Current = Version_Count - 1
};

void ezParticleBehaviorFactory_Wind::Save(ezStreamWriter& inout_stream) const
{
  const ezUInt8 uiVersion = (int)BehaviorWindVersion::Version_Current;
  inout_stream << uiVersion;

  inout_stream << m_fWindInfluence;
}

void ezParticleBehaviorFactory_Wind::Load(ezStreamReader& inout_stream, const ezParticleEffectDescriptor& ownerEffectDescriptor, const ezParticleSystemDescriptor& ownerSystemDescriptor)
{
  ezUInt8 uiVersion = 0;
  inout_stream >> uiVersion;

  EZ_ASSERT_DEV(uiVersion <= (int)BehaviorWindVersion::Version_Current, "Invalid version {0}", uiVersion);

  inout_stream >> m_fWindInfluence;
}

ezParticleBehavior_Wind::ezParticleBehavior_Wind() = default;
ezParticleBehavior_Wind::~ezParticleBehavior_Wind() = default;

void ezParticleBehavior_Wind::CreateRequiredStreams()
{
  CreateStream("Position", ezProcessingStream::DataType::Float4, &m_pStreamPosition, false);
}

void ezParticleBehavior_Wind::OnFinalize()
{
  if (m_fWindInfluence <= 0)
  {
    m_pWindSampleGrid = nullptr;
    return;
  }

  RequestUpdateFromWorld();

  const ezVec3U32 numSamples = GetOwnerEffect()->GetNumWindSamples();
  const ezVec4I32 vNumSamples(numSamples.x, numSamples.y, numSamples.z, numSamples.x * numSamples.y * numSamples.z);

  // keep the sampled wind when the effect is only reconfigured, to not lose the smoothing
  if (m_pWindSampleGrid != nullptr && m_vNumWindSamples == vNumSamples)
    return;

  m_vNumWindSamples = vNumSamples;

  m_pWindSampleGrid = EZ_NEW(ezFoundation::GetAlignedAllocator(), WindSampleGrid);
  m_pWindSampleGrid->m_vMinPos.Set(1000.0f);
  m_pWindSampleGrid->m_vMaxPos.Set(-1000.0f);
  m_pWindSampleGrid->m_vInvCellSize.SetZero();
  m_pWindSampleGrid->m_Samples.SetCountUninitialized(m_vNumWindSamples.w);
  ezMemoryUtils::ZeroFill(m_pWindSampleGrid->m_Samples.GetData(), m_vNumWindSamples.w);
}

void ezParticleBehavior_Wind::UpdateFromWorld(const ezWorld& world, ezGameObjectHandle hOwnerObject)
{
  const ezBoundingBoxSphere& bounds = GetOwnerEffect()->GetSimulationSpaceBoundingVolume();

  if (m_pWindSampleGrid == nullptr || !bounds.IsValid())
    return;

  auto& grid = *m_pWindSampleGrid;

  const ezUInt32 uiNumSamplesX = m_vNumWindSamples.x;
  const ezUInt32 uiNumSamplesY = m_vNumWindSamples.y;
  const ezUInt32 uiNumSamplesZ = m_vNumWindSamples.z;
  const ezUInt32 uiTotalNumSamples = m_vNumWindSamples.w;
  EZ_ASSERT_DEBUG(grid.m_Samples.GetCount() == uiTotalNumSamples, "Invalid number of samples");

  const ezSimdVec4f interpolationFactor = ezSimdVec4f(1.0f - ezMath::Pow(0.1f, world.GetClock().GetTimeDiff().AsFloatInSeconds()));

  const ezSimdBBox boundingBox = ezSimdConversion::ToBBox(bounds.GetBox());
  const ezSimdVec4f boundsSize = boundingBox.GetExtents();
  const ezSimdVec4f gridSize = ezSimdVec4i(uiNumSamplesX, uiNumSamplesY, uiNumSamplesZ).ToFloat();
  ezSimdVec4f cellSize = boundsSize.CompDiv(gridSize);
  const ezSimdVec4f minPos = boundingBox.m_Min + cellSize * 0.5f;
  const ezSimdVec4f maxPos = boundingBox.m_Max - cellSize * 0.5f;

  // the previous values are blended towards the new ones
  const ezSimdVec4b oldGridValid = grid.m_vMinPos < grid.m_vMaxPos;
  grid.m_vMinPos = ezSimdVec4f::Select(oldGridValid, ezSimdVec4f::Lerp(grid.m_vMinPos, minPos, interpolationFactor), minPos);
  grid.m_vMaxPos = ezSimdVec4f::Select(oldGridValid, ezSimdVec4f::Lerp(grid.m_vMaxPos, maxPos, interpolationFactor), maxPos);

  const ezSimdVec4f finalGridSize = grid.m_vMaxPos - grid.m_vMinPos;
  const ezSimdVec4f maxIndices = gridSize - ezSimdVec4f(1.0f);
  cellSize = ezSimdVec4f::Select(maxIndices != ezSimdVec4f::MakeZero(), finalGridSize.CompDiv(maxIndices), ezSimdVec4f::MakeZero());
  grid.m_vInvCellSize = ezSimdVec4f::Select(cellSize != ezSimdVec4f::MakeZero(), ezSimdVec4f(1.0f).CompDiv(cellSize), ezSimdVec4f::MakeZero());
  EZ_ASSERT_DEBUG(grid.m_vInvCellSize.IsValid<3>(), "");

  if (auto pWind = world.GetModuleReadOnly<ezWindWorldModuleInterface>())
  {
    for (ezUInt32 i = 0; i < uiTotalNumSamples; ++i)
    {
      ezUInt32 index = i;
      const ezUInt32 z = i / (uiNumSamplesX * uiNumSamplesY);
      index -= z * (uiNumSamplesX * uiNumSamplesY);
      const ezUInt32 y = index / uiNumSamplesX;
      const ezUInt32 x = index - (y * uiNumSamplesX);

      const ezSimdVec4f samplePos = grid.m_vMinPos + cellSize.CompMul(ezSimdVec4i(x, y, z).ToFloat());

      grid.m_Samples[i] = ezSimdVec4f::Lerp(grid.m_Samples[i], pWind->GetWindAtSimd(samplePos), interpolationFactor);

#if EZ_ENABLED(EZ_COMPILE_FOR_DEVELOPMENT)
      if (cvar_ParticlesDebugWindSamples)
      {
        const ezColor c = ezColorScheme::GetColor(ezColorScheme::Blue, 8);

        const ezVec3 samplePos0 = ezSimdConversion::ToVec3(samplePos);
        ezDebugRenderer::DrawCross(&world, samplePos0, 0.1f, c);

        const ezVec3 vWind = ezSimdConversion::ToVec3(grid.m_Samples[i]);
        const float fWindStrength = vWind.GetLength();
        ezDebugRenderer::Draw3DText(&world, ezFmt("{} m/s", ezArgF(fWindStrength, 2)), samplePos0, c);

        if (fWindStrength > 0.01f)
        {
          const ezQuat q = ezQuat::MakeShortestRotation(ezVec3::MakeAxisX(), vWind);
          ezDebugRenderer::DrawArrow(&world, fWindStrength, c, ezTransform::Make(samplePos0, q));
        }
      }
#endif
    }
  }
  else
  {
    for (auto& sample : grid.m_Samples)
    {
      sample.SetZero();
    }
  }
}

void ezParticleBehavior_Wind::Process(ezUInt64 uiNumElements)
{
  EZ_PROFILE_SCOPE("PFX: Wind");

  if (m_pWindSampleGrid == nullptr)
    return;

  const float tDiff = (float)m_TimeDiff.GetSeconds();
  const ezSimdFloat fWindFactor = m_fWindInfluence * tDiff;

  ezProcessingStreamIterator<ezSimdVec4f> itPosition(m_pStreamPosition, uiNumElements, 0);

  while (!itPosition.HasReachedEnd())
  {
    ezSimdVec4f windOffset = GetWindAt(itPosition.Current()) * fWindFactor;
    itPosition.Current() += windOffset;

    itPosition.Advance();
  }
}

ezSimdVec4f ezParticleBehavior_Wind::GetWindAt(const ezSimdVec4f& vPosition) const
{
  const auto& grid = *m_pWindSampleGrid;

  const ezUInt32 uiTotalNumSamples = m_vNumWindSamples.w;
  EZ_ASSERT_DEBUG(grid.m_Samples.GetCount() == uiTotalNumSamples, "Invalid sample count");

  // Sample grid with trilinear interpolation
  ezSimdVec4f gridSpacePos = (vPosition - grid.m_vMinPos).CompMul(grid.m_vInvCellSize);
  gridSpacePos = gridSpacePos.CompMax(ezSimdVec4f::MakeZero());

  const ezSimdVec4f gridSpacePosFloor = gridSpacePos.Floor();
  const ezSimdVec4f weights = gridSpacePos - gridSpacePosFloor;

  const ezSimdVec4i maxIndices = ezSimdConversion::ToVec4i(m_vNumWindSamples) - ezSimdVec4i(1);
  const ezSimdVec4i pos0 = ezSimdVec4i::Truncate(gridSpacePosFloor).CompMin(maxIndices);
  const ezSimdVec4i pos1 = (pos0 + ezSimdVec4i(1)).CompMin(maxIndices);

  const ezInt32 xCount = m_vNumWindSamples.x;
  const ezInt32 xyCount = xCount * m_vNumWindSamples.y;
  const ezSimdVec4i cXcXYcXcXY = ezSimdVec4i(xCount, xyCount, xCount, xyCount);
  const ezSimdVec4i y0z0y1z1 = pos0.GetCombined<ezSwizzle::YZYZ>(pos1).CompMul(cXcXYcXcXY);
  const ezSimdVec4i y0y0y1y1 = y0z0y1z1.Get<ezSwizzle::XXZZ>();
  const ezSimdVec4i x0x0x1x1 = pos0.GetCombined<ezSwizzle::XXXX>(pos1);
  const ezSimdVec4i x0x1x0x1 = x0x0x1x1.Get<ezSwizzle::XZXZ>();
  const ezSimdVec4i y0y0y1y1_plus_x0x1x0x1 = y0y0y1y1 + x0x1x0x1;

  const ezSimdVec4f wX = weights.Get<ezSwizzle::XXXX>();
  const ezSimdVec4f wY = weights.Get<ezSwizzle::YYYY>();

  const ezSimdVec4f* pSamples = grid.m_Samples.GetData();

  const ezSimdVec4i indices_z0 = y0z0y1z1.Get<ezSwizzle::YYYY>() + y0y0y1y1_plus_x0x1x0x1;
  const ezSimdVec4f sample_z0y0x0 = pSamples[indices_z0.x()];
  const ezSimdVec4f sample_z0y0x1 = pSamples[indices_z0.y()];
  const ezSimdVec4f res_z0y0 = ezSimdVec4f::Lerp(sample_z0y0x0, sample_z0y0x1, wX);

  const ezSimdVec4f sample_z0y1x0 = pSamples[indices_z0.z()];
  const ezSimdVec4f sample_z0y1x1 = pSamples[indices_z0.w()];
  const ezSimdVec4f res_z0y1 = ezSimdVec4f::Lerp(sample_z0y1x0, sample_z0y1x1, wX);

  const ezSimdVec4f res_z0 = ezSimdVec4f::Lerp(res_z0y0, res_z0y1, wY);

  const ezSimdVec4i indices_z1 = y0z0y1z1.Get<ezSwizzle::WWWW>() + y0y0y1y1_plus_x0x1x0x1;
  const ezSimdVec4f sample_z1y0x0 = pSamples[indices_z1.x()];
  const ezSimdVec4f sample_z1y0x1 = pSamples[indices_z1.y()];
  const ezSimdVec4f res_z1y0 = ezSimdVec4f::Lerp(sample_z1y0x0, sample_z1y0x1, wX);

  const ezSimdVec4f sample_z1y1x0 = pSamples[indices_z1.z()];
  const ezSimdVec4f sample_z1y1x1 = pSamples[indices_z1.w()];
  const ezSimdVec4f res_z1y1 = ezSimdVec4f::Lerp(sample_z1y1x0, sample_z1y1x1, wX);

  const ezSimdVec4f res_z1 = ezSimdVec4f::Lerp(res_z1y0, res_z1y1, wY);

  const ezSimdVec4f wZ = weights.Get<ezSwizzle::ZZZZ>();
  const ezSimdVec4f res = ezSimdVec4f::Lerp(res_z0, res_z1, wZ);

  return res;
}

void ezParticleBehavior_Wind::RequestRequiredWorldModulesForCache(ezParticleWorldModule* pParticleModule)
{
  pParticleModule->CacheWorldModule<ezWindWorldModuleInterface>();
}


EZ_STATICLINK_FILE(ParticlePlugin, ParticlePlugin_Behavior_ParticleBehavior_Wind);
