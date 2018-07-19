#include <PCH.h>
#include <ParticlePlugin/WorldModule/ParticleWorldModule.h>
#include <Core/World/World.h>
#include <ParticlePlugin/Resources/ParticleEffectResource.h>

ezParticleEffectHandle ezParticleWorldModule::InternalCreateEffectInstance(const ezParticleEffectResourceHandle& hResource, ezUInt64 uiRandomSeed, bool bIsShared)
{
  EZ_LOCK(m_Mutex);

  ezParticleEffectInstance* pInstance = nullptr;

  if (!m_ParticleEffectsFreeList.IsEmpty())
  {
    pInstance = m_ParticleEffectsFreeList.PeekBack();
    m_ParticleEffectsFreeList.PopBack();
  }
  else
  {
    pInstance = &m_ParticleEffects.ExpandAndGetRef();
  }

  ezParticleEffectHandle hEffectHandle(m_ActiveEffects.Insert(pInstance));
  pInstance->Construct(hEffectHandle, hResource, GetWorld(), this, uiRandomSeed, bIsShared);

  return hEffectHandle;
}

ezParticleEffectHandle ezParticleWorldModule::InternalCreateSharedEffectInstance(const char* szSharedName, const ezParticleEffectResourceHandle& hResource, ezUInt64 uiRandomSeed, const void* pSharedInstanceOwner)
{
  EZ_LOCK(m_Mutex);

  ezStringBuilder fullName;
  fullName.Format("{{0}}-{{1}}[{2}]", szSharedName, hResource.GetResourceID(), uiRandomSeed);

  bool bExisted = false;
  auto it = m_SharedEffects.FindOrAdd(fullName, &bExisted);
  ezParticleEffectInstance* pEffect = nullptr;

  if (bExisted)
  {
    TryGetEffectInstance(it.Value(), pEffect);
  }

  if (!pEffect)
  {
    it.Value() = InternalCreateEffectInstance(hResource, uiRandomSeed, true);
    TryGetEffectInstance(it.Value(), pEffect);
  }

  EZ_ASSERT_DEBUG(pEffect != nullptr, "Invalid effect pointer");
  pEffect->AddSharedInstance(pSharedInstanceOwner);

  return it.Value();
}


ezParticleEffectHandle ezParticleWorldModule::CreateEffectInstance(const ezParticleEffectResourceHandle& hResource, ezUInt64 uiRandomSeed, const char* szSharedName, const void*& inout_pSharedInstanceOwner)
{
  EZ_ASSERT_DEBUG(hResource.IsValid(), "Invalid Particle Effect resource handle");

  bool bIsShared = !ezStringUtils::IsNullOrEmpty(szSharedName) && (inout_pSharedInstanceOwner != nullptr);

  if (!bIsShared)
  {
    ezResourceLock<ezParticleEffectResource> pResource(hResource, ezResourceAcquireMode::NoFallback);
    bIsShared |= pResource->GetDescriptor().m_Effect.m_bAlwaysShared;
  }

  if (!bIsShared)
  {
    inout_pSharedInstanceOwner = nullptr;
    return InternalCreateEffectInstance(hResource, uiRandomSeed, false);
  }
  else
  {
    return InternalCreateSharedEffectInstance(szSharedName, hResource, uiRandomSeed, inout_pSharedInstanceOwner);
  }
}

void ezParticleWorldModule::DestroyEffectInstance(const ezParticleEffectHandle& hEffect, bool bInterruptImmediately, const void* pSharedInstanceOwner)
{
  EZ_LOCK(m_Mutex);

  ezParticleEffectInstance* pInstance = nullptr;
  if (TryGetEffectInstance(hEffect, pInstance))
  {
    if (pSharedInstanceOwner != nullptr)
    {
      pInstance->RemoveSharedInstance(pSharedInstanceOwner);
      return; // never delete these
    }

    if (!pInstance->m_bIsFinishing)
    {
      // make sure not to insert it into m_FinishingEffects twice
      pInstance->m_bIsFinishing = true;
      pInstance->SetEmitterEnabled(false);
      m_FinishingEffects.PushBack(pInstance);
    }

    if (bInterruptImmediately)
    {
      pInstance->Interrupt();
    }
  }
}

bool ezParticleWorldModule::TryGetEffectInstance(const ezParticleEffectHandle& hEffect, ezParticleEffectInstance*& out_pEffect)
{
  return m_ActiveEffects.TryGetValue(hEffect.GetInternalID(), out_pEffect);
}

void ezParticleWorldModule::UpdateEffects(const ezWorldModule::UpdateContext& context)
{
  // do this outside the lock to allow tasks to enter it
  // this should actually never have to have any effect, as this is already done at the end of the previous frame
  EnsureUpdatesFinished(context);

  EZ_LOCK(m_Mutex);

  DestroyFinishedEffects();
  ReconfigureEffects();

  m_EffectUpdateTaskGroup = ezTaskSystem::CreateTaskGroup(ezTaskPriority::EarlyNextFrame);

  const ezTime tDiff = GetWorld()->GetClock().GetTimeDiff();
  for (ezUInt32 i = 0; i < m_ParticleEffects.GetCount(); ++i)
  {
    if (!m_ParticleEffects[i].ShouldBeUpdated())
      continue;

    m_ParticleEffects[i].ProcessEventQueues();

    ezParticleffectUpdateTask* pTask = m_ParticleEffects[i].GetUpdateTask();
    pTask->m_UpdateDiff = tDiff;

    ezTaskSystem::AddTaskToGroup(m_EffectUpdateTaskGroup, pTask);
  }

  ezTaskSystem::StartTaskGroup(m_EffectUpdateTaskGroup);
}

void ezParticleWorldModule::DestroyFinishedEffects()
{
  EZ_LOCK(m_Mutex);

  for (ezUInt32 i = 0; i < m_FinishingEffects.GetCount(); )
  {
    ezParticleEffectInstance* pEffect = m_FinishingEffects[i];

    if (!pEffect->HasActiveParticles())
    {
      if (m_ActiveEffects.Remove(pEffect->GetHandle().GetInternalID()))
      {
        pEffect->Destruct();

        m_ParticleEffectsFreeList.PushBack(pEffect);
      }

      m_FinishingEffects.RemoveAtSwap(i);
    }
    else
    {
      ++i;
    }
  }
}

void ezParticleWorldModule::ReconfigureEffects()
{
  EZ_LOCK(m_Mutex);

  for (auto pEffect : m_EffectsToReconfigure)
  {
    pEffect->Reconfigure(false);
  }

  m_EffectsToReconfigure.Clear();
}





EZ_STATICLINK_FILE(ParticlePlugin, ParticlePlugin_WorldModule_ParticleEffects);

