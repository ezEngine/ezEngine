#include <PCH.h>
#include <ParticlePlugin/WorldModule/ParticleWorldModule.h>
#include <Foundation/Threading/Lock.h>
#include <Core/World/World.h>
#include <ParticlePlugin/Effect/ParticleEffectInstance.h>
#include <Core/ResourceManager/ResourceManager.h>
#include <Core/ResourceManager/ResourceBase.h>
#include <ParticlePlugin/Resources/ParticleEffectResource.h>
#include <RendererCore/Pipeline/ExtractedRenderData.h>
#include <Foundation/Threading/TaskSystem.h>
#include <ParticlePlugin/Streams/ParticleStream.h>
#include <RendererCore/RenderWorld/RenderWorld.h>

EZ_IMPLEMENT_WORLD_MODULE(ezParticleWorldModule);

ezParticleWorldModule::ezParticleWorldModule(ezWorld* pWorld)
  : ezWorldModule(pWorld)
{
  m_uiExtractedFrame = 0;
}

ezParticleWorldModule::~ezParticleWorldModule()
{
  ClearParticleStreamFactories();
}

void ezParticleWorldModule::Initialize()
{
  ConfigureParticleStreamFactories();

  {
    auto updateDesc = EZ_CREATE_MODULE_UPDATE_FUNCTION_DESC(ezParticleWorldModule::UpdateEffects, this);
    updateDesc.m_Phase = ezWorldModule::UpdateFunctionDesc::Phase::PreAsync;
    updateDesc.m_bOnlyUpdateWhenSimulating = true;
    updateDesc.m_fPriority = 1000.0f; // kick off particle tasks as early as possible

    RegisterUpdateFunction(updateDesc);
  }

  {
    auto finishDesc = EZ_CREATE_MODULE_UPDATE_FUNCTION_DESC(ezParticleWorldModule::EnsureUpdatesFinished, this);
    finishDesc.m_Phase = ezWorldModule::UpdateFunctionDesc::Phase::PostTransform;
    finishDesc.m_bOnlyUpdateWhenSimulating = true;
    finishDesc.m_fPriority = -1000.0f; // sync with particle tasks as late as possible

    RegisterUpdateFunction(finishDesc);
  }

  ezResourceManager::s_ResourceEvents.AddEventHandler(ezMakeDelegate(&ezParticleWorldModule::ResourceEventHandler, this));
}


void ezParticleWorldModule::Deinitialize()
{
  EZ_LOCK(m_Mutex);

  ezResourceManager::s_ResourceEvents.RemoveEventHandler(ezMakeDelegate(&ezParticleWorldModule::ResourceEventHandler, this));

  m_ParticleSystemFreeList.Clear();
  m_FinishingEffects.Clear();

  m_ParticleEffects.Clear();
  m_ParticleSystems.Clear();
}

void ezParticleWorldModule::EnsureUpdatesFinished(const ezWorldModule::UpdateContext& context)
{
  // do NOT lock here, otherwise tasks cannot enter the lock
  ezTaskSystem::WaitForGroup(m_EffectUpdateTaskGroup);
}

void ezParticleWorldModule::ExtractRenderData(const ezView& view, ezExtractedRenderData* pExtractedRenderData) const
{
  EZ_ASSERT_RELEASE(ezTaskSystem::IsTaskGroupFinished(m_EffectUpdateTaskGroup), "Particle Effect Update Task is not finished!");

  EZ_LOCK(m_Mutex);

  // increase frame count to identify which system has been updated in this frame already
  ++m_uiExtractedFrame;

  for (auto it = m_ActiveEffects.GetIterator(); it.IsValid(); ++it)
  {
    const ezParticleEffectInstance* pEffect = it.Value();

    if (pEffect->IsSharedEffect())
    {
      const auto& shared = pEffect->GetAllSharedInstances();

      for (ezUInt32 shi = 0; shi < shared.GetCount(); ++shi)
      {
        /// \todo Determine per shared instance, whether it is visible at all

        ExtractEffectRenderData(pEffect, view, pExtractedRenderData, pEffect->GetTransform(shared[shi].m_pSharedInstanceOwner));
      }
    }
    else
    {
      ezTransform systemTransform = ezTransform::Identity();

      if (pEffect->IsSimulatedInLocalSpace())
        systemTransform = pEffect->GetTransform(nullptr);

      /// \todo Determine whether this instance is visible at all

      ExtractEffectRenderData(pEffect, view, pExtractedRenderData, systemTransform);
    }
  }
}


void ezParticleWorldModule::ExtractEffectRenderData(const ezParticleEffectInstance* pEffect, const ezView& view, ezExtractedRenderData* pExtractedRenderData, const ezTransform& systemTransform) const
{
  if (pEffect->m_uiEffectIsInView < ezRenderWorld::GetFrameCounter())
    return;

  for (ezUInt32 i = 0; i < pEffect->GetParticleSystems().GetCount(); ++i)
  {
    const ezParticleSystemInstance* pSystem = pEffect->GetParticleSystems()[i];

    if (pSystem == nullptr)
      continue;

    if (!pSystem->HasActiveParticles() || !pSystem->IsVisible())
      continue;

    pSystem->ExtractSystemRenderData(view, pExtractedRenderData, systemTransform, m_uiExtractedFrame);
  }
}

void ezParticleWorldModule::ResourceEventHandler(const ezResourceEvent& e)
{
  if (e.m_EventType == ezResourceEventType::ResourceContentUnloading && e.m_pResource->GetDynamicRTTI()->IsDerivedFrom<ezParticleEffectResource>())
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

void ezParticleWorldModule::ConfigureParticleStreamFactories()
{
  ClearParticleStreamFactories();

  ezStringBuilder fullName;

  for (ezRTTI* pRtti = ezRTTI::GetFirstInstance(); pRtti != nullptr; pRtti = pRtti->GetNextInstance())
  {
    if (!pRtti->IsDerivedFrom<ezParticleStreamFactory>() || !pRtti->GetAllocator()->CanAllocate())
      continue;

    ezParticleStreamFactory* pFactory = reinterpret_cast<ezParticleStreamFactory*>(pRtti->GetAllocator()->Allocate());

    ezParticleStreamFactory::GetFullStreamName(pFactory->GetStreamName(), pFactory->GetStreamDataType(), fullName);

    m_StreamFactories[fullName] = pFactory;
  }
}

void ezParticleWorldModule::ClearParticleStreamFactories()
{
  for (auto pFactory : m_StreamFactories)
  {
    pFactory->GetDynamicRTTI()->GetAllocator()->Deallocate(pFactory);
  }

  m_StreamFactories.Clear();
}

ezParticleStream* ezParticleWorldModule::CreateStreamDefaultInitializer(ezParticleSystemInstance* pOwner, const char* szFullStreamName) const
{
  auto it = m_StreamFactories.Find(szFullStreamName);
  if (!it.IsValid())
    return nullptr;

  return it.Value()->CreateParticleStream(pOwner);
}

EZ_STATICLINK_FILE(ParticlePlugin, ParticlePlugin_WorldModule_ParticleWorldModule);

