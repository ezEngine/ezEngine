#include <ParticlePlugin/PCH.h>
#include <ParticlePlugin/System/ParticleSystemInstance.h>
#include <CoreUtils/DataProcessing/Stream/ProcessingStreamSpawner.h>
#include <CoreUtils/DataProcessing/Stream/ProcessingStreamProcessor.h>
#include <CoreUtils/DataProcessing/Stream/ProcessingStreamIterator.h>
#include <GameUtils/Interfaces/PhysicsWorldModule.h>
#include <Core/World/World.h>
#include <ParticlePlugin/Emitter/ParticleEmitter.h>
#include <ParticlePlugin/Behavior/ParticleBehavior.h>
#include <ParticlePlugin/System/ParticleSystemDescriptor.h>
#include <ParticlePlugin/Initializer/ParticleInitializer.h>
#include <ParticlePlugin/Type/ParticleType.h>
#include <CoreUtils/DataProcessing/Stream/DefaultImplementations/ZeroInitializer.h>


ezParticleSystemInstance::ezParticleSystemInstance()
{
  m_bVisible = true;
  m_bEmitterEnabled = true;
}

bool ezParticleSystemInstance::HasActiveParticles() const
{
  return m_StreamGroup.GetNumActiveElements() > 0;
}

void ezParticleSystemInstance::ConfigureFromTemplate(const ezParticleSystemDescriptor* pTemplate)
{
  EZ_LOCK(m_Mutex);

  m_bVisible = pTemplate->m_bVisible;

  bool allSpawnersEqual = true;
  bool allProcessorsEqual = true;

  for (auto& info : m_StreamInfo)
  {
    info.m_bGetsInitialized = false;
    info.m_bInUse = false;
  }

  // emitters
  if (allSpawnersEqual)
  {
    const auto& factories = pTemplate->GetEmitterFactories();

    if (factories.GetCount() == m_Emitters.GetCount())
    {
      for (ezUInt32 i = 0; i < factories.GetCount(); ++i)
      {
        if (factories[i]->GetEmitterType() != m_Emitters[i]->GetDynamicRTTI())
        {
          allSpawnersEqual = false;
          break;
        }
      }
    }
    else
    {
      allSpawnersEqual = false;
    }
  }

  // initializers
  if (allSpawnersEqual)
  {
    const auto& factories = pTemplate->GetInitializerFactories();

    if (factories.GetCount() == m_Initializers.GetCount())
    {
      for (ezUInt32 i = 0; i < factories.GetCount(); ++i)
      {
        if (factories[i]->GetInitializerType() != m_Initializers[i]->GetDynamicRTTI())
        {
          allSpawnersEqual = false;
          break;
        }
      }
    }
    else
    {
      allSpawnersEqual = false;
    }
  }

  // behaviors
  if (allProcessorsEqual)
  {
    const auto& factories = pTemplate->GetBehaviorFactories();

    if (factories.GetCount() == m_Behaviors.GetCount())
    {
      for (ezUInt32 i = 0; i < factories.GetCount(); ++i)
      {
        if (factories[i]->GetBehaviorType() != m_Behaviors[i]->GetDynamicRTTI())
        {
          allProcessorsEqual = false;
          break;
        }
      }
    }
    else
    {
      allProcessorsEqual = false;
    }
  }

  // types
  if (allProcessorsEqual)
  {
    const auto& factories = pTemplate->GetTypeFactories();

    if (factories.GetCount() == m_Types.GetCount())
    {
      for (ezUInt32 i = 0; i < factories.GetCount(); ++i)
      {
        if (factories[i]->GetTypeType() != m_Types[i]->GetDynamicRTTI())
        {
          allProcessorsEqual = false;
          break;
        }
      }
    }
    else
    {
      allProcessorsEqual = false;
    }
  }

  if (!allSpawnersEqual)
  {
    // all spawners get cleared, so clear this as well
    // this has to be done before any streams get created
    for (auto& info : m_StreamInfo)
    {
      info.m_pInitializer = nullptr;
    }
  }

  if (!allSpawnersEqual)
  {
    // recreate emitters, initializers and behaviors
    m_StreamGroup.ClearSpawners();

    // emitters
    {
      m_Emitters.Clear();

      for (const auto pFactory : pTemplate->GetEmitterFactories())
      {
        ezParticleEmitter* pEmitter = pFactory->CreateEmitter(this);
        m_StreamGroup.AddSpawner(pEmitter);
        m_Emitters.PushBack(pEmitter);
      }
    }

    // initializers
    {
      m_Initializers.Clear();

      for (const auto pFactory : pTemplate->GetInitializerFactories())
      {
        ezParticleInitializer* pInitializer = pFactory->CreateInitializer(this);
        m_StreamGroup.AddSpawner(pInitializer);
        m_Initializers.PushBack(pInitializer);
      }
    }
  }
  else
  {
    // just re-initialize the emitters with the new properties

    // emitters
    {
      const auto& factories = pTemplate->GetEmitterFactories();

      for (ezUInt32 i = 0; i < factories.GetCount(); ++i)
      {
        m_Emitters[i]->Reset(this);
        factories[i]->CopyEmitterProperties(m_Emitters[i]);
        m_Emitters[i]->AfterPropertiesConfigured(false);
        m_Emitters[i]->CreateRequiredStreams();
      }
    }

    // initializers
    {
      const auto& factories = pTemplate->GetInitializerFactories();

      for (ezUInt32 i = 0; i < factories.GetCount(); ++i)
      {
        m_Initializers[i]->Reset(this);
        factories[i]->CopyInitializerProperties(m_Initializers[i]);
        m_Initializers[i]->AfterPropertiesConfigured(false);
        m_Initializers[i]->CreateRequiredStreams();
      }
    }
  }

  if (!allProcessorsEqual)
  {
    m_StreamGroup.ClearProcessors();

    // behaviors
    {
      m_Behaviors.Clear();

      for (const auto pFactory : pTemplate->GetBehaviorFactories())
      {
        ezParticleBehavior* pBehavior = pFactory->CreateBehavior(this);
        m_StreamGroup.AddProcessor(pBehavior);
        m_Behaviors.PushBack(pBehavior);
      }
    }

    // types
    {
      m_Types.Clear();

      for (const auto pFactory : pTemplate->GetTypeFactories())
      {
        ezParticleType* pType = pFactory->CreateType(this);
        m_StreamGroup.AddProcessor(pType);
        m_Types.PushBack(pType);
      }
    }
  }
  else
  {
    // behaviors
    {
      const auto& factories = pTemplate->GetBehaviorFactories();

      for (ezUInt32 i = 0; i < factories.GetCount(); ++i)
      {
        m_Behaviors[i]->Reset(this);
        factories[i]->CopyBehaviorProperties(m_Behaviors[i]);
        m_Behaviors[i]->AfterPropertiesConfigured(false);
        m_Behaviors[i]->CreateRequiredStreams();
      }
    }

    // types
    {
      const auto& factories = pTemplate->GetTypeFactories();

      for (ezUInt32 i = 0; i < factories.GetCount(); ++i)
      {
        m_Types[i]->Reset(this);
        factories[i]->CopyTypeProperties(m_Types[i]);
        m_Types[i]->AfterPropertiesConfigured(false);
        m_Types[i]->CreateRequiredStreams();
      }
    }
  }

  // setup stream initializers for all streams that have none yet
  CreateStreamZeroInitializers();
}

void ezParticleSystemInstance::Initialize(ezUInt32 uiMaxParticles, ezWorld* pWorld, ezUInt64 uiRandomSeed, ezParticleEffectInstance* pOwnerEffect)
{
  EZ_LOCK(m_Mutex);

  if (uiRandomSeed == 0)
  {
    m_Random.InitializeFromCurrentTime();
  }
  else
  {
    m_Random.Initialize(uiRandomSeed);
  }

  m_pOwnerEffect = pOwnerEffect;
  m_bEmitterEnabled = true;
  m_pWorld = pWorld;

  m_StreamGroup.Clear();
  m_Emitters.Clear();
  m_Initializers.Clear();
  m_Behaviors.Clear();

  m_StreamInfo.Clear();
  m_StreamGroup.SetSize(uiMaxParticles);
}

ezParticleSystemState::Enum ezParticleSystemInstance::Update(const ezTime& tDiff)
{
  EZ_LOCK(m_Mutex);

  if (m_bEmitterEnabled)
  {
    // if all emitters are finished, we deactivate this on the whole system
    m_bEmitterEnabled = false;

    for (auto pEmitter : m_Emitters)
    {
      if (pEmitter->IsFinished() == ezParticleEmitterState::Active)
      {
        m_bEmitterEnabled = true;
        const ezUInt32 uiSpawn = pEmitter->ComputeSpawnCount(tDiff);

        if (uiSpawn > 0)
        {
          m_StreamGroup.SpawnElements(uiSpawn);
        }
      }
    }
  }

  bool bHasReactingEmitters = false;

  // always check reactive emitters, as long as there are particles alive, they might produce more
  {
    for (auto pEmitter : m_Emitters)
    {
      if (pEmitter->IsFinished() == ezParticleEmitterState::OnlyReacting)
      {
        bHasReactingEmitters = true;

        const ezUInt32 uiSpawn = pEmitter->ComputeSpawnCount(tDiff);

        if (uiSpawn > 0)
        {
          m_StreamGroup.SpawnElements(uiSpawn);
        }
      }
    }
  }

  for (auto pBehavior : m_Behaviors)
  {
    pBehavior->StepParticleSystem(tDiff);
  }

  for (auto pType : m_Types)
  {
    pType->StepParticleSystem(tDiff);
  }

  m_StreamGroup.Process();

  if (m_bEmitterEnabled)
    return ezParticleSystemState::Active;

  // all emitters are done, but some particles are still alive
  if (HasActiveParticles())
    return ezParticleSystemState::EmittersFinished;

  return bHasReactingEmitters ? ezParticleSystemState::OnlyReacting : ezParticleSystemState::Inactive;
}

const ezProcessingStream* ezParticleSystemInstance::QueryStream(const char* szName, ezProcessingStream::DataType Type) const
{
  ezStringBuilder fullName(szName);
  fullName.AppendFormat("(%i)", (int)Type);

  return m_StreamGroup.GetStreamByName(fullName);
}

void ezParticleSystemInstance::CreateStream(const char* szName, ezProcessingStream::DataType Type, ezProcessingStream** ppStream, ezParticleStreamBinding& binding, bool bExpectInitializedValue)
{
  EZ_ASSERT_DEV(ppStream != nullptr, "The pointer to the stream pointer must not be null");

  ezStringBuilder fullName(szName);
  fullName.AppendFormat("(%i)", (int)Type);

  StreamInfo* pInfo = nullptr;

  ezProcessingStream* pStream = m_StreamGroup.GetStreamByName(fullName);
  if (pStream == nullptr)
  {
    pStream = m_StreamGroup.AddStream(fullName, Type);

    pInfo = &m_StreamInfo.ExpandAndGetRef();
    pInfo->m_sName = fullName;
  }
  else
  {
    for (auto& info : m_StreamInfo)
    {
      if (info.m_sName == fullName)
      {
        pInfo = &info;
        break;
      }
    }

    EZ_ASSERT_DEV(pInfo != nullptr, "Could not find info for stream");
  }

  pInfo->m_bInUse = true;
  if (!bExpectInitializedValue)
    pInfo->m_bGetsInitialized = true;

  EZ_ASSERT_DEV(pStream != nullptr, "Stream creation failed ('%s' -> '%s')", szName, fullName.GetData());
  *ppStream = pStream;

  auto& bind = binding.m_Bindings.ExpandAndGetRef();
  bind.m_ppStream = ppStream;
  bind.m_sName = fullName;
}

void ezParticleSystemInstance::CreateStreamZeroInitializers()
{
  for (ezUInt32 i = 0; i < m_StreamInfo.GetCount(); )
  {
    auto& info = m_StreamInfo[i];

    if ((!info.m_bInUse || info.m_bGetsInitialized) && info.m_pInitializer)
    {
      m_StreamGroup.RemoveSpawner(info.m_pInitializer);
      info.m_pInitializer = nullptr;
    }

    if (!info.m_bInUse)
    {
      m_StreamGroup.RemoveStreamByName(info.m_sName.GetData());
      m_StreamInfo.RemoveAtSwap(i);
    }
    else
    {
      ++i;
    }
  }

  for (auto& info : m_StreamInfo)
  {
    if (info.m_bGetsInitialized)
      continue;

    EZ_ASSERT_DEV(info.m_bInUse, "Invalid state");

    ezLog::Warning("Particle stream '%s' is zero-initialized.", info.m_sName.GetData());

    if (info.m_pInitializer == nullptr)
    {
      info.m_pInitializer = EZ_DEFAULT_NEW(ezProcessingStreamSpawnerZeroInitialized, info.m_sName);
      m_StreamGroup.AddSpawner(info.m_pInitializer);
    }
  }
}

void ezParticleStreamBinding::UpdateBindings(const ezProcessingStreamGroup* pGroup) const
{
  for (const auto& bind : m_Bindings)
  {
    ezProcessingStream* pStream = pGroup->GetStreamByName(bind.m_sName);
    EZ_ASSERT_DEV(pStream != nullptr, "Stream binding '%s' is invalid now", bind.m_sName.GetData());

    *bind.m_ppStream = pStream;
  }
}

ezParticleSystemInstance::StreamInfo::StreamInfo()
{
  m_pInitializer = nullptr;
  m_bGetsInitialized = false;
  m_bInUse = false;
}

void ezParticleSystemInstance::ProcessEventQueue(const ezParticleEventQueue* pQueue)
{
  for (auto pEmitter : m_Emitters)
  {
    pEmitter->ProcessEventQueue(pQueue);
  }
}

void ezParticleSystemInstance::ExtractRenderData(const ezView& view, ezExtractedRenderData* pExtractedRenderData, const ezTransform& instanceTransform, ezUInt64 uiExtractedFrame) const
{
  for (auto pType : m_Types)
  {
    pType->ExtractRenderData(view, pExtractedRenderData, instanceTransform, uiExtractedFrame);
  }

}

void ezParticleSystemInstance::AddParticleDeathEventHandler(ParticleDeathHandler handler)
{
  m_StreamGroup.m_ElementRemovedEvent.AddEventHandler(handler);
}

void ezParticleSystemInstance::RemoveParticleDeathEventHandler(ParticleDeathHandler handler)
{
  m_StreamGroup.m_ElementRemovedEvent.RemoveEventHandler(handler);
}

