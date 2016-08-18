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



ezParticleEffectInstance* ezParticleWorldModule::CreateParticleEffectInstance(const ezParticleEffectResourceHandle& hResource)
{
  ezParticleEffectInstance* pInstance = EZ_DEFAULT_NEW(ezParticleEffectInstance);
  m_ParticleEffects.PushBack(pInstance);

  ezResourceLock<ezParticleEffectResource> pResource(hResource, ezResourceAcquireMode::NoFallback);

  pInstance->Configure(hResource, GetWorld());

  return pInstance;
}


void ezParticleWorldModule::DestroyParticleEffectInstance(ezParticleEffectInstance* pInstance)
{
  pInstance->SetEmitterEnabled(false);
  m_FinishingEffects.PushBack(pInstance);
}


void ezParticleWorldModule::UpdateEffects()
{
  ReconfigureEffects();

  for (ezUInt32 i = 0; i < m_ParticleEffects.GetCount(); ++i)
  {
    m_ParticleEffects[i]->Update();
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
    
    for (ezUInt32 i = 0; i < pEffect->GetParticleSystems().GetCount(); ++i)
    {
      if (!pEffect->GetParticleSystems()[i]->HasActiveParticles())
        continue;

      // Generate batch id from mesh, material and part index. 
      ezUInt32 data[] = { 42 };
      ezUInt32 uiBatchId = ezHashing::MurmurHash(data, sizeof(data));

      ezParticleRenderData* pRenderData = CreateParticleRenderData(uiBatchId);
      {
        pRenderData->m_GlobalTransform = pEffect->GetTransform();
        //pRenderData->m_GlobalBounds = GetOwner()->GetGlobalBounds(); // TODO
        pRenderData->m_pParticleSystem = pEffect->GetParticleSystems()[i];
      }

      // make sure it stays alive until the renderer is done with it
      pRenderData->m_pParticleSystem->IncreaseRefCount();

      // Determine render data category.
      ezRenderData::Category category;
      category = ezDefaultRenderDataCategories::SimpleTransparent;

      ezUInt32 uiSortingKey = 0;
      pExtractedRenderData->AddRenderData(pRenderData, category, uiSortingKey);
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
    pEffect->Reconfigure();
  }

  m_EffectsToReconfigure.Clear();
}
