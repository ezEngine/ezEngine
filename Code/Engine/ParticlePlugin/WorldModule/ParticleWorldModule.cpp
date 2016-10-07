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

  m_ParticleSystemFreeList.Clear();
  m_FinishingEffects.Clear();

  m_ParticleEffects.Clear();
  m_ParticleSystems.Clear();
}


void ezParticleWorldModule::InternalUpdateBefore()
{
  if (!GetWorld()->GetWorldSimulationEnabled())
    return;

  EZ_LOCK(GetWorld()->GetWriteMarker());

  UpdateEffects();
}

void ezParticleWorldModule::InternalUpdateAfter()
{
  EnsureUpdatesFinished();
}

void ezParticleWorldModule::EnsureUpdatesFinished()
{
  // do NOT lock here, otherwise tasks cannot enter the lock
  ezTaskSystem::WaitForGroup(m_EffectUpdateTaskGroup);
}

void ezParticleWorldModule::ExtractRenderData(const ezView& view, ezExtractedRenderData* pExtractedRenderData)
{
  EZ_ASSERT_RELEASE(ezTaskSystem::IsTaskGroupFinished(m_EffectUpdateTaskGroup), "Particle Effect Update Task is not finished!");

  EZ_LOCK(m_Mutex);

  ++m_uiExtractedFrame;

  for (ezUInt32 e = 0; e < m_ParticleEffects.GetCount(); ++e)
  {
    ezParticleEffectInstance* pEffect = &m_ParticleEffects[e];

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

        pSystem->ExtractRenderData(view, pExtractedRenderData, systemTransform, m_uiExtractedFrame);
      }
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
      if (m_ParticleEffects[i].GetResource() == hResource)
      {
        m_EffectsToReconfigure.PushBack(&m_ParticleEffects[i]);
      }
    }
  }
}








