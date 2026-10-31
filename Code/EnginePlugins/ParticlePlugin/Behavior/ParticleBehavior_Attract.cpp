#include <ParticlePlugin/ParticlePluginPCH.h>

#include <Core/World/SpatialSystem.h>
#include <Core/World/World.h>
#include <Foundation/DataProcessing/Stream/ProcessingStreamIterator.h>
#include <Foundation/Profiling/Profiling.h>
#include <Foundation/SimdMath/SimdConversion.h>
#include <ParticlePlugin/Behavior/ParticleBehavior_Attract.h>
#include <ParticlePlugin/Components/ParticleAttractorComponent.h>
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
    EZ_MEMBER_PROPERTY("MaxAttractors", m_uiMaxAttractors)->AddAttributes(new ezDefaultValueAttribute(1), new ezClampValueAttribute(1, 4)),
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

void ezParticleBehaviorFactory_Attract::Load(ezStreamReader& inout_stream, const ezParticleEffectDescriptor& ownerEffectDescriptor, const ezParticleSystemDescriptor& ownerSystemDescriptor)
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
}

void ezParticleBehavior_Attract::OnFinalize()
{
  RequestUpdateFromWorld();

  // search right away in the first UpdateFromWorld()
  m_uiAttractorSearchTimer = 0;
}

void ezParticleBehavior_Attract::UpdateFromWorld(const ezWorld& world, ezGameObjectHandle hOwnerObject)
{
  const ezParticleEffectInstance* pEffect = GetOwnerEffect();

  // Refresh which attractors are nearby roughly once per second.
  if (m_uiAttractorSearchTimer == 0)
  {
    m_uiAttractorSearchTimer = 90;
    m_AttractorHandles.Clear();

    if (const ezSpatialSystem* pSpatialSystem = world.GetSpatialSystem())
    {
      const ezSpatialData::Category category = ezParticleAttractorComponent::GetSpatialCategory();

      ezBoundingBoxSphere effectBounds;
      pEffect->GetBoundingVolume(effectBounds);

      const ezVec3 worldCenter = pEffect->GetTransform().TransformPosition(effectBounds.m_vCenter);
      const float fSearchRadius = ezMath::Max(0.0f, effectBounds.m_fSphereRadius);

      ezSpatialSystem::QueryParams queryParams;
      queryParams.m_uiCategoryBitmask = category.GetBitmask();

      struct SortEntry
      {
        EZ_DECLARE_POD_TYPE();
        float fDistSqr;
        ezComponentHandle hComponent;
      };

      ezTempHybridArray<SortEntry, 8> candidates;

      pSpatialSystem->FindObjectsInSphere(
        ezBoundingSphere::MakeFromCenterAndRadius(worldCenter, fSearchRadius),
        queryParams,
        [&](ezGameObject* pObject) -> ezVisitorExecution::Enum
        {
          ezParticleAttractorComponent* pAttractor = nullptr;
          if (pObject->TryGetComponentOfBaseType(pAttractor))
          {
            SortEntry entry;
            entry.hComponent = pAttractor->GetHandle();
            entry.fDistSqr = (pObject->GetGlobalPosition() - worldCenter).GetLengthSquared();
            candidates.PushBack(entry);
          }
          return ezVisitorExecution::Continue;
        });

      candidates.Sort([](const SortEntry& a, const SortEntry& b)
        { return a.fDistSqr < b.fDistSqr; });

      // older assets may still contain larger values than the property allows now
      const ezUInt32 uiCount = ezMath::Min<ezUInt32>(candidates.GetCount(), ezMath::Min<ezUInt8>(m_uiMaxAttractors, 4));
      for (ezUInt32 i = 0; i < uiCount; ++i)
      {
        m_AttractorHandles.PushBack(candidates[i].hComponent);
      }
    }
  }
  else
  {
    m_uiAttractorSearchTimer--;
  }

  // Every frame: resolve the handles and read the current position and properties.
  m_Attractors.Clear();
  for (const ezComponentHandle& hComponent : m_AttractorHandles)
  {
    const ezParticleAttractorComponent* pAttractor = nullptr;
    if (world.TryGetComponent(hComponent, pAttractor))
    {
      AttractorData& data = m_Attractors.ExpandAndGetRef();
      data.m_vPosition = pAttractor->GetOwner()->GetGlobalPosition();
      data.m_fStrength = pAttractor->m_fStrength;
      data.m_fRadius = pAttractor->m_fRadius;
      data.m_fMinDistance = pAttractor->m_fMinDistance;
      data.m_fKillDistance = pAttractor->m_fKillDistance;
    }
  }
}

void ezParticleBehavior_Attract::Process(ezUInt64 uiNumElements)
{
  EZ_PROFILE_SCOPE("PFX: AttractToPosition");

  const float tDiff = m_TimeDiff.AsFloatInSeconds();
  if (tDiff <= 0.0f)
    return;

  // written by UpdateFromWorld() on the main thread
  const ezArrayPtr<const AttractorData> attractors = m_Attractors;

  if (attractors.IsEmpty())
    return;

  if (m_bAffectVelocity)
  {
    for (const AttractorData& a : attractors)
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
    for (const AttractorData& a : attractors)
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
