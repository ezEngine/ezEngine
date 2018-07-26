#include <PCH.h>

#include <Core/World/World.h>
#include <Core/World/WorldModule.h>
#include <Foundation/DataProcessing/Stream/ProcessingStreamIterator.h>
#include <Foundation/Profiling/Profiling.h>
#include <Foundation/Time/Clock.h>
#include <GameEngine/Interfaces/PhysicsWorldModule.h>
#include <ParticlePlugin/Behavior/ParticleBehavior_Gravity.h>
#include <ParticlePlugin/Finalizer/ParticleFinalizer_ApplyVelocity.h>
#include <ParticlePlugin/System/ParticleSystemInstance.h>

// clang-format off
EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezParticleBehaviorFactory_Gravity, 1, ezRTTIDefaultAllocator<ezParticleBehaviorFactory_Gravity>)
{
  EZ_BEGIN_PROPERTIES
  {
    EZ_MEMBER_PROPERTY("GravityFactor", m_fGravityFactor)->AddAttributes(new ezDefaultValueAttribute(1.0f)),
  }
  EZ_END_PROPERTIES;
}
EZ_END_DYNAMIC_REFLECTED_TYPE;

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezParticleBehavior_Gravity, 1, ezRTTIDefaultAllocator<ezParticleBehavior_Gravity>)
EZ_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

ezParticleBehaviorFactory_Gravity::ezParticleBehaviorFactory_Gravity()
{
  m_fGravityFactor = 1.0f;
}

const ezRTTI* ezParticleBehaviorFactory_Gravity::GetBehaviorType() const
{
  return ezGetStaticRTTI<ezParticleBehavior_Gravity>();
}

void ezParticleBehaviorFactory_Gravity::CopyBehaviorProperties(ezParticleBehavior* pObject, bool bFirstTime) const
{
  ezParticleBehavior_Gravity* pBehavior = static_cast<ezParticleBehavior_Gravity*>(pObject);

  pBehavior->m_fGravityFactor = m_fGravityFactor;

  pBehavior->m_pPhysicsModule = pBehavior->GetOwnerSystem()->GetWorld()->GetOrCreateModule<ezPhysicsWorldModuleInterface>();
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

void ezParticleBehaviorFactory_Gravity::QueryFinalizerDependencies(ezSet<const ezRTTI*>& inout_FinalizerDeps) const
{
  inout_FinalizerDeps.Insert(ezGetStaticRTTI<ezParticleFinalizerFactory_ApplyVelocity>());
}

//////////////////////////////////////////////////////////////////////////

void ezParticleBehavior_Gravity::CreateRequiredStreams()
{
  CreateStream("Velocity", ezProcessingStream::DataType::Float3, &m_pStreamVelocity, false);
}

void ezParticleBehavior_Gravity::Process(ezUInt64 uiNumElements)
{
  EZ_PROFILE("PFX: Gravity");

  const ezVec3 vGravity = m_pPhysicsModule != nullptr ? m_pPhysicsModule->GetGravity() : ezVec3(0.0f, 0.0f, -10.0f);

  const float tDiff = (float)m_TimeDiff.GetSeconds();
  const ezVec3 addGravity = vGravity * m_fGravityFactor * tDiff;

  ezProcessingStreamIterator<ezVec3> itVelocity(m_pStreamVelocity, uiNumElements, 0);

  while (!itVelocity.HasReachedEnd())
  {
    itVelocity.Current() += addGravity;

    itVelocity.Advance();
  }
}



EZ_STATICLINK_FILE(ParticlePlugin, ParticlePlugin_Behavior_ParticleBehavior_Gravity);
