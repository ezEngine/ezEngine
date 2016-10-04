#include <ParticlePlugin/PCH.h>
#include <ParticlePlugin/WorldModule/ParticleWorldModule.h>
#include <Foundation/Threading/Lock.h>
#include <Core/World/World.h>
#include <ParticlePlugin/Effect/ParticleEffectInstance.h>
#include <Core/ResourceManager/ResourceManager.h>
#include <Core/ResourceManager/ResourceBase.h>
#include <ParticlePlugin/Resources/ParticleEffectResource.h>
#include <RendererCore/Pipeline/ExtractedRenderData.h>
#include <Foundation/Threading/TaskSystem.h>

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezParticleWorldModule, 1, ezRTTIDefaultAllocator<ezParticleWorldModule>)
EZ_END_DYNAMIC_REFLECTED_TYPE

ezParticleWorldModule::ezParticleWorldModule()
{
  m_uiExtractedFrame = 0;
}

void ezParticleWorldModule::InternalStartup()
{
  ezResourceManager::s_ResourceEvents.AddEventHandler(ezMakeDelegate(&ezParticleWorldModule::ResourceEventHandler, this));


}

void ezParticleWorldModule::InternalBeforeWorldDestruction()
{

}

void ezParticleWorldModule::InternalAfterWorldDestruction()
{
  EZ_LOCK(m_Mutex);

  ezResourceManager::s_ResourceEvents.RemoveEventHandler(ezMakeDelegate(&ezParticleWorldModule::ResourceEventHandler, this));

  for (auto pEffect : m_ParticleEffects)
  {
    EZ_DEFAULT_DELETE(pEffect);
  }

  m_FinishingEffects.Clear();
  m_ParticleEffects.Clear();

  for (auto pSystem : m_ParticleSystemFreeList)
  {
    EZ_DEFAULT_DELETE(pSystem);
  }

  m_ParticleSystemFreeList.Clear();
}


void ezParticleWorldModule::InternalUpdate()
{
  if (!GetWorld()->GetWorldSimulationEnabled())
    return;

  EZ_LOCK(GetWorld()->GetWriteMarker());

  UpdateEffects();
}



ezParticleEffectHandle ezParticleWorldModule::InternalCreateSharedInstance(const char* szSharedName, const ezParticleEffectResourceHandle& hResource, ezUInt64 uiRandomSeed, const void* pSharedInstanceOwner)
{
  EZ_LOCK(m_Mutex);

  ezStringBuilder fullName;
  fullName.Format("{%s}-{%s}[%llu]", szSharedName, hResource.GetResourceID().GetData(), uiRandomSeed);

  bool bExisted = false;
  auto it = m_SharedEffects.FindOrAdd(fullName, &bExisted);
  ezParticleEffectInstance* pEffect = nullptr;

  if (bExisted)
  {
    TryGetEffect(it.Value(), pEffect);
  }

  if (!pEffect)
  {
    it.Value() = InternalCreateInstance(hResource, uiRandomSeed, true);
    TryGetEffect(it.Value(), pEffect);
  }

  EZ_ASSERT_DEBUG(pEffect != nullptr, "Invalid effect pointer");
  pEffect->AddSharedInstance(pSharedInstanceOwner);

  return it.Value();
}

ezParticleEffectHandle ezParticleWorldModule::InternalCreateInstance(const ezParticleEffectResourceHandle& hResource, ezUInt64 uiRandomSeed, bool bIsShared)
{
  EZ_LOCK(m_Mutex);

  ezParticleEffectInstance* pInstance = EZ_DEFAULT_NEW(ezParticleEffectInstance);
  m_ParticleEffects.PushBack(pInstance);
  pInstance->m_hHandle = m_ActiveEffects.Insert(pInstance);

  ezResourceLock<ezParticleEffectResource> pResource(hResource, ezResourceAcquireMode::NoFallback);

  pInstance->Configure(hResource, GetWorld(), this, uiRandomSeed, bIsShared);

  return pInstance->GetHandle();
}


ezParticleEffectHandle ezParticleWorldModule::CreateParticleEffectInstance(const ezParticleEffectResourceHandle& hResource, ezUInt64 uiRandomSeed, const char* szSharedName, const void* pSharedInstanceOwner)
{
  EZ_ASSERT_DEBUG(hResource.IsValid(), "Invalid Particle Effect resource handle");

  if (ezStringUtils::IsNullOrEmpty(szSharedName) || pSharedInstanceOwner == nullptr)
  {
    return InternalCreateInstance(hResource, uiRandomSeed, false);
  }
  else
  {
    return InternalCreateSharedInstance(szSharedName, hResource, uiRandomSeed, pSharedInstanceOwner);
  }
}

void ezParticleWorldModule::DestroyParticleEffectInstance(const ezParticleEffectHandle& hEffect, bool bInterruptImmediately, const void* pSharedInstanceOwner)
{
  EZ_LOCK(m_Mutex);

  ezParticleEffectInstance* pInstance = nullptr;
  if (TryGetEffect(hEffect, pInstance))
  {
    if (pSharedInstanceOwner != nullptr)
    {
      pInstance->RemoveSharedInstance(pSharedInstanceOwner);
      return; // never delete these
    }

    pInstance->SetEmitterEnabled(false);
    m_FinishingEffects.PushBack(pInstance);

    // as far as the outside world is concerned, the effect is dead now
    m_ActiveEffects.Remove(hEffect.GetInternalID());

    if (bInterruptImmediately)
    {
      pInstance->Interrupt();
    }
  }
}


bool ezParticleWorldModule::TryGetEffect(const ezParticleEffectHandle& hEffect, ezParticleEffectInstance*& out_pEffect)
{
  return m_ActiveEffects.TryGetValue(hEffect.GetInternalID(), out_pEffect);
}


void ezParticleWorldModule::EnsureParticleUpdateFinished()
{
  // do NOT lock here, otherwise tasks cannot enter the lock
  ezTaskSystem::WaitForGroup(m_EffectUpdateTaskGroup);
}

void ezParticleWorldModule::UpdateEffects()
{
  // do this outside the lock to allow tasks to enter it
  EnsureParticleUpdateFinished();

  EZ_LOCK(m_Mutex);

  m_EffectUpdateTaskGroup = ezTaskSystem::CreateTaskGroup(ezTaskPriority::EarlyNextFrame);

  DestroyFinishedEffects();

  ReconfigureEffects();

  const ezTime tDiff = GetWorld()->GetClock().GetTimeDiff();
  for (ezUInt32 i = 0; i < m_ParticleEffects.GetCount(); ++i)
  {
    ezParticleffectUpdateTask* pTask = m_ParticleEffects[i]->GetUpdateTask();
    pTask->m_UpdateDiff = tDiff;

    ezTaskSystem::AddTaskToGroup(m_EffectUpdateTaskGroup, pTask);
  }

  ezTaskSystem::StartTaskGroup(m_EffectUpdateTaskGroup);
}

void ezParticleWorldModule::ExtractRenderData(const ezView& view, ezExtractedRenderData* pExtractedRenderData)
{
  // do this outside the lock!
  EnsureParticleUpdateFinished();

  EZ_LOCK(m_Mutex);

  ++m_uiExtractedFrame;

  for (ezUInt32 e = 0; e < m_ParticleEffects.GetCount(); ++e)
  {
    ezParticleEffectInstance* pEffect = m_ParticleEffects[e];

    const auto& shared = pEffect->GetAllSharedInstances();

    for (ezUInt32 shi = 0; shi < ezMath::Max<ezUInt32>(1, shared.GetCount()); ++shi)
    {
      ezTransform systemTransform;

      if (!shared.IsEmpty())
        systemTransform = pEffect->GetTransform(shared[shi].m_pSharedInstanceOwner);
      else if (pEffect->IsSimulatedInLocalSpace())
        systemTransform = pEffect->GetTransform(nullptr);
      else
        systemTransform.SetIdentity();

      for (ezUInt32 i = 0; i < pEffect->GetParticleSystems().GetCount(); ++i)
      {
        const ezParticleSystemInstance* pSystem = pEffect->GetParticleSystems()[i];

        if (pSystem == nullptr)
          continue;

        if (!pSystem->HasActiveParticles() || !pSystem->IsVisible())
          continue;

        EZ_LOCK(pSystem->m_Mutex);

        pSystem->ExtractRenderData(view, pExtractedRenderData, systemTransform, m_uiExtractedFrame);
      }
    }
  }
}

void ezParticleWorldModule::DestroyFinishedEffects()
{
  EZ_LOCK(m_Mutex);

  for (ezUInt32 i = 0; i < m_FinishingEffects.GetCount(); )
  {
    ezParticleEffectInstance* pEffect = m_FinishingEffects[i];

    if (!pEffect->HasActiveParticles())
    {
      m_ParticleEffects.RemoveSwap(pEffect);
      m_FinishingEffects.RemoveAtSwap(i);

      EZ_DEFAULT_DELETE(pEffect);
    }
    else
    {
      ++i;
    }
  }
}


void ezParticleWorldModule::ResourceEventHandler(const ezResourceEvent& e)
{
  if (e.m_EventType == ezResourceEventType::ResourceContentUnloaded && e.m_pResource->GetDynamicRTTI()->IsDerivedFrom<ezParticleEffectResource>())
  {
    EZ_LOCK(m_Mutex);

    ezParticleEffectResourceHandle hResource((ezParticleEffectResource*)(e.m_pResource));

    const ezUInt32 numEffects = m_ParticleEffects.GetCount();
    for (ezUInt32 i = 0; i < numEffects; ++i)
    {
      if (m_ParticleEffects[i]->GetResource() == hResource)
      {
        m_EffectsToReconfigure.PushBack(m_ParticleEffects[i]);
      }
    }
  }
}


void ezParticleWorldModule::ReconfigureEffects()
{
  EZ_LOCK(m_Mutex);

  for (auto pEffect : m_EffectsToReconfigure)
  {
    pEffect->Reconfigure(0, false);
  }

  m_EffectsToReconfigure.Clear();
}

ezParticleSystemInstance* ezParticleWorldModule::CreateParticleSystemInstance(ezUInt32 uiMaxParticles, ezWorld* pWorld, ezUInt64 uiRandomSeed, ezParticleEffectInstance* pOwnerEffect)
{
  EZ_LOCK(m_Mutex);

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

void ezParticleWorldModule::DestroyParticleSystemInstance(ezParticleSystemInstance* pInstance)
{
  EZ_LOCK(m_Mutex);

  EZ_ASSERT_DEBUG(pInstance != nullptr, "Invalid particle system");
  m_ParticleSystemFreeList.PushBack(pInstance);
}






