#include <ParticlePlugin/PCH.h>
#include <ParticlePlugin/WorldModule/ParticleWorldModule.h>
#include <ParticlePlugin/Manager/ParticleEffectManager.h>
#include <Foundation/Threading/Lock.h>
#include <Core/World/World.h>
#include <ParticlePlugin/Effect/ParticleEffectInstance.h>
#include <Core/ResourceManager/ResourceManager.h>
#include <Core/ResourceManager/ResourceBase.h>
#include <ParticlePlugin/Resources/ParticleEffectResource.h>
#include <ParticlePlugin/Renderer/ParticleRenderData.h>
#include <RendererCore/Pipeline/ExtractedRenderData.h>

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezParticleWorldModule, 1, ezRTTIDefaultAllocator<ezParticleWorldModule>)
EZ_END_DYNAMIC_REFLECTED_TYPE

void ezParticleWorldModule::InternalStartup()
{
  ezResourceManager::s_ResourceEvents.AddEventHandler(ezMakeDelegate(&ezParticleWorldModule::ResourceEventHandler, this));


}

void ezParticleWorldModule::InternalBeforeWorldDestruction()
{

}

void ezParticleWorldModule::InternalAfterWorldDestruction()
{
  ezResourceManager::s_ResourceEvents.RemoveEventHandler(ezMakeDelegate(&ezParticleWorldModule::ResourceEventHandler, this));

  for (auto pEffect : m_ParticleEffects)
  {
    EZ_DEFAULT_DELETE(pEffect);
  }

  m_FinishingEffects.Clear();
  m_ParticleEffects.Clear();
}

void ezParticleWorldModule::InternalUpdate()
{
  if (!GetWorld()->GetWorldSimulationEnabled())
    return;

  EZ_LOCK(GetWorld()->GetWriteMarker());

  UpdateEffects();
}



ezParticleEffectHandle ezParticleWorldModule::InternalCreateSharedInstance(const char* szSharedName, const ezParticleEffectResourceHandle& hResource, ezUInt64 uiRandomSeed, ezUInt32 uiInstanceIdentifier)
{
  /// \todo Mutex

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
  pEffect->AddSharedInstance(uiInstanceIdentifier);

  return it.Value();
}

ezParticleEffectHandle ezParticleWorldModule::InternalCreateInstance(const ezParticleEffectResourceHandle& hResource, ezUInt64 uiRandomSeed, bool bIsShared)
{
  ezParticleEffectInstance* pInstance = EZ_DEFAULT_NEW(ezParticleEffectInstance);
  m_ParticleEffects.PushBack(pInstance);
  pInstance->m_hHandle = m_ActiveEffects.Insert(pInstance);

  ezResourceLock<ezParticleEffectResource> pResource(hResource, ezResourceAcquireMode::NoFallback);

  pInstance->Configure(hResource, GetWorld(), this, uiRandomSeed, bIsShared);

  return pInstance->GetHandle();
}


ezParticleEffectHandle ezParticleWorldModule::CreateParticleEffectInstance(const ezParticleEffectResourceHandle& hResource, ezUInt64 uiRandomSeed, const char* szSharedName, ezUInt32 uiInstanceIdentifier)
{
  EZ_ASSERT_DEBUG(hResource.IsValid(), "Invalid Particle Effect resource handle");

  if (ezStringUtils::IsNullOrEmpty(szSharedName) || uiInstanceIdentifier == 0xFFFFFFFF)
  {
    return InternalCreateInstance(hResource, uiRandomSeed, false);
  }
  else
  {
    return InternalCreateSharedInstance(szSharedName, hResource, uiRandomSeed, uiInstanceIdentifier);
  }
}

void ezParticleWorldModule::DestroyParticleEffectInstance(const ezParticleEffectHandle& hEffect, bool bInterruptImmediately, ezUInt32 uiInstanceIdentifier)
{
  ezParticleEffectInstance* pInstance = nullptr;
  if (TryGetEffect(hEffect, pInstance))
  {
    if (uiInstanceIdentifier != 0xFFFFFFFF)
    {
      pInstance->RemoveSharedInstance(uiInstanceIdentifier);
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

void ezParticleWorldModule::UpdateEffects()
{
  ReconfigureEffects();

  for (ezUInt32 i = 0; i < m_ParticleEffects.GetCount(); ++i)
  {
    m_ParticleEffects[i]->PreSimulate();

    if (!m_ParticleEffects[i]->Update(GetWorld()->GetClock().GetTimeDiff()))
    {
      const ezParticleEffectHandle hEffect = m_ParticleEffects[i]->GetHandle();
      EZ_ASSERT_DEBUG(!hEffect.IsInvalidated(), "Invalid particle effect handle");

      DestroyParticleEffectInstance(hEffect, false, 0xFFFFFFFF);
    }
  }

  DestroyFinishedEffects();
}

static ezParticleRenderData* CreateParticleRenderData(ezUInt32 uiBatchId)
{
  return ezCreateRenderDataForThisFrame<ezParticleRenderData>(nullptr, uiBatchId);
}

void ezParticleWorldModule::ExtractRenderData(const ezView& view, ezExtractedRenderData* pExtractedRenderData)
{
  for (ezUInt32 e = 0; e < m_ParticleEffects.GetCount(); ++e)
  {
    ezParticleEffectInstance* pEffect = m_ParticleEffects[e];

    const auto& shared = pEffect->GetAllSharedInstances();

    for (ezUInt32 shi = 0; shi < ezMath::Max<ezUInt32>(1, shared.GetCount()); ++shi)
    {
      ezTransform systemTransform;

      if (!shared.IsEmpty())
        systemTransform = pEffect->GetTransform(shared[shi].m_uiIdentifier);
      else if (pEffect->IsSimulatedInLocalSpace())
        systemTransform = pEffect->GetTransform(0xFFFFFFFF);
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

        // Generate batch id from mesh, material and part index. 
        ezUInt32 data[] = { 42 };
        ezUInt32 uiBatchId = ezHashing::MurmurHash(data, sizeof(data));

        ezParticleRenderData* pRenderData = CreateParticleRenderData(uiBatchId);
        {
          pRenderData->m_GlobalTransform = systemTransform;
          //pRenderData->m_GlobalBounds = GetOwner()->GetGlobalBounds(); // TODO
          pRenderData->m_pParticleSystem = pSystem;
        }

        // make sure it stays alive until the renderer is done with it
        pRenderData->m_pParticleSystem->IncreaseRefCount();

        // Determine render data category.
        ezRenderData::Category category;
        category = ezDefaultRenderDataCategories::SimpleTransparent;

        ezUInt32 uiSortingKey = 0;
        pExtractedRenderData->AddRenderData(pRenderData, category, uiSortingKey);

        pSystem->ExtractRenderData(view, pExtractedRenderData);
      }
    }
  }
}

void ezParticleWorldModule::DestroyFinishedEffects()
{
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
  // TODO Mutex

  if (e.m_EventType == ezResourceEventType::ResourceContentUnloaded && e.m_pResource->GetDynamicRTTI()->IsDerivedFrom<ezParticleEffectResource>())
  {
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
  for (auto pEffect : m_EffectsToReconfigure)
  {
    pEffect->Reconfigure(0, false);
  }

  m_EffectsToReconfigure.Clear();
}
