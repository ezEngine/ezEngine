#include <ParticlePluginPCH.h>

#include <Core/World/World.h>
#include <Foundation/DataProcessing/Stream/ProcessingStreamIterator.h>
#include <Foundation/Math/Float16.h>
#include <Foundation/Profiling/Profiling.h>
#include <Foundation/Time/Clock.h>
#include <GameEngine/Interfaces/PhysicsWorldModule.h>
#include <ParticlePlugin/Behavior/ParticleBehavior_Raycast.h>
#include <ParticlePlugin/Effect/ParticleEffectInstance.h>
#include <ParticlePlugin/Events/ParticleEvent.h>
#include <ParticlePlugin/Finalizer/ParticleFinalizer_ApplyVelocity.h>
#include <ParticlePlugin/Finalizer/ParticleFinalizer_LastPosition.h>
#include <ParticlePlugin/System/ParticleSystemInstance.h>

// clang-format off
EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezParticleBehaviorFactory_Raycast, 1, ezRTTIDefaultAllocator<ezParticleBehaviorFactory_Raycast>)
{
  EZ_BEGIN_PROPERTIES
  {
    EZ_ENUM_MEMBER_PROPERTY("Reaction", ezParticleRaycastHitReaction, m_Reaction),
    EZ_MEMBER_PROPERTY("BounceFactor", m_fBounceFactor)->AddAttributes(new ezDefaultValueAttribute(0.6f), new ezClampValueAttribute(0.0f, 1.0f)),
    EZ_MEMBER_PROPERTY("CollisionLayer", m_uiCollisionLayer)->AddAttributes(new ezDynamicEnumAttribute("PhysicsCollisionLayer")),
    EZ_MEMBER_PROPERTY("OnCollideEvent", m_sOnCollideEvent),
  }
  EZ_END_PROPERTIES;
}
EZ_END_DYNAMIC_REFLECTED_TYPE;

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezParticleBehavior_Raycast, 1, ezRTTIDefaultAllocator<ezParticleBehavior_Raycast>)
EZ_END_DYNAMIC_REFLECTED_TYPE;

EZ_BEGIN_STATIC_REFLECTED_ENUM(ezParticleRaycastHitReaction, 1)
  EZ_ENUM_CONSTANTS(ezParticleRaycastHitReaction::Bounce, ezParticleRaycastHitReaction::Die, ezParticleRaycastHitReaction::Stop)
EZ_END_STATIC_REFLECTED_ENUM;
// clang-format on

ezParticleBehaviorFactory_Raycast::ezParticleBehaviorFactory_Raycast() = default;
ezParticleBehaviorFactory_Raycast::~ezParticleBehaviorFactory_Raycast() = default;

const ezRTTI* ezParticleBehaviorFactory_Raycast::GetBehaviorType() const
{
  return ezGetStaticRTTI<ezParticleBehavior_Raycast>();
}

void ezParticleBehaviorFactory_Raycast::CopyBehaviorProperties(ezParticleBehavior* pObject, bool bFirstTime) const
{
  ezParticleBehavior_Raycast* pBehavior = static_cast<ezParticleBehavior_Raycast*>(pObject);

  pBehavior->m_Reaction = m_Reaction;
  pBehavior->m_uiCollisionLayer = m_uiCollisionLayer;
  pBehavior->m_sOnCollideEvent = ezTempHashedString(m_sOnCollideEvent.GetData());
  pBehavior->m_fBounceFactor = m_fBounceFactor;

  pBehavior->m_pPhysicsModule = (ezPhysicsWorldModuleInterface*)pBehavior->GetOwnerSystem()->GetOwnerWorldModule()->GetCachedWorldModule(ezGetStaticRTTI<ezPhysicsWorldModuleInterface>());
}

enum class BehaviorRaycastVersion
{
  Version_0 = 0,
  Version_1,
  Version_2, // added event
  Version_3, // added bounce factor

  // insert new version numbers above
  Version_Count,
  Version_Current = Version_Count - 1
};


void ezParticleBehaviorFactory_Raycast::Save(ezStreamWriter& stream) const
{
  const ezUInt8 uiVersion = (int)BehaviorRaycastVersion::Version_Current;
  stream << uiVersion;

  stream << m_uiCollisionLayer;
  stream << m_sOnCollideEvent;

  ezParticleRaycastHitReaction::StorageType hr = m_Reaction.GetValue();
  stream << hr;

  stream << m_fBounceFactor;
}

void ezParticleBehaviorFactory_Raycast::Load(ezStreamReader& stream)
{
  ezUInt8 uiVersion = 0;
  stream >> uiVersion;

  EZ_ASSERT_DEV(uiVersion <= (int)BehaviorRaycastVersion::Version_Current, "Invalid version {0}", uiVersion);

  if (uiVersion >= 2)
  {
    stream >> m_uiCollisionLayer;
    stream >> m_sOnCollideEvent;

    ezParticleRaycastHitReaction::StorageType hr;
    stream >> hr;
    m_Reaction.SetValue(hr);
  }

  if (uiVersion >= 3)
  {
    stream >> m_fBounceFactor;
  }
}

void ezParticleBehaviorFactory_Raycast::QueryFinalizerDependencies(ezSet<const ezRTTI*>& inout_FinalizerDeps) const
{
  inout_FinalizerDeps.Insert(ezGetStaticRTTI<ezParticleFinalizerFactory_ApplyVelocity>());
  inout_FinalizerDeps.Insert(ezGetStaticRTTI<ezParticleFinalizerFactory_LastPosition>());
}

//////////////////////////////////////////////////////////////////////////

void ezParticleBehavior_Raycast::CreateRequiredStreams()
{
  CreateStream("Position", ezProcessingStream::DataType::Float4, &m_pStreamPosition, false);
  CreateStream("LastPosition", ezProcessingStream::DataType::Float3, &m_pStreamLastPosition, false);
  CreateStream("Velocity", ezProcessingStream::DataType::Float3, &m_pStreamVelocity, false);
}

void ezParticleBehavior_Raycast::Process(ezUInt64 uiNumElements)
{
  EZ_PROFILE_SCOPE("PFX: Raycast");

  const float tDiff = (float)m_TimeDiff.GetSeconds();

  ezProcessingStreamIterator<ezVec4> itPosition(m_pStreamPosition, uiNumElements, 0);
  ezProcessingStreamIterator<const ezVec3> itLastPosition(m_pStreamLastPosition, uiNumElements, 0);
  ezProcessingStreamIterator<ezVec3> itVelocity(m_pStreamVelocity, uiNumElements, 0);

  ezPhysicsCastResult hitResult;

  ezUInt32 i = 0;
  while (!itPosition.HasReachedEnd())
  {
    const ezVec3 vLastPos = itLastPosition.Current();
    const ezVec3 vCurPos = itPosition.Current().GetAsVec3();

    if (!vLastPos.IsZero())
    {
      const ezVec3 vChange = vCurPos - vLastPos;

      if (!vChange.IsZero(0.001f))
      {
        ezVec3 vDirection = vChange;

        const float fMaxLen = vDirection.GetLengthAndNormalize();

        if (m_pPhysicsModule != nullptr && m_pPhysicsModule->Raycast(hitResult, vLastPos, vDirection, fMaxLen, ezPhysicsQueryParameters(m_uiCollisionLayer)))
        {
          if (m_Reaction == ezParticleRaycastHitReaction::Bounce)
          {
            const ezVec3 vNewDir = vChange.GetReflectedVector(hitResult.m_vNormal) * m_fBounceFactor;

            itPosition.Current() = ezVec3(hitResult.m_vPosition + hitResult.m_vNormal * 0.05f + vNewDir).GetAsVec4(0);
            itVelocity.Current() = vNewDir / tDiff;
          }
          else if (m_Reaction == ezParticleRaycastHitReaction::Die)
          {
            /// \todo Get current element index from iterator ?
            m_pStreamGroup->RemoveElement(i);
          }
          else if (m_Reaction == ezParticleRaycastHitReaction::Stop)
          {
            itVelocity.Current().SetZero();
          }

          if (m_sOnCollideEvent.GetHash() != 0)
          {
            ezParticleEvent e;
            e.m_EventType = m_sOnCollideEvent;
            e.m_vPosition = hitResult.m_vPosition;
            e.m_vNormal = hitResult.m_vNormal;
            e.m_vDirection = vDirection;

            GetOwnerEffect()->AddParticleEvent(e);
          }
        }
      }
    }

    itPosition.Advance();
    itLastPosition.Advance();
    itVelocity.Advance();

    ++i;
  }
}

void ezParticleBehavior_Raycast::RequestRequiredWorldModulesForCache(ezParticleWorldModule* pParticleModule)
{
  pParticleModule->CacheWorldModule<ezPhysicsWorldModuleInterface>();
}

EZ_STATICLINK_FILE(ParticlePlugin, ParticlePlugin_Behavior_ParticleBehavior_Raycast);
