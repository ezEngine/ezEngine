#include <ParticlePluginPCH.h>

#include <ParticlePlugin/WorldModule/ParticleWorldModule.h>

ezParticleSystemInstance* ezParticleWorldModule::CreateSystemInstance(ezUInt32 uiMaxParticles, ezWorld* pWorld,
                                                                      ezParticleEffectInstance* pOwnerEffect, float fSpawnMultiplier)
{
  EZ_LOCK(m_Mutex);

  ezParticleSystemInstance* pResult = nullptr;

  if (!m_ParticleSystemFreeList.IsEmpty())
  {
    pResult = m_ParticleSystemFreeList.PeekBack();
    m_ParticleSystemFreeList.PopBack();
  }

  if (pResult == nullptr)
  {
    pResult = &m_ParticleSystems.ExpandAndGetRef();
  }

  pResult->Construct(uiMaxParticles, pWorld, pOwnerEffect, fSpawnMultiplier);

  return pResult;
}

void ezParticleWorldModule::DestroySystemInstance(ezParticleSystemInstance* pInstance)
{
  EZ_LOCK(m_Mutex);

  EZ_ASSERT_DEBUG(pInstance != nullptr, "Invalid particle system");
  pInstance->Destruct();
  m_ParticleSystemFreeList.PushBack(pInstance);
}


EZ_STATICLINK_FILE(ParticlePlugin, ParticlePlugin_WorldModule_ParticleSystems);
