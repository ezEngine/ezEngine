#include <ParticlePlugin/ParticlePluginPCH.h>

#include <Foundation/DataProcessing/Stream/ProcessingStreamIterator.h>
#include <Foundation/Profiling/Profiling.h>
#include <Foundation/SimdMath/SimdConversion.h>
#include <ParticlePlugin/Behavior/ParticleBehavior_Attract.h>
#include <ParticlePlugin/Effect/ParticleEffectInstance.h>
#include <ParticlePlugin/Finalizer/ParticleFinalizer_ApplyVelocity.h>
#include <ParticlePlugin/System/ParticleSystemInstance.h>

// clang-format off
EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezParticleBehaviorFactory_Attract, 1, ezRTTIDefaultAllocator<ezParticleBehaviorFactory_Attract>)
{
  EZ_BEGIN_PROPERTIES
  {
    EZ_MEMBER_PROPERTY("Influence", m_fInfluence)->AddAttributes(new ezDefaultValueAttribute(1.0f)),
    EZ_MEMBER_PROPERTY("AffectVelocity", m_bAffectVelocity)->AddAttributes(new ezDefaultValueAttribute(true)),
    EZ_MEMBER_PROPERTY("MaxAttractors", m_uiMaxAttractors)->AddAttributes(new ezDefaultValueAttribute(1), new ezClampValueAttribute(1, 8)),
  }
  EZ_END_PROPERTIES;
}
EZ_END_DYNAMIC_REFLECTED_TYPE;

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezParticleBehavior_Attract, 1, ezRTTIDefaultAllocator<ezParticleBehavior_Attract>)
EZ_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

ezParticleBehaviorFactory_Attract::ezParticleBehaviorFactory_Attract() = default;

const ezRTTI* ezParticleBehaviorFactory_Attract::GetBehaviorType() const
{
  return ezGetStaticRTTI<ezParticleBehavior_Attract>();
}

void ezParticleBehaviorFactory_Attract::CopyBehaviorProperties(ezParticleBehavior* pObject, bool bFirstTime) const
{
  ezParticleBehavior_Attract* pBehavior = static_cast<ezParticleBehavior_Attract*>(pObject);

  pBehavior->m_fInfluence = m_fInfluence;
  pBehavior->m_bAffectVelocity = m_bAffectVelocity;
  pBehavior->m_uiMaxAttractors = m_uiMaxAttractors;
}

void ezParticleBehaviorFactory_Attract::QueryFinalizerDependencies(ezSet<const ezRTTI*>& inout_finalizerDeps) const
{
  if (m_bAffectVelocity)
  {
    inout_finalizerDeps.Insert(ezGetStaticRTTI<ezParticleFinalizerFactory_ApplyVelocity>());
  }
}

void ezParticleBehaviorFactory_Attract::Save(ezStreamWriter& inout_stream) const
{
  inout_stream.WriteVersion(1);

  inout_stream << m_fInfluence;
  inout_stream << m_bAffectVelocity;
  inout_stream << m_uiMaxAttractors;
}

void ezParticleBehaviorFactory_Attract::Load(ezStreamReader& inout_stream)
{
  /*const auto version =*/inout_stream.ReadVersion(1);

  inout_stream >> m_fInfluence;
  inout_stream >> m_bAffectVelocity;
  inout_stream >> m_uiMaxAttractors;
}

void ezParticleBehavior_Attract::CreateRequiredStreams()
{
  CreateStream("Position", ezProcessingStream::DataType::Float4, &m_pStreamPosition, false);

  if (m_bAffectVelocity)
  {
    CreateStream("Velocity", ezProcessingStream::DataType::Half4, &m_pStreamVelocity, false);
  }

  GetOwnerEffect()->RequestAttractorSamples(m_uiMaxAttractors);
}

void ezParticleBehavior_Attract::Process(ezUInt64 uiNumElements)
{
  EZ_PROFILE_SCOPE("PFX: AttractToPosition");

  const float tDiff = m_TimeDiff.AsFloatInSeconds();
  if (tDiff <= 0.0f)
    return;

  // Attractor data is pre-computed every frame by FindNearbyAttractors() on the main thread.
  const ezArrayPtr<const ezParticleAttractorData> attractors = GetOwnerEffect()->GetAttractorData();

  if (attractors.IsEmpty())
    return;

  if (m_bAffectVelocity)
  {
    for (const ezParticleAttractorData& a : attractors)
    {
      const ezSimdVec4f vAttractorPos = ezSimdConversion::ToVec3(a.m_vPosition);
      const ezSimdFloat fMaxDistSqr = a.m_fRadius * a.m_fRadius;
      const ezSimdFloat fMinDistSqr = a.m_fMinDistance * a.m_fMinDistance;
      const ezSimdFloat fRadius = a.m_fRadius;
      const ezSimdFloat fInvRange = (a.m_fRadius > a.m_fMinDistance) ? (1.0f / (a.m_fRadius - a.m_fMinDistance)) : 0.0f;
      const ezSimdFloat fScaledStrength = a.m_fStrength * m_fInfluence * tDiff;
      const ezSimdFloat fKillDistSqr = a.m_fKillDistance * a.m_fKillDistance;

      ezProcessingStreamIterator<ezSimdVec4f> itPosition(m_pStreamPosition, uiNumElements, 0);
      ezProcessingStreamIterator<ezFloat16Vec4> itVelocity(m_pStreamVelocity, uiNumElements, 0);
      ezUInt32 idx = 0;

      while (!itPosition.HasReachedEnd())
      {
        const ezSimdVec4f vPos = itPosition.Current();
        const ezSimdVec4f vToTarget = vAttractorPos - vPos;
        const ezSimdFloat fDistSqr = vToTarget.GetLengthSquared<3>();

        if (fDistSqr < fKillDistSqr)
        {
          m_pStreamGroup->RemoveElement(idx);
        }
        else if (fDistSqr < fMaxDistSqr && fDistSqr > fMinDistSqr)
        {
          ezSimdVec4f vDir = vToTarget;
          const ezSimdFloat fDist = vDir.GetLengthAndNormalize<3, ezMathAcc::BITS_23>();

          const ezSimdFloat fAcceleration = fScaledStrength * ((fRadius - fDist) * fInvRange);

          const ezVec4 vel = itVelocity.Current();
          ezSimdVec4f vNewVel = ezSimdVec4f(vel.x, vel.y, vel.z, 0.0f) * ezSimdFloat(vel.w) + vDir * fAcceleration;
          const ezSimdFloat fNewSpeed = vNewVel.GetLengthAndNormalize<3, ezMathAcc::BITS_23>();

          itVelocity.Current() = ezVec4(vNewVel.GetComponent<0>(), vNewVel.GetComponent<1>(), vNewVel.GetComponent<2>(), fNewSpeed);
        }

        ++idx;
        itPosition.Advance();
        itVelocity.Advance();
      }
    }
  }
  else
  {
    for (const ezParticleAttractorData& a : attractors)
    {
      const ezSimdVec4f vAttractorPos = ezSimdConversion::ToVec3(a.m_vPosition);
      const ezSimdFloat fMaxDistSqr = a.m_fRadius * a.m_fRadius;
      const ezSimdFloat fMinDistSqr = a.m_fMinDistance * a.m_fMinDistance;
      const ezSimdFloat fRadius = a.m_fRadius;
      const ezSimdFloat fInvRange = (a.m_fRadius > a.m_fMinDistance) ? (1.0f / (a.m_fRadius - a.m_fMinDistance)) : 0.0f;
      const ezSimdFloat fScaledStrength = a.m_fStrength * m_fInfluence * tDiff;
      const ezSimdFloat fKillDistSqr = a.m_fKillDistance * a.m_fKillDistance;

      ezProcessingStreamIterator<ezSimdVec4f> itPosition(m_pStreamPosition, uiNumElements, 0);
      ezUInt32 idx = 0;

      while (!itPosition.HasReachedEnd())
      {
        const ezSimdVec4f vPos = itPosition.Current();
        const ezSimdVec4f vToTarget = vAttractorPos - vPos;
        const ezSimdFloat fDistSqr = vToTarget.GetLengthSquared<3>();

        if (fDistSqr < fKillDistSqr)
        {
          m_pStreamGroup->RemoveElement(idx);
        }
        else if (fDistSqr < fMaxDistSqr && fDistSqr > fMinDistSqr)
        {
          ezSimdVec4f vDir = vToTarget;
          const ezSimdFloat fDist = vDir.GetLengthAndNormalize<3, ezMathAcc::BITS_23>();

          const ezSimdFloat fMoveAmount = fScaledStrength * ((fRadius - fDist) * fInvRange);

          // Clamp move amount so particles don't overshoot the min-distance shell.
          const ezSimdFloat fMaxMove = fDist - ezSimdFloat(a.m_fMinDistance);
          itPosition.Current() = vPos + vDir * fMoveAmount.Min(fMaxMove);
        }

        ++idx;
        itPosition.Advance();
      }
    }
  }
}


EZ_STATICLINK_FILE(ParticlePlugin, ParticlePlugin_Behavior_ParticleBehavior_Attract);
