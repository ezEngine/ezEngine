#include <ParticlePlugin/ParticlePluginPCH.h>

#include <Foundation/DataProcessing/Stream/ProcessingStreamIterator.h>
#include <Foundation/Profiling/Profiling.h>
#include <Foundation/SimdMath/SimdConversion.h>
#include <ParticlePlugin/Behavior/ParticleBehavior_Turbulence.h>
#include <ParticlePlugin/Effect/ParticleEffectInstance.h>
#include <ParticlePlugin/Finalizer/ParticleFinalizer_ApplyVelocity.h>
#include <ParticlePlugin/System/ParticleSystemInstance.h>

// clang-format off
EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezParticleBehaviorFactory_Turbulence, 1, ezRTTIDefaultAllocator<ezParticleBehaviorFactory_Turbulence>)
{
  EZ_BEGIN_PROPERTIES
  {
    EZ_MEMBER_PROPERTY("Strength", m_fStrength)->AddAttributes(new ezDefaultValueAttribute(2.0f)),
    EZ_MEMBER_PROPERTY("Frequency", m_fFrequency)->AddAttributes(new ezDefaultValueAttribute(1.0f), new ezClampValueAttribute(0.01f, {})),
    EZ_MEMBER_PROPERTY("ChangeSpeed", m_vScrollSpeed)->AddAttributes(new ezDefaultValueAttribute(ezVec3(1.0f))),
    EZ_MEMBER_PROPERTY("Octaves", m_uiOctaves)->AddAttributes(new ezDefaultValueAttribute(1), new ezClampValueAttribute(1, 4)),
    EZ_MEMBER_PROPERTY("AffectVelocity", m_bAffectVelocity)->AddAttributes(new ezDefaultValueAttribute(true)),
  }
  EZ_END_PROPERTIES;
}
EZ_END_DYNAMIC_REFLECTED_TYPE;

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezParticleBehavior_Turbulence, 1, ezRTTIDefaultAllocator<ezParticleBehavior_Turbulence>)
EZ_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

ezParticleBehaviorFactory_Turbulence::ezParticleBehaviorFactory_Turbulence() = default;

const ezRTTI* ezParticleBehaviorFactory_Turbulence::GetBehaviorType() const
{
  return ezGetStaticRTTI<ezParticleBehavior_Turbulence>();
}

void ezParticleBehaviorFactory_Turbulence::CopyBehaviorProperties(ezParticleBehavior* pObject, bool bFirstTime) const
{
  ezParticleBehavior_Turbulence* pBehavior = static_cast<ezParticleBehavior_Turbulence*>(pObject);

  pBehavior->m_fStrength = m_fStrength;
  pBehavior->m_fFrequency = m_fFrequency;
  pBehavior->m_vScrollSpeed = m_vScrollSpeed;
  pBehavior->m_uiOctaves = m_uiOctaves;
  pBehavior->m_bAffectVelocity = m_bAffectVelocity;
}

void ezParticleBehaviorFactory_Turbulence::QueryFinalizerDependencies(ezSet<const ezRTTI*>& inout_finalizerDeps) const
{
  if (m_bAffectVelocity)
  {
    inout_finalizerDeps.Insert(ezGetStaticRTTI<ezParticleFinalizerFactory_ApplyVelocity>());
  }
}

void ezParticleBehaviorFactory_Turbulence::Save(ezStreamWriter& inout_stream) const
{
  const ezUInt8 uiVersion = 1;
  inout_stream << uiVersion;

  inout_stream << m_fStrength;
  inout_stream << m_fFrequency;
  inout_stream << m_vScrollSpeed;
  inout_stream << m_uiOctaves;
  inout_stream << m_bAffectVelocity;
}

void ezParticleBehaviorFactory_Turbulence::Load(ezStreamReader& inout_stream)
{
  ezUInt8 uiVersion = 0;
  inout_stream >> uiVersion;

  EZ_ASSERT_DEV(uiVersion <= 1, "Invalid version {0}", uiVersion);

  inout_stream >> m_fStrength;
  inout_stream >> m_fFrequency;
  inout_stream >> m_vScrollSpeed;
  inout_stream >> m_uiOctaves;
  inout_stream >> m_bAffectVelocity;
}

void ezParticleBehavior_Turbulence::OnFinalize()
{
  m_Noise.Initialize(GetOwnerEffect()->GetRNG());
}

void ezParticleBehavior_Turbulence::CreateRequiredStreams()
{
  CreateStream("Position", ezProcessingStream::DataType::Float4, &m_pStreamPosition, false);

  if (m_bAffectVelocity)
  {
    CreateStream("Velocity", ezProcessingStream::DataType::Half4, &m_pStreamVelocity, false);
  }
}

void ezParticleBehavior_Turbulence::Process(ezUInt64 uiNumElements)
{
  EZ_PROFILE_SCOPE("PFX: NoiseForce");

  const float tDiff = m_TimeDiff.AsFloatInSeconds();
  if (tDiff <= 0.0f)
    return;

  m_TotalTime += m_TimeDiff;

  const float freq = m_fFrequency;
  const ezVec3 scroll = m_vScrollSpeed * m_TotalTime.AsFloatInSeconds();

  // Small offsets for curl noise approximation: sample noise at 3 different offsets
  // to get uncorrelated X, Y, Z force components
  const float kOffset1 = 31.416f;
  const float kOffset2 = 67.123f;

  ezProcessingStreamIterator<ezSimdVec4f> itPosition(m_pStreamPosition, uiNumElements, 0);

  const ezSimdVec4f constHalf(0.5f);
  const ezSimdVec4f constMul(2.0f * m_fStrength * tDiff);

  // Precompute (scroll * freq) so per-particle noise coords can be computed with a single MulAdd
  const ezSimdVec4f simdScrollFreq(scroll.x * freq, scroll.y * freq, scroll.z * freq, 0.0f);
  const ezSimdFloat simdFreq(freq);

  if (m_bAffectVelocity)
  {
    ezProcessingStreamIterator<ezFloat16Vec4> itVelocity(m_pStreamVelocity, uiNumElements, 0);

    while (!itPosition.HasReachedEnd())
    {
      const ezSimdVec4f simPos = itPosition.Current();

      // Noise sampling coordinates: (pos + scroll) * freq, computed per-axis via SIMD
      const ezSimdVec4f sPos = ezSimdVec4f::MulAdd(simPos, simdFreq, simdScrollFreq);
      const float sx = sPos.GetComponent<0>();
      const float sy = sPos.GetComponent<1>();
      const float sz = sPos.GetComponent<2>();

      // Sample noise at 3 offset positions for curl-noise approximation
      // Each component uses a different spatial offset to get uncorrelated values
      const ezSimdVec4f noise = m_Noise.NoiseZeroToOne(ezSimdVec4f(sx, sx + kOffset1, sx + kOffset2, 0), ezSimdVec4f(sy, sy + kOffset1, sy + kOffset2, 0), ezSimdVec4f(sz, sz + kOffset1, sz + kOffset2, 0), m_uiOctaves);

      // Map [0,1] -> [-1,1]; velocity is stored as (dirX, dirY, dirZ, speed)
      const ezSimdVec4f forceSimd = (noise - constHalf).CompMul(constMul);

      const ezVec4 vel4 = itVelocity.Current();
      ezSimdVec4f velSimd = ezSimdConversion::ToVec4(vel4);

      // Compute dir * speed + force in SIMD (velSimd.w = speed, broadcast via w())
      ezSimdVec4f newVelSimd = ezSimdVec4f::MulAdd(velSimd, velSimd.w(), forceSimd);

      const ezSimdFloat newSpeed = newVelSimd.GetLength<3>();
      newVelSimd.NormalizeIfNotZero<3>(ezSimdVec4f(0.0f, 0.0f, 1.0f, 0.0f));
      newVelSimd.SetW(newSpeed);

      itVelocity.Current() = ezSimdConversion::ToVec4(newVelSimd);

      itPosition.Advance();
      itVelocity.Advance();
    }
  }
  else
  {
    // Direct position offset mode
    while (!itPosition.HasReachedEnd())
    {
      const ezSimdVec4f simPos = itPosition.Current();

      const ezSimdVec4f sPos = ezSimdVec4f::MulAdd(simPos, simdFreq, simdScrollFreq);
      const float sx = sPos.GetComponent<0>();
      const float sy = sPos.GetComponent<1>();
      const float sz = sPos.GetComponent<2>();

      const ezSimdVec4f noise = m_Noise.NoiseZeroToOne(ezSimdVec4f(sx, sx + kOffset1, sx + kOffset2, 0), ezSimdVec4f(sy, sy + kOffset1, sy + kOffset2, 0), ezSimdVec4f(sz, sz + kOffset1, sz + kOffset2, 0), m_uiOctaves);

      const ezSimdVec4f offset = (noise - constHalf).CompMul(constMul);

      itPosition.Current() = simPos + offset;

      itPosition.Advance();
    }
  }
}


EZ_STATICLINK_FILE(ParticlePlugin, ParticlePlugin_Behavior_ParticleBehavior_Turbulence);
