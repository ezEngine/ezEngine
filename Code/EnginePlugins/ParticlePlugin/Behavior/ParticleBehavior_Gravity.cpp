#include <ParticlePlugin/ParticlePluginPCH.h>

#include <Core/Interfaces/PhysicsWorldModule.h>
#include <Core/World/World.h>
#include <Core/World/WorldModule.h>
#include <Foundation/DataProcessing/Stream/ProcessingStreamIterator.h>
#include <Foundation/Profiling/Profiling.h>
#include <Foundation/Time/Clock.h>
#include <ParticlePlugin/Behavior/ParticleBehavior_Gravity.h>
#include <ParticlePlugin/Finalizer/ParticleFinalizer_ApplyVelocity.h>
#include <ParticlePlugin/System/ParticleSystemInstance.h>
#include <ParticlePlugin/WorldModule/ParticleWorldModule.h>

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

  pBehavior->m_pPhysicsModule = (ezPhysicsWorldModuleInterface*)pBehavior->GetOwnerSystem()->GetOwnerWorldModule()->GetCachedWorldModule(ezGetStaticRTTI<ezPhysicsWorldModuleInterface>());
}

void ezParticleBehaviorFactory_Gravity::Save(ezStreamWriter& inout_stream) const
{
  const ezUInt8 uiVersion = 1;
  inout_stream << uiVersion;

  inout_stream << m_fGravityFactor;
}

void ezParticleBehaviorFactory_Gravity::Load(ezStreamReader& inout_stream, const ezParticleEffectDescriptor& ownerEffectDescriptor, const ezParticleSystemDescriptor& ownerSystemDescriptor)
{
  ezUInt8 uiVersion = 0;
  inout_stream >> uiVersion;

  inout_stream >> m_fGravityFactor;
}

void ezParticleBehaviorFactory_Gravity::QueryFinalizerDependencies(ezSet<const ezRTTI*>& inout_finalizerDeps) const
{
  inout_finalizerDeps.Insert(ezGetStaticRTTI<ezParticleFinalizerFactory_ApplyVelocity>());
}

//////////////////////////////////////////////////////////////////////////

void ezParticleBehavior_Gravity::CreateRequiredStreams()
{
  CreateStream("Velocity", ezProcessingStream::DataType::Half4, &m_pStreamVelocity, false);
}

void ezParticleBehavior_Gravity::Process(ezUInt64 uiNumElements)
{
  EZ_PROFILE_SCOPE("PFX: Gravity");

  const ezVec3 vGravity = m_pPhysicsModule != nullptr ? m_pPhysicsModule->GetGravity() : ezVec3(0.0f, 0.0f, -10.0f);

  const float tDiff = (float)m_TimeDiff.GetSeconds();
  const ezVec3 addGravity = vGravity * m_fGravityFactor * tDiff;

  ezProcessingStreamIterator<ezFloat16Vec4> itVelocity(m_pStreamVelocity, uiNumElements, 0);

  while (!itVelocity.HasReachedEnd())
  {
    const ezVec4 vel = itVelocity.Current();
    const ezVec3 dir(vel.x, vel.y, vel.z);
    const float speed = vel.w;

    const ezVec3 newVel = dir * speed + addGravity;
    const float newSpeed = newVel.GetLength();
    const ezVec3 newDir = newSpeed > 0.0f ? newVel / newSpeed : ezVec3(0, 0, 1);

    itVelocity.Current() = ezVec4(newDir.x, newDir.y, newDir.z, newSpeed);

    itVelocity.Advance();
  }
}

void ezParticleBehavior_Gravity::RequestRequiredWorldModulesForCache(ezParticleWorldModule* pParticleModule)
{
  pParticleModule->CacheWorldModule<ezPhysicsWorldModuleInterface>();
}


EZ_STATICLINK_FILE(ParticlePlugin, ParticlePlugin_Behavior_ParticleBehavior_Gravity);
