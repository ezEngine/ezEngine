#include <ParticlePlugin/PCH.h>
#include <ParticlePlugin/Behavior/ParticleBehavior_Raycast.h>
#include <GameUtils/Interfaces/PhysicsWorldModule.h>
#include <ParticlePlugin/System/ParticleSystemInstance.h>
#include <Core/World/World.h>
#include <Foundation/Time/Clock.h>
#include <CoreUtils/DataProcessing/Stream/ProcessingStreamIterator.h>
#include <ParticlePlugin/Events/ParticleEvent.h>
#include <ParticlePlugin/Effect/ParticleEffectInstance.h>

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezParticleBehaviorFactory_Raycast, 1, ezRTTIDefaultAllocator<ezParticleBehaviorFactory_Raycast>)
{
  EZ_BEGIN_PROPERTIES
  {
    EZ_ENUM_MEMBER_PROPERTY("Reaction", ezParticleRaycastHitReaction, m_Reaction),
    EZ_MEMBER_PROPERTY("Collision Layer", m_uiCollisionLayer)->AddAttributes(new ezDynamicEnumAttribute("PhysicsCollisionLayer")),
    EZ_MEMBER_PROPERTY("OnCollide Event", m_sOnCollideEvent),
  }
  EZ_END_PROPERTIES
}
EZ_END_DYNAMIC_REFLECTED_TYPE

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezParticleBehavior_Raycast, 1, ezRTTIDefaultAllocator<ezParticleBehavior_Raycast>)
EZ_END_DYNAMIC_REFLECTED_TYPE

EZ_BEGIN_STATIC_REFLECTED_ENUM(ezParticleRaycastHitReaction, 1)
EZ_ENUM_CONSTANTS(ezParticleRaycastHitReaction::Bounce, ezParticleRaycastHitReaction::Die)
EZ_END_STATIC_REFLECTED_ENUM()

ezParticleBehaviorFactory_Raycast::ezParticleBehaviorFactory_Raycast()
{
  m_uiCollisionLayer = 0;
}


const ezRTTI* ezParticleBehaviorFactory_Raycast::GetBehaviorType() const
{
  return ezGetStaticRTTI<ezParticleBehavior_Raycast>();
}

void ezParticleBehaviorFactory_Raycast::CopyBehaviorProperties(ezParticleBehavior* pObject) const
{
  ezParticleBehavior_Raycast* pBehavior = static_cast<ezParticleBehavior_Raycast*>(pObject);

  pBehavior->m_Reaction = m_Reaction;
  pBehavior->m_uiCollisionLayer = m_uiCollisionLayer;
  pBehavior->m_sOnCollideEvent = ezTempHashedString(m_sOnCollideEvent.GetData());
}

enum class BehaviorRaycastVersion
{
  Version_0 = 0,
  Version_1,
  Version_2, // added event

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
}

void ezParticleBehaviorFactory_Raycast::Load(ezStreamReader& stream)
{
  ezUInt8 uiVersion = 0;
  stream >> uiVersion;

  EZ_ASSERT_DEV(uiVersion <= (int)BehaviorRaycastVersion::Version_Current, "Invalid version %u", uiVersion);

  if (uiVersion >= 2)
  {
    stream >> m_uiCollisionLayer;
    stream >> m_sOnCollideEvent;

    ezParticleRaycastHitReaction::StorageType hr;
    stream >> hr;
    m_Reaction.SetValue(hr);
  }
}

void ezParticleBehavior_Raycast::AfterPropertiesConfigured(bool bFirstTime)
{
  m_pPhysicsModule = static_cast<ezPhysicsWorldModuleInterface*>(ezWorldModule::FindModule(GetOwnerSystem()->GetWorld(), ezPhysicsWorldModuleInterface::GetStaticRTTI()));
}


void ezParticleBehavior_Raycast::CreateRequiredStreams()
{
  CreateStream("Position", ezProcessingStream::DataType::Float3, &m_pStreamPosition, false);
  CreateStream("LastPosition", ezProcessingStream::DataType::Float3, &m_pStreamLastPosition, false);
  CreateStream("Velocity", ezProcessingStream::DataType::Float3, &m_pStreamVelocity, false);
}

void ezParticleBehavior_Raycast::Process(ezUInt64 uiNumElements)
{
  const float tDiff = (float)m_TimeDiff.GetSeconds();

  ezProcessingStreamIterator<ezVec3> itPosition(m_pStreamPosition, uiNumElements);
  ezProcessingStreamIterator<ezVec3> itLastPosition(m_pStreamLastPosition, uiNumElements);
  ezProcessingStreamIterator<ezVec3> itVelocity(m_pStreamVelocity, uiNumElements);

  ezGameObjectHandle hHitObj;
  ezSurfaceResourceHandle hHitSurface;

  ezUInt32 i = 0;
  while (!itPosition.HasReachedEnd())
  {
    const ezVec3 vLastPos = itLastPosition.Current();
    const ezVec3 vCurPos = itPosition.Current();

    if (!vLastPos.IsZero())
    {
      const ezVec3 vChange = vCurPos - vLastPos;

      if (!vChange.IsZero(0.001f))
      {
        ezVec3 vDirection = vChange;
        const float fMaxLen = vDirection.GetLengthAndNormalize();

        ezVec3 vHitPos, vHitNorm;
        if (m_pPhysicsModule->CastRay(vLastPos, vDirection, fMaxLen, m_uiCollisionLayer, vHitPos, vHitNorm, hHitObj, hHitSurface))
        {
          if (m_Reaction == ezParticleRaycastHitReaction::Bounce)
          {
            const ezVec3 vNewDir = vChange.GetReflectedVector(vHitNorm);

            itPosition.Current() = vHitPos + vHitNorm * 0.05f + vNewDir;
            itVelocity.Current() = vNewDir / tDiff;
          }
          else if (m_Reaction == ezParticleRaycastHitReaction::Die)
          {
            /// \todo Get current element index from iterator ?
            m_pStreamGroup->RemoveElement(i);
          }

          if (m_sOnCollideEvent.GetHash() != 0)
          {
            ezParticleEvent e;
            e.m_vPosition = vHitPos;
            e.m_vNormal = vHitNorm;
            e.m_vDirection = vDirection;

            GetOwnerEffect()->GetEventQueue(m_sOnCollideEvent)->AddEvent(e);
          }
        }
      }
    }

    itLastPosition.Current() = itPosition.Current();

    itPosition.Advance();
    itLastPosition.Advance();
    itVelocity.Advance();

    ++i;
  }
}

