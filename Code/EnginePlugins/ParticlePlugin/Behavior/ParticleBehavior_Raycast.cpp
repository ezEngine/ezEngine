#include <ParticlePlugin/ParticlePluginPCH.h>

#include <Core/Interfaces/PhysicsWorldModule.h>
#include <Core/World/World.h>
#include <Foundation/DataProcessing/Stream/ProcessingStreamIterator.h>
#include <Foundation/Math/Float16.h>
#include <Foundation/Profiling/Profiling.h>
#include <ParticlePlugin/Behavior/ParticleBehavior_Raycast.h>
#include <ParticlePlugin/Effect/ParticleEffectInstance.h>
#include <ParticlePlugin/Events/ParticleEvent.h>
#include <ParticlePlugin/Finalizer/ParticleFinalizer_ApplyVelocity.h>
#include <ParticlePlugin/Finalizer/ParticleFinalizer_LastPosition.h>
#include <ParticlePlugin/System/ParticleSystemInstance.h>
#include <ParticlePlugin/WorldModule/ParticleWorldModule.h>
#include <RendererCore/Debug/DebugRenderer.h>

// clang-format off
EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezParticleBehavior_Raycast, 1, ezRTTIDefaultAllocator<ezParticleBehavior_Raycast>)
{
  EZ_BEGIN_PROPERTIES
  {
    EZ_ENUM_MEMBER_PROPERTY("Reaction", ezParticleRaycastHitReaction, m_Reaction),
    EZ_MEMBER_PROPERTY("BounceFactor", m_fBounceFactor)->AddAttributes(new ezDefaultValueAttribute(0.5f), new ezClampValueAttribute(0.0f, 1.0f)),
    EZ_MEMBER_PROPERTY("SlideFactor", m_fSlideFactor)->AddAttributes(new ezDefaultValueAttribute(0.5f), new ezClampValueAttribute(0.0f, 1.0f)),
    EZ_MEMBER_PROPERTY("SizeFactor", m_fSizeFactor)->AddAttributes(new ezDefaultValueAttribute(0.1f), new ezClampValueAttribute(0.0f, 1.0f)),
    EZ_MEMBER_PROPERTY("CollisionLayer", m_uiCollisionLayer)->AddAttributes(new ezDynamicEnumAttribute("PhysicsCollisionLayer")),
    EZ_MEMBER_PROPERTY("OnCollideEvent", m_sOnCollideEvent)->AddAttributes(new ezDynamicStringEnumAttribute("ParticleEventNamesEnum")),
  }
  EZ_END_PROPERTIES;
}
EZ_END_DYNAMIC_REFLECTED_TYPE;

EZ_BEGIN_STATIC_REFLECTED_ENUM(ezParticleRaycastHitReaction, 1)
  EZ_ENUM_CONSTANTS(ezParticleRaycastHitReaction::Bounce, ezParticleRaycastHitReaction::Die, ezParticleRaycastHitReaction::Stop)
EZ_END_STATIC_REFLECTED_ENUM;
// clang-format on

ezParticleBehavior_Raycast::~ezParticleBehavior_Raycast() = default;

enum class BehaviorRaycastVersion
{
  Version_0 = 0,
  Version_1,
  Version_2, // added event
  Version_3, // added bounce factor
  Version_4, // added slide and size factor

  // insert new version numbers above
  Version_Count,
  Version_Current = Version_Count - 1
};


void ezParticleBehavior_Raycast::Save(ezStreamWriter& inout_stream) const
{
  const ezUInt8 uiVersion = (int)BehaviorRaycastVersion::Version_Current;
  inout_stream << uiVersion;

  inout_stream << m_uiCollisionLayer;
  inout_stream << m_sOnCollideEvent;
  inout_stream << m_Reaction;
  inout_stream << m_fBounceFactor;
  inout_stream << m_fSlideFactor;
  inout_stream << m_fSizeFactor;
}

void ezParticleBehavior_Raycast::Load(ezStreamReader& inout_stream, const ezParticleEffectDescriptor& ownerEffectDescriptor, const ezParticleSystemDescriptor& ownerSystemDescriptor)
{
  ezUInt8 uiVersion = 0;
  inout_stream >> uiVersion;

  EZ_ASSERT_DEV(uiVersion <= (int)BehaviorRaycastVersion::Version_Current, "Invalid version {0}", uiVersion);

  if (uiVersion >= 2)
  {
    inout_stream >> m_uiCollisionLayer;
    inout_stream >> m_sOnCollideEvent;
    inout_stream >> m_Reaction;
  }

  if (uiVersion >= 3)
  {
    inout_stream >> m_fBounceFactor;
  }

  if (uiVersion >= 4)
  {
    inout_stream >> m_fSlideFactor;
    inout_stream >> m_fSizeFactor;
  }
}

void ezParticleBehavior_Raycast::QueryFinalizerDependencies(ezSet<const ezRTTI*>& inout_finalizerDeps) const
{
  inout_finalizerDeps.Insert(ezGetStaticRTTI<ezParticleFinalizer_ApplyVelocity>());
  inout_finalizerDeps.Insert(ezGetStaticRTTI<ezParticleFinalizer_LastPosition>());
}

//////////////////////////////////////////////////////////////////////////

bool ezParticleBehavior_Raycast::GetInstanceDataDesc(ezInstanceDataDesc& out_desc) const
{
  out_desc.FillFromType<InstanceData>();
  return true;
}

ezParticleBehavior_Raycast::ezParticleBehavior_Raycast()
{
  // do this right after ezParticleFinalizer_ApplyVelocity has run
  m_fPriority = 526.0f;
}

void ezParticleBehavior_Raycast::CreateRequiredStreams(const ezParticleModuleContext& ctx) const
{
  InstanceData& data = ctx.GetInstanceData<InstanceData>();

  ctx.CreateStream(ezParticleStreamNames::s_sPosition, ezProcessingStream::DataType::Float4, &data.m_pStreamPosition, false);
  ctx.CreateStream(ezParticleStreamNames::s_sLastPosition, ezProcessingStream::DataType::Float3, &data.m_pStreamLastPosition, false);
  ctx.CreateStream(ezParticleStreamNames::s_sVelocity, ezProcessingStream::DataType::Half4, &data.m_pStreamVelocity, false);
}

void ezParticleBehavior_Raycast::QueryOptionalStreams(const ezParticleModuleContext& ctx) const
{
  InstanceData& data = ctx.GetInstanceData<InstanceData>();

  data.m_pStreamSize = ctx.m_pSystem->QueryStream(ezParticleStreamNames::s_sSize, ezProcessingStream::DataType::Half);
}

void ezParticleBehavior_Raycast::OnFinalize(const ezParticleModuleContext& ctx) const
{
  InstanceData& data = ctx.GetInstanceData<InstanceData>();

  data.m_pPhysicsModule = (ezPhysicsWorldModuleInterface*)ctx.m_pSystem->GetOwnerWorldModule()->GetCachedWorldModule(ezGetStaticRTTI<ezPhysicsWorldModuleInterface>());
}

void ezParticleBehavior_Raycast::Process(const ezParticleModuleContext& ctx, ezUInt64 uiNumElements) const
{
  InstanceData& data = ctx.GetInstanceData<InstanceData>();

  EZ_PROFILE_SCOPE("PFX: Raycast");

  const float tDiff = (float)ctx.GetTimeDiff().GetSeconds();

  ezProcessingStreamIterator<ezVec4> itPosition(data.m_pStreamPosition, uiNumElements, 0);
  ezProcessingStreamIterator<const ezVec3> itLastPosition(data.m_pStreamLastPosition, uiNumElements, 0);
  ezProcessingStreamIterator<ezFloat16Vec4> itVelocity(data.m_pStreamVelocity, uiNumElements, 0);

  ezFloat16 fDummySize = 0.0f;
  const ezFloat16* pSize = data.m_pStreamSize != nullptr ? data.m_pStreamSize->GetData<ezFloat16>() : &fDummySize;

  ezPhysicsCastResult hitResult;

  ezUInt32 i = 0;
  while (!itPosition.HasReachedEnd())
  {
    const ezVec3 vLastPos = itLastPosition.Current();
    const ezVec3 vCurPos = itPosition.Current().GetAsVec3();

    if (!vLastPos.IsZero())
    {
      const ezVec3 vChange = vCurPos - vLastPos;

      if (!vChange.IsZero(ezMath::DefaultEpsilon<float>()))
      {
        ezVec3 vDirection = vChange;

        const float fSize = ezMath::Max(*pSize * m_fSizeFactor, 0.01f);
        const float fMaxLen = vDirection.GetLengthAndNormalize();

        ezPhysicsQueryParameters query(m_uiCollisionLayer);
        query.m_ShapeTypes = ezPhysicsShapeType::Static | ezPhysicsShapeType::Dynamic;

        if (data.m_pPhysicsModule != nullptr && data.m_pPhysicsModule->Raycast(hitResult, vLastPos, vDirection, fMaxLen + fSize, query))
        {
          hitResult.m_vPosition -= vDirection * fSize;
          const float fRemainingLen = (vCurPos - hitResult.m_vPosition).GetLength();
          const float fRemainder = fRemainingLen / fMaxLen;

          if (m_Reaction == ezParticleRaycastHitReaction::Bounce)
          {
            const ezVec3 vTangentDir = vChange - hitResult.m_vNormal * hitResult.m_vNormal.Dot(vChange);
            const ezVec3 vNormalDir = vTangentDir - vChange;

            const ezVec3 vNewDir = vNormalDir * m_fBounceFactor + vTangentDir * m_fSlideFactor;

            if (vNewDir.GetLengthSquared() < ezMath::Square(0.01f))
            {
              itPosition.Current() = hitResult.m_vPosition.GetAsPositionVec4();
              itVelocity.Current() = ezVec4(0, 0, 1, 0);
            }
            else
            {
              itPosition.Current() = (hitResult.m_vPosition + vNewDir * fRemainder).GetAsVec4(0);

              const ezVec3 newVel = vNewDir / tDiff;
              const float newSpeed = newVel.GetLength();
              const ezVec3 newDir = newSpeed > 0.0f ? newVel / newSpeed : ezVec3(0, 0, 1);

              itVelocity.Current() = ezVec4(newDir.x, newDir.y, newDir.z, newSpeed);
            }
          }
          else if (m_Reaction == ezParticleRaycastHitReaction::Die)
          {
            /// \todo Get current element index from iterator ?
            ctx.m_pSystem->RemoveParticle(i);
          }
          else if (m_Reaction == ezParticleRaycastHitReaction::Stop)
          {
            itPosition.Current() = hitResult.m_vPosition.GetAsPositionVec4();
            itVelocity.Current() = ezVec4(0, 0, 1, 0);
          }

          if (!m_sOnCollideEvent.IsEmpty())
          {
            ezParticleEvent e;
            e.m_EventType = m_sOnCollideEvent;
            e.m_vPosition = hitResult.m_vPosition;
            e.m_vNormal = hitResult.m_vNormal;
            e.m_vDirection = vDirection;

            ctx.GetEffect()->AddParticleEvent(e);
          }
        }

        if constexpr (false)
        {
          ezDebugRenderer::DrawLineSphere(data.m_pPhysicsModule->GetWorld(), ezBoundingSphere::MakeFromCenterAndRadius(itPosition.Current().GetAsVec3(), fSize), ezColor::Red);
        }
      }
    }

    itPosition.Advance();
    itLastPosition.Advance();
    itVelocity.Advance();

    if (data.m_pStreamSize != nullptr)
      ++pSize;

    ++i;
  }
}

void ezParticleBehavior_Raycast::RequestRequiredWorldModulesForCache(ezParticleWorldModule* pParticleModule) const
{
  pParticleModule->CacheWorldModule<ezPhysicsWorldModuleInterface>();
}

EZ_STATICLINK_FILE(ParticlePlugin, ParticlePlugin_Behavior_ParticleBehavior_Raycast);
