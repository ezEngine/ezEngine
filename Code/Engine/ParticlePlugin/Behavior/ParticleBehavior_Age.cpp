#include <ParticlePlugin/PCH.h>
#include <ParticlePlugin/Behavior/ParticleBehavior_Age.h>
#include <ParticlePlugin/System/ParticleSystemInstance.h>
#include <CoreUtils/DataProcessing/Stream/StreamElementIterator.h>
#include <Core/World/World.h>
#include <ParticlePlugin/Effect/ParticleEffectInstance.h>
#include <ParticlePlugin/Events/ParticleEvent.h>

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezParticleBehaviorFactory_Age, 1, ezRTTIDefaultAllocator<ezParticleBehaviorFactory_Age>)
{
  EZ_BEGIN_PROPERTIES
  {
    EZ_MEMBER_PROPERTY("OnDeath Event", m_sOnDeathEvent),
  }
  EZ_END_PROPERTIES
}
EZ_END_DYNAMIC_REFLECTED_TYPE

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezParticleBehavior_Age, 1, ezRTTIDefaultAllocator<ezParticleBehavior_Age>)
EZ_END_DYNAMIC_REFLECTED_TYPE

const ezRTTI* ezParticleBehaviorFactory_Age::GetBehaviorType() const
{
  return ezGetStaticRTTI<ezParticleBehavior_Age>();
}

void ezParticleBehaviorFactory_Age::CopyBehaviorProperties(ezParticleBehavior* pObject) const
{
  ezParticleBehavior_Age* pBehavior = static_cast<ezParticleBehavior_Age*>(pObject);

  pBehavior->m_sOnDeathEvent = ezTempHashedString(m_sOnDeathEvent.GetData());
}


enum class BehaviorAgeVersion
{
  Version_0 = 0,
  Version_1,
  Version_2,

  // insert new version numbers above
  Version_Count,
  Version_Current = Version_Count - 1
};

void ezParticleBehaviorFactory_Age::Save(ezStreamWriter& stream) const
{
  const ezUInt8 uiVersion = (int)BehaviorAgeVersion::Version_Current;
  stream << uiVersion;

  stream << m_sOnDeathEvent;
}

void ezParticleBehaviorFactory_Age::Load(ezStreamReader& stream)
{
  ezUInt8 uiVersion = 0;
  stream >> uiVersion;

  EZ_ASSERT_DEV(uiVersion <= (int)BehaviorAgeVersion::Version_Current, "Invalid version %u", uiVersion);

  if (uiVersion >= 2)
  {
    stream >> m_sOnDeathEvent;
  }
}

void ezParticleBehavior_Age::CreateRequiredStreams()
{
  CreateStream("LifeTime", ezStream::DataType::Float2, &m_pStreamLifeTime);

  if (m_sOnDeathEvent.GetHash() != 0)
  {
    CreateStream("Position", ezStream::DataType::Float3, &m_pStreamPosition);
    CreateStream("Velocity", ezStream::DataType::Float3, &m_pStreamVelocity);
  }
}

void ezParticleBehavior_Age::Process(ezUInt64 uiNumElements)
{
  ezVec2* pLifeTime = m_pStreamLifeTime->GetWritableData<ezVec2>();

  const ezVec3* pPosition = nullptr;
  const ezVec3* pVelocity = nullptr;

  if (m_sOnDeathEvent.GetHash() != 0)
  {
    pPosition = m_pStreamPosition->GetData<ezVec3>();
    pVelocity = m_pStreamVelocity->GetData<ezVec3>();
  }

  const float tDiff = (float)m_TimeDiff.GetSeconds();

  for (ezUInt32 i = 0; i < uiNumElements; ++i)
  {
    pLifeTime[i].x -= tDiff;

    if (pLifeTime[i].x <= 0)
    {
      pLifeTime[i].x = 0;

      if (m_sOnDeathEvent.GetHash() != 0)
      {
        ezParticleEventQueue* pQueue = GetOwnerEffect()->GetEventQueue(m_sOnDeathEvent);

        ezParticleEvent e;
        e.m_vPosition = pPosition[i];
        e.m_vDirection = pVelocity[i];
        e.m_vNormal.SetZero();
        pQueue->AddEvent(e);
      }

      /// \todo Get current element index from iterator ?
      m_pStreamGroup->RemoveElement(i);
    }
  }
}

