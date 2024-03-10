#include <ParticlePlugin/ParticlePluginPCH.h>

#include <Core/Interfaces/PhysicsWorldModule.h>
#include <Core/ResourceManager/Resource.h>
#include <Core/ResourceManager/ResourceManager.h>
#include <Core/World/World.h>
#include <Foundation/Threading/Lock.h>
#include <Foundation/Threading/TaskSystem.h>
#include <ParticlePlugin/Components/ParticleComponent.h>
#include <ParticlePlugin/Components/ParticleFinisherComponent.h>
#include <ParticlePlugin/Effect/ParticleEffectInstance.h>
#include <ParticlePlugin/Module/ParticleModule.h>
#include <ParticlePlugin/Resources/ParticleEffectResource.h>
#include <ParticlePlugin/Streams/ParticleStream.h>
#include <ParticlePlugin/WorldModule/ParticleWorldModule.h>
#include <RendererCore/Pipeline/ExtractedRenderData.h>
#include <RendererCore/RenderWorld/RenderWorld.h>

// clang-format off
EZ_IMPLEMENT_WORLD_MODULE(ezParticleWorldModule);
EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezParticleWorldModule, 1, ezRTTINoAllocator)
EZ_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

ezParticleWorldModule::ezParticleWorldModule(ezWorld* pWorld)
  : ezWorldModule(pWorld)
{
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

  ezResourceManager::GetResourceEvents().AddEventHandler(ezMakeDelegate(&ezParticleWorldModule::ResourceEventHandler, this));

  ezRTTI::ForEachDerivedType<ezParticleModule>(
    [this](const ezRTTI* pRtti)
    {
      ezUniquePtr<ezParticleModule> pModule = pRtti->GetAllocator()->Allocate<ezParticleModule>();
      pModule->RequestRequiredWorldModulesForCache(this);
    },
    ezRTTI::ForEachOptions::ExcludeNonAllocatable);
}


void ezParticleWorldModule::Deinitialize()
{
  EZ_LOCK(m_Mutex);

  ezResourceManager::GetResourceEvents().RemoveEventHandler(ezMakeDelegate(&ezParticleWorldModule::ResourceEventHandler, this));

  WorldClear();
}

void ezParticleWorldModule::EnsureUpdatesFinished(const ezWorldModule::UpdateContext& context)
{
  // do NOT lock here, otherwise tasks cannot enter the lock
  ezTaskSystem::WaitForGroup(m_EffectUpdateTaskGroup);

  {
    EZ_LOCK(m_Mutex);

    // The simulation tasks are done and the game objects have their global transform updated at this point, so we can push the transform
    // to the particle effects for the next simulation step and also ensure that the bounding volumes are correct for culling and rendering.
    if (ezParticleComponentManager* pManager = GetWorld()->GetComponentManager<ezParticleComponentManager>())
    {
      pManager->UpdatePfxTransformsAndBounds();
    }

    if (ezParticleFinisherComponentManager* pManager = GetWorld()->GetComponentManager<ezParticleFinisherComponentManager>())
    {
      pManager->UpdateBounds();
    }

    for (ezUInt32 i = 0; i < m_NeedFinisherComponent.GetCount(); ++i)
    {
      CreateFinisherComponent(m_NeedFinisherComponent[i]);
    }

    m_NeedFinisherComponent.Clear();
  }
}

void ezParticleWorldModule::ExtractEffectRenderData(const ezParticleEffectInstance* pEffect, ezMsgExtractRenderData& ref_msg, const ezTransform& systemTransform) const
{
  EZ_ASSERT_DEBUG(ezTaskSystem::IsTaskGroupFinished(m_EffectUpdateTaskGroup), "Particle Effect Update Task is not finished!");

  EZ_LOCK(m_Mutex);

  for (ezUInt32 i = 0; i < pEffect->GetParticleSystems().GetCount(); ++i)
  {
    const ezParticleSystemInstance* pSystem = pEffect->GetParticleSystems()[i];

    if (pSystem == nullptr)
      continue;

    if (!pSystem->HasActiveParticles() || !pSystem->IsVisible())
      continue;

    pSystem->ExtractSystemRenderData(ref_msg, systemTransform);
  }
}

void ezParticleWorldModule::ResourceEventHandler(const ezResourceEvent& e)
{
  if (e.m_Type == ezResourceEvent::Type::ResourceContentUnloading && e.m_pResource->GetDynamicRTTI()->IsDerivedFrom<ezParticleEffectResource>())
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

  ezRTTI::ForEachDerivedType<ezParticleStreamFactory>(
    [&](const ezRTTI* pRtti)
    {
      ezParticleStreamFactory* pFactory = pRtti->GetAllocator()->Allocate<ezParticleStreamFactory>();

      ezParticleStreamFactory::GetFullStreamName(pFactory->GetStreamName(), pFactory->GetStreamDataType(), fullName);

      m_StreamFactories[fullName] = pFactory;
    },
    ezRTTI::ForEachOptions::ExcludeNonAllocatable);
}

void ezParticleWorldModule::ClearParticleStreamFactories()
{
  for (auto it : m_StreamFactories)
  {
    it.Value()->GetDynamicRTTI()->GetAllocator()->Deallocate(it.Value());
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

ezWorldModule* ezParticleWorldModule::GetCachedWorldModule(const ezRTTI* pRtti) const
{
  ezWorldModule* pModule = nullptr;
  m_WorldModuleCache.TryGetValue(pRtti, pModule);
  return pModule;
}

void ezParticleWorldModule::CacheWorldModule(const ezRTTI* pRtti)
{
  m_WorldModuleCache[pRtti] = GetWorld()->GetOrCreateModule(pRtti);
}

void ezParticleWorldModule::CreateFinisherComponent(ezParticleEffectInstance* pEffect)
{
  if (pEffect && !pEffect->IsSharedEffect())
  {
    pEffect->SetVisibleIf(nullptr);

    ezWorld* pWorld = GetWorld();

    const ezTransform transform = pEffect->GetTransform();

    ezGameObjectDesc go;
    go.m_LocalPosition = transform.m_vPosition;
    go.m_LocalRotation = transform.m_qRotation;
    go.m_LocalScaling = transform.m_vScale;
    // go.m_Tags = GetOwner()->GetTags(); // TODO: pass along tags -> needed for rendering filters

    ezGameObject* pFinisher;
    pWorld->CreateObject(go, pFinisher);

    ezParticleFinisherComponent* pFinisherComp;
    ezParticleFinisherComponent::CreateComponent(pFinisher, pFinisherComp);

    pFinisherComp->m_EffectController = ezParticleEffectController(this, pEffect->GetHandle());
    pFinisherComp->m_EffectController.SetTransform(transform, ezVec3::MakeZero()); // clear the velocity
  }
}

void ezParticleWorldModule::WorldClear()
{
  // make sure no particle update task is still in the pipeline
  ezTaskSystem::WaitForGroup(m_EffectUpdateTaskGroup);

  EZ_LOCK(m_Mutex);

  m_FinishingEffects.Clear();
  m_NeedFinisherComponent.Clear();

  m_ActiveEffects.Clear();
  m_ParticleEffects.Clear();
  m_ParticleSystems.Clear();
  m_ParticleEffectsFreeList.Clear();
  m_ParticleSystemFreeList.Clear();
}

EZ_STATICLINK_FILE(ParticlePlugin, ParticlePlugin_WorldModule_ParticleWorldModule);
