#include <ParticlePlugin/ParticlePluginPCH.h>

#include <Core/World/World.h>
#include <ParticlePlugin/Resources/ParticleEffectResource.h>
#include <ParticlePlugin/WorldModule/ParticleWorldModule.h>
#include <RendererCore/RenderWorld/RenderWorld.h>

ezParticleEffectHandle ezParticleWorldModule::InternalCreateEffectInstance(const ezParticleEffectResourceHandle& hResource, ezUInt64 uiRandomSeed, bool bIsShared, ezArrayPtr<ezParticleEffectFloatParam> floatParams, ezArrayPtr<ezParticleEffectColorParam> colorParams)
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
  pInstance->Construct(hEffectHandle, hResource, GetWorld(), this, uiRandomSeed, bIsShared, floatParams, colorParams);

  return hEffectHandle;
}

ezParticleEffectHandle ezParticleWorldModule::InternalCreateSharedEffectInstance(
  const char* szSharedName, const ezParticleEffectResourceHandle& hResource, ezUInt64 uiRandomSeed, const void* pSharedInstanceOwner)
{
  EZ_LOCK(m_Mutex);

  ezStringBuilder fullName;
  fullName.SetFormat("{{0}}-{{1}}[{2}]", szSharedName, hResource.GetResourceID(), uiRandomSeed);

  bool bExisted = false;
  auto it = m_SharedEffects.FindOrAdd(fullName, &bExisted);
  ezParticleEffectInstance* pEffect = nullptr;

  if (bExisted)
  {
    TryGetEffectInstance(it.Value(), pEffect);
  }

  if (!pEffect)
  {
    it.Value() =
      InternalCreateEffectInstance(hResource, uiRandomSeed, true, ezArrayPtr<ezParticleEffectFloatParam>(), ezArrayPtr<ezParticleEffectColorParam>());
    TryGetEffectInstance(it.Value(), pEffect);
  }

  EZ_ASSERT_DEBUG(pEffect != nullptr, "Invalid effect pointer");
  pEffect->AddSharedInstance(pSharedInstanceOwner);

  return it.Value();
}


ezParticleEffectHandle ezParticleWorldModule::CreateEffectInstance(const ezParticleEffectResourceHandle& hResource, ezUInt64 uiRandomSeed, const char* szSharedName, const void*& inout_pSharedInstanceOwner, ezArrayPtr<ezParticleEffectFloatParam> floatParams, ezArrayPtr<ezParticleEffectColorParam> colorParams)
{
  EZ_ASSERT_DEBUG(hResource.IsValid(), "Invalid Particle Effect resource handle");

  bool bIsShared = !ezStringUtils::IsNullOrEmpty(szSharedName) && (inout_pSharedInstanceOwner != nullptr);

  if (!bIsShared)
  {
    ezResourceLock<ezParticleEffectResource> pResource(hResource, ezResourceAcquireMode::BlockTillLoaded);
    bIsShared |= pResource->GetDescriptor()->m_Effect.m_bAlwaysShared;
  }

  if (!bIsShared)
  {
    inout_pSharedInstanceOwner = nullptr;
    return InternalCreateEffectInstance(hResource, uiRandomSeed, false, floatParams, colorParams);
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

      if (!bInterruptImmediately)
      {
        m_NeedFinisherComponent.PushBack(pInstance);
      }
    }

    if (bInterruptImmediately)
    {
      pInstance->Interrupt();
    }
  }
}

bool ezParticleWorldModule::TryGetEffectInstance(const ezParticleEffectHandle& hEffect, ezParticleEffectInstance*& out_pEffect)
{
  // called during Async (owner messages) and from the effect update tasks, while other threads may create effects
  EZ_LOCK(m_Mutex);
  return m_ActiveEffects.TryGetValue(hEffect.GetInternalID(), out_pEffect);
}

bool ezParticleWorldModule::TryGetEffectInstance(const ezParticleEffectHandle& hEffect, const ezParticleEffectInstance*& out_pEffect) const
{
  EZ_LOCK(m_Mutex);
  ezParticleEffectInstance* pEffect = nullptr;
  bool bResult = m_ActiveEffects.TryGetValue(hEffect.GetInternalID(), pEffect);
  out_pEffect = pEffect;
  return bResult;
}

void ezParticleWorldModule::QueueEffectReconfigure(ezParticleEffectInstance* pEffect)
{
  EZ_LOCK(m_Mutex);

  if (!m_EffectsToReconfigure.Contains(pEffect))
  {
    m_EffectsToReconfigure.PushBack(pEffect);
  }
}

ezSharedPtr<ezRefCounted> ezParticleWorldModule::GetOrCreateSharedData(ezStringView sKey, ezDelegate<ezSharedPtr<ezRefCounted>()> createFunc)
{
  EZ_LOCK(m_SharedDataMutex);

  bool bExisted = false;
  SharedData& data = m_SharedData.FindOrAdd(sKey, &bExisted);

  if (!bExisted)
  {
    data.m_pData = createFunc();
  }

  data.m_LastUsed = ezTime::Now();
  return data.m_pData;
}

void ezParticleWorldModule::ReleaseUnusedSharedData()
{
  EZ_LOCK(m_SharedDataMutex);

  const ezTime tNow = ezTime::Now();

  for (auto it = m_SharedData.GetIterator(); it.IsValid();)
  {
    SharedData& data = it.Value();

    if (data.m_pData != nullptr && data.m_pData->GetRefCount() > 1)
    {
      data.m_LastUsed = tNow;
    }
    else if (tNow - data.m_LastUsed > ezTime::MakeFromSeconds(2))
    {
      it = m_SharedData.Remove(it);
      continue;
    }

    ++it;
  }
}

void ezParticleWorldModule::UpdateEffects(const ezWorldModule::UpdateContext& context)
{
  EZ_LOCK(m_Mutex);

  DestroyFinishedEffects();
  ReconfigureEffects();
  ReleaseUnusedSharedData();

  // effects that were created or reconfigured since the last PostTransform phase haven't seen the world yet
  // this is the main thread and no update tasks are running, so the modules can read the world here as well
  for (ezUInt32 i = 0; i < m_ParticleEffects.GetCount(); ++i)
  {
    if (m_ParticleEffects[i].m_bModulesNeedWorldUpdate && !m_ParticleEffects[i].GetHandle().IsInvalidated())
    {
      m_ParticleEffects[i].UpdateModulesFromWorld();
    }
  }

  m_EffectUpdateTaskGroup = ezTaskSystem::CreateTaskGroup(ezTaskPriority::LateThisFrame);

  const ezTime tDiff = GetWorld()->GetClock().GetTimeDiff();
  for (ezUInt32 i = 0; i < m_ParticleEffects.GetCount(); ++i)
  {
    if (!m_ParticleEffects[i].ShouldBeUpdated())
      continue;

    m_ParticleEffects[i].ProcessEventQueues();

    const ezSharedPtr<ezTask>& pTask = m_ParticleEffects[i].GetUpdateTask();
    static_cast<ezParticleEffectUpdateTask*>(pTask.Borrow())->m_UpdateDiff = tDiff;

    ezTaskSystem::AddTaskToGroup(m_EffectUpdateTaskGroup, pTask);
  }

  ezTaskSystem::StartTaskGroup(m_EffectUpdateTaskGroup);
}

void ezParticleWorldModule::DestroyFinishedEffects()
{
  EZ_LOCK(m_Mutex);

  for (ezUInt32 i = 0; i < m_FinishingEffects.GetCount();)
  {
    ezParticleEffectInstance* pEffect = m_FinishingEffects[i];

    if (!pEffect->HasActiveParticles())
    {
      if (m_ActiveEffects.Remove(pEffect->GetHandle().GetInternalID()))
      {
        pEffect->Destruct();

        m_ParticleEffectsFreeList.PushBack(pEffect);
      }

      m_FinishingEffects.RemoveAtAndSwap(i);
    }
    else
    {
      ++i;
    }
  }

  for (ezUInt32 i = 0; i < m_NeedFinisherComponent.GetCount(); ++i)
  {
    ezParticleEffectInstance* pEffect = m_NeedFinisherComponent[i];

    CreateFinisherComponent(pEffect);
  }

  m_NeedFinisherComponent.Clear();
}

void ezParticleWorldModule::ReconfigureEffects()
{
  EZ_LOCK(m_Mutex);

  for (auto pEffect : m_EffectsToReconfigure)
  {
    // may have been destroyed in the meantime
    if (pEffect->GetHandle().IsInvalidated())
      continue;

    pEffect->Reconfigure(false, ezArrayPtr<ezParticleEffectFloatParam>(), ezArrayPtr<ezParticleEffectColorParam>());
  }

  m_EffectsToReconfigure.Clear();
}
