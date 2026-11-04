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
EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezParticleBehavior_Attract, 1, ezRTTIDefaultAllocator<ezParticleBehavior_Attract>)
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
// clang-format on

ezParticleBehavior_Attract::ezParticleBehavior_Attract() = default;

void ezParticleBehavior_Attract::QueryFinalizerDependencies(ezSet<const ezRTTI*>& inout_finalizerDeps) const
{
  if (m_bAffectVelocity)
  {
    inout_finalizerDeps.Insert(ezGetStaticRTTI<ezParticleFinalizer_ApplyVelocity>());
  }
}

void ezParticleBehavior_Attract::Save(ezStreamWriter& inout_stream) const
{
  inout_stream.WriteVersion(1);

  inout_stream << m_fInfluence;
  inout_stream << m_bAffectVelocity;
  inout_stream << m_uiMaxAttractors;
}

void ezParticleBehavior_Attract::Load(ezStreamReader& inout_stream, const ezParticleEffectDescriptor& ownerEffectDescriptor, const ezParticleSystemDescriptor& ownerSystemDescriptor)
{
  /*const auto version =*/inout_stream.ReadVersion(1);

  inout_stream >> m_fInfluence;
  inout_stream >> m_bAffectVelocity;
  inout_stream >> m_uiMaxAttractors;
}

bool ezParticleBehavior_Attract::GetInstanceDataDesc(ezInstanceDataDesc& out_desc) const
{
  out_desc.FillFromType<InstanceData>();
  return true;
}

void ezParticleBehavior_Attract::CreateRequiredStreams(const ezParticleModuleContext& ctx) const
{
  InstanceData& data = ctx.GetInstanceData<InstanceData>();

  ctx.CreateStream(ezParticleStreamNames::s_sPosition, ezProcessingStream::DataType::Float4, &data.m_pStreamPosition, false);

  if (m_bAffectVelocity)
  {
    ctx.CreateStream(ezParticleStreamNames::s_sVelocity, ezProcessingStream::DataType::Half4, &data.m_pStreamVelocity, false);
  }
}

void ezParticleBehavior_Attract::OnFinalize(const ezParticleModuleContext& ctx) const
{
  InstanceData& data = ctx.GetInstanceData<InstanceData>();

  // search right away in the first UpdateFromWorld()
  data.m_uiAttractorSearchTimer = 0;
}

void ezParticleBehavior_Attract::UpdateFromWorld(const ezParticleModuleContext& ctx, const ezWorld& world, ezGameObjectHandle hOwnerObject) const
{
  InstanceData& data = ctx.GetInstanceData<InstanceData>();

  const ezParticleEffectInstance* pEffect = ctx.GetEffect();

  // Refresh which attractors are nearby roughly once per second.
  if (data.m_uiAttractorSearchTimer == 0)
  {
    data.m_uiAttractorSearchTimer = 90;
    data.m_AttractorHandles.Clear();

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
        data.m_AttractorHandles.PushBack(candidates[i].hComponent);
      }
    }
  }
  else
  {
    data.m_uiAttractorSearchTimer--;
  }

  // Every frame: resolve the handles and read the current position and properties.
  data.m_Attractors.Clear();
  for (const ezComponentHandle& hComponent : data.m_AttractorHandles)
  {
    const ezParticleAttractorComponent* pAttractor = nullptr;
    if (world.TryGetComponent(hComponent, pAttractor))
    {
      AttractorData& attractor = data.m_Attractors.ExpandAndGetRef();
      attractor.m_vPosition = pAttractor->GetOwner()->GetGlobalPosition();
      attractor.m_fStrength = pAttractor->m_fStrength;
      attractor.m_fRadius = pAttractor->m_fRadius;
      attractor.m_fMinDistance = pAttractor->m_fMinDistance;
      attractor.m_fKillDistance = pAttractor->m_fKillDistance;
    }
  }
}

void ezParticleBehavior_Attract::Process(const ezParticleModuleContext& ctx, ezUInt64 uiNumElements) const
{
  InstanceData& data = ctx.GetInstanceData<InstanceData>();

  EZ_PROFILE_SCOPE("PFX: AttractToPosition");

  const float tDiff = ctx.GetTimeDiff().AsFloatInSeconds();
  if (tDiff <= 0.0f)
    return;

  // written by UpdateFromWorld() on the main thread
  const ezArrayPtr<const AttractorData> attractors = data.m_Attractors;

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

      ezProcessingStreamIterator<ezSimdVec4f> itPosition(data.m_pStreamPosition, uiNumElements, 0);
      ezProcessingStreamIterator<ezFloat16Vec4> itVelocity(data.m_pStreamVelocity, uiNumElements, 0);
      ezUInt32 idx = 0;

      while (!itPosition.HasReachedEnd())
      {
        const ezSimdVec4f vPos = itPosition.Current();
        const ezSimdVec4f vToTarget = vAttractorPos - vPos;
        const ezSimdFloat fDistSqr = vToTarget.GetLengthSquared<3>();

        if (fDistSqr < fKillDistSqr)
        {
          ctx.m_pSystem->RemoveParticle(idx);
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

      ezProcessingStreamIterator<ezSimdVec4f> itPosition(data.m_pStreamPosition, uiNumElements, 0);
      ezUInt32 idx = 0;

      while (!itPosition.HasReachedEnd())
      {
        const ezSimdVec4f vPos = itPosition.Current();
        const ezSimdVec4f vToTarget = vAttractorPos - vPos;
        const ezSimdFloat fDistSqr = vToTarget.GetLengthSquared<3>();

        if (fDistSqr < fKillDistSqr)
        {
          ctx.m_pSystem->RemoveParticle(idx);
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
