#include <ParticlePlugin/PCH.h>
#include <ParticlePlugin/Manager/ParticleEffectManager.h>
#include <ParticlePlugin/System/ParticleSystemInstance.h>
#include <ParticlePlugin/Effect/ParticleEffectInstance.h>
#include <ParticlePlugin/Resources/ParticleEffectResource.h>
#include <Core/ResourceManager/ResourceBase.h>

EZ_IMPLEMENT_SINGLETON(ezParticleEffectManager);


ezParticleEffectManager::ezParticleEffectManager()
  : m_SingletonRegistrar(this)
{

}

void ezParticleEffectManager::Shutdown()
{
  for (auto pSystem : m_ParticleSystemFreeList)
  {
    EZ_DEFAULT_DELETE(pSystem);
  }

  m_ParticleSystemFreeList.Clear();
}

ezParticleSystemInstance* ezParticleEffectManager::CreateParticleSystemInstance(ezUInt32 uiMaxParticles, ezWorld* pWorld, ezUInt64 uiRandomSeed, ezParticleEffectInstance* pOwnerEffect)
{
  // TODO: Mutex

  ezParticleSystemInstance* pResult = nullptr;

  for (ezUInt32 i = 0; i < m_ParticleSystemFreeList.GetCount(); ++i)
  {
    if (m_ParticleSystemFreeList[i]->GetRefCount() == 0)
    {
      pResult = m_ParticleSystemFreeList[i];
      m_ParticleSystemFreeList.RemoveAtSwap(i);

      break;
    }
  }

  if (pResult == nullptr)
  {
    pResult = EZ_DEFAULT_NEW(ezParticleSystemInstance);
  }

  pResult->Initialize(uiMaxParticles, pWorld, uiRandomSeed, pOwnerEffect);

  return pResult;
}

void ezParticleEffectManager::DestroyParticleSystemInstance(ezParticleSystemInstance* pInstance)
{
  // TODO: Mutex
  EZ_ASSERT_DEBUG(pInstance != nullptr, "Invalid particle system");
  m_ParticleSystemFreeList.PushBack(pInstance);
}
