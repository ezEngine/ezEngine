#include <ParticlePlugin/PCH.h>
#include <ParticlePlugin/Behavior/ParticleBehavior_Gravity.h>
#include <Core/World/WorldModule.h>
#include <GameUtils/Interfaces/PhysicsWorldModule.h>
#include <ParticlePlugin/System/ParticleSystemInstance.h>
#include <Core/World/World.h>
#include <Foundation/Time/Clock.h>
#include <CoreUtils/DataProcessing/Stream/ProcessingStreamIterator.h>

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezParticleBehaviorFactory_Gravity, 1, ezRTTIDefaultAllocator<ezParticleBehaviorFactory_Gravity>)
{
  EZ_BEGIN_PROPERTIES
  {
    EZ_MEMBER_PROPERTY("Gravity Factor", m_fGravityFactor)->AddAttributes(new ezDefaultValueAttribute(1.0f)),
  }
  EZ_END_PROPERTIES
}
EZ_END_DYNAMIC_REFLECTED_TYPE

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezParticleBehavior_Gravity, 1, ezRTTIDefaultAllocator<ezParticleBehavior_Gravity>)
EZ_END_DYNAMIC_REFLECTED_TYPE

ezParticleBehaviorFactory_Gravity::ezParticleBehaviorFactory_Gravity()
{
  m_fGravityFactor = 1.0f;
}

const ezRTTI* ezParticleBehaviorFactory_Gravity::GetBehaviorType() const
{
  return ezGetStaticRTTI<ezParticleBehavior_Gravity>();
}

void ezParticleBehaviorFactory_Gravity::CopyBehaviorProperties(ezParticleBehavior* pObject) const
{
  ezParticleBehavior_Gravity* pBehavior = static_cast<ezParticleBehavior_Gravity*>(pObject);

  pBehavior->m_fGravityFactor = m_fGravityFactor;
}

void ezParticleBehaviorFactory_Gravity::Save(ezStreamWriter& stream) const
{
  const ezUInt8 uiVersion = 1;
  stream << uiVersion;

  stream << m_fGravityFactor;
}

void ezParticleBehaviorFactory_Gravity::Load(ezStreamReader& stream)
{
  ezUInt8 uiVersion = 0;
  stream >> uiVersion;

  stream >> m_fGravityFactor;
}

void ezParticleBehavior_Gravity::AfterPropertiesConfigured(bool bFirstTime)
{
  m_pPhysicsModule = static_cast<ezPhysicsWorldModuleInterface*>(ezWorldModule::FindModule(GetOwnerSystem()->GetWorld(), ezPhysicsWorldModuleInterface::GetStaticRTTI()));

}

void ezParticleBehavior_Gravity::CreateRequiredStreams()
{
  CreateStream("Velocity", ezProcessingStream::DataType::Float3, &m_pStreamVelocity, false);
}

void ezParticleBehavior_Gravity::Process(ezUInt64 uiNumElements)
{
  const ezVec3 vGravity = m_pPhysicsModule->GetGravity();

  const float tDiff = (float)m_TimeDiff.GetSeconds();
  const ezVec3 addGravity = vGravity * m_fGravityFactor * tDiff;

  ezProcessingStreamIterator<ezVec3> itVelocity(m_pStreamVelocity, uiNumElements);

  while (!itVelocity.HasReachedEnd())
  {
    itVelocity.Current() += addGravity;

    itVelocity.Advance();
  }
}
