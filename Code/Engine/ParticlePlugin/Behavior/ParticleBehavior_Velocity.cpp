#include <ParticlePlugin/PCH.h>
#include <ParticlePlugin/Behavior/ParticleBehavior_Velocity.h>
#include <Core/World/WorldModule.h>
#include <GameUtils/Interfaces/PhysicsWorldModule.h>
#include <ParticlePlugin/System/ParticleSystemInstance.h>
#include <Core/World/World.h>
#include <Foundation/Time/Clock.h>
#include <CoreUtils/DataProcessing/Stream/ProcessingStreamIterator.h>

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezParticleBehaviorFactory_Velocity, 1, ezRTTIDefaultAllocator<ezParticleBehaviorFactory_Velocity>)
{
  EZ_BEGIN_PROPERTIES
  {
    EZ_MEMBER_PROPERTY("Rise Speed", m_fRiseSpeed),
    EZ_MEMBER_PROPERTY("Acceleration", m_fAcceleration),
  }
  EZ_END_PROPERTIES
}
EZ_END_DYNAMIC_REFLECTED_TYPE

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezParticleBehavior_Velocity, 1, ezRTTIDefaultAllocator<ezParticleBehavior_Velocity>)
EZ_END_DYNAMIC_REFLECTED_TYPE

ezParticleBehaviorFactory_Velocity::ezParticleBehaviorFactory_Velocity()
{
  m_fRiseSpeed = 0;
  m_fAcceleration = 0;
}

const ezRTTI* ezParticleBehaviorFactory_Velocity::GetBehaviorType() const
{
  return ezGetStaticRTTI<ezParticleBehavior_Velocity>();
}

void ezParticleBehaviorFactory_Velocity::CopyBehaviorProperties(ezParticleBehavior* pObject) const
{
  ezParticleBehavior_Velocity* pBehavior = static_cast<ezParticleBehavior_Velocity*>(pObject);

  pBehavior->m_fRiseSpeed = m_fRiseSpeed;
  pBehavior->m_fAcceleration = m_fAcceleration;
}

enum class BehaviorVelocityVersion
{
  Version_0 = 0,
  Version_1,
  Version_2, // added rise speed and acceleration

  // insert new version numbers above
  Version_Count,
  Version_Current = Version_Count - 1
};

void ezParticleBehaviorFactory_Velocity::Save(ezStreamWriter& stream) const
{
  const ezUInt8 uiVersion = (int)BehaviorVelocityVersion::Version_Current;
  stream << uiVersion;

  stream << m_fRiseSpeed;
  stream << m_fAcceleration;
}

void ezParticleBehaviorFactory_Velocity::Load(ezStreamReader& stream)
{
  ezUInt8 uiVersion = 0;
  stream >> uiVersion;

  EZ_ASSERT_DEV(uiVersion <= (int)BehaviorVelocityVersion::Version_Current, "Invalid version %u", uiVersion);

  stream >> m_fRiseSpeed;
  stream >> m_fAcceleration;
}

void ezParticleBehavior_Velocity::AfterPropertiesConfigured(bool bFirstTime)
{
  m_pPhysicsModule = static_cast<ezPhysicsWorldModuleInterface*>(ezWorldModule::FindModule(GetOwnerSystem()->GetWorld(), ezPhysicsWorldModuleInterface::GetStaticRTTI()));
}

void ezParticleBehavior_Velocity::CreateRequiredStreams()
{
  CreateStream("Position", ezProcessingStream::DataType::Float3, &m_pStreamPosition);
  CreateStream("Velocity", ezProcessingStream::DataType::Float3, &m_pStreamVelocity);
}

void ezParticleBehavior_Velocity::Process(ezUInt64 uiNumElements)
{
  const float tDiff = (float)m_TimeDiff.GetSeconds();
  const ezVec3 vRise = m_pPhysicsModule->GetGravity().GetNormalized() * tDiff * -m_fRiseSpeed;

  const float fVelocityFactor = 1.0f + (m_fAcceleration * tDiff);

  ezProcessingStreamIterator<ezVec3> itPosition(m_pStreamPosition, uiNumElements);
  ezProcessingStreamIterator<ezVec3> itVelocity(m_pStreamVelocity, uiNumElements);

  while (!itPosition.HasReachedEnd())
  {
    itPosition.Current() += itVelocity.Current() * tDiff + vRise;
    itVelocity.Current() *= fVelocityFactor;

    itPosition.Advance();
    itVelocity.Advance();
  }
}


