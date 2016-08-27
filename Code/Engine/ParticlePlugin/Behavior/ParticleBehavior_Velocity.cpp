#include <ParticlePlugin/PCH.h>
#include <ParticlePlugin/Behavior/ParticleBehavior_Velocity.h>
#include <Core/World/WorldModule.h>
#include <GameUtils/Interfaces/PhysicsWorldModule.h>
#include <ParticlePlugin/System/ParticleSystemInstance.h>
#include <Core/World/World.h>
#include <Foundation/Time/Clock.h>
#include <CoreUtils/DataProcessing/Stream/StreamElementIterator.h>

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezParticleBehaviorFactory_Velocity, 1, ezRTTIDefaultAllocator<ezParticleBehaviorFactory_Velocity>)
//{
//  EZ_BEGIN_PROPERTIES
//  {
//    EZ_MEMBER_PROPERTY("Damping", m_fVelocityDamping)
//  }
//  EZ_END_PROPERTIES
//}
EZ_END_DYNAMIC_REFLECTED_TYPE

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezParticleBehavior_Velocity, 1, ezRTTIDefaultAllocator<ezParticleBehavior_Velocity>)
EZ_END_DYNAMIC_REFLECTED_TYPE

const ezRTTI* ezParticleBehaviorFactory_Velocity::GetBehaviorType() const
{
  return ezGetStaticRTTI<ezParticleBehavior_Velocity>();
}

void ezParticleBehaviorFactory_Velocity::CopyBehaviorProperties(ezParticleBehavior* pObject) const
{
  ezParticleBehavior_Velocity* pBehavior = static_cast<ezParticleBehavior_Velocity*>(pObject);


}

void ezParticleBehaviorFactory_Velocity::Save(ezStreamWriter& stream) const
{
  ezUInt8 uiVersion = 1;
  stream << uiVersion;

  //stream << m_fVelocityDamping;
}

void ezParticleBehaviorFactory_Velocity::Load(ezStreamReader& stream)
{
  ezUInt8 uiVersion = 0;
  stream >> uiVersion;

  //stream >> m_fVelocityDamping;
}

void ezParticleBehavior_Velocity::CreateRequiredStreams()
{
  CreateStream("Position", ezStream::DataType::Float3, &m_pStreamPosition);
  CreateStream("Velocity", ezStream::DataType::Float3, &m_pStreamVelocity);
}

void ezParticleBehavior_Velocity::Process(ezUInt64 uiNumElements)
{
  const float tDiff = (float)m_TimeDiff.GetSeconds();
  
  ezStreamElementIterator<ezVec3> itPosition(m_pStreamPosition, uiNumElements);
  ezStreamElementIterator<ezVec3> itVelocity(m_pStreamVelocity, uiNumElements);

  while (!itPosition.HasReachedEnd())
  {
    itPosition.Current() += itVelocity.Current() * tDiff;

    itPosition.Advance();
    itVelocity.Advance();
  }
}

