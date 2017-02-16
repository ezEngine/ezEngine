#include <PCH.h>
#include <ParticlePlugin/System/ParticleSystemInstance.h>
#include <ParticlePlugin/Effect/ParticleEffectInstance.h>
#include <Foundation/DataProcessing/Stream/ProcessingStreamProcessor.h>
#include <Foundation/DataProcessing/Stream/ProcessingStreamIterator.h>
#include <GameEngine/Interfaces/PhysicsWorldModule.h>
#include <Core/World/World.h>
#include <ParticlePlugin/Emitter/ParticleEmitter.h>
#include <ParticlePlugin/Behavior/ParticleBehavior.h>
#include <ParticlePlugin/System/ParticleSystemDescriptor.h>
#include <ParticlePlugin/Initializer/ParticleInitializer.h>
#include <ParticlePlugin/Type/ParticleType.h>
#include <Foundation/DataProcessing/Stream/DefaultImplementations/ZeroInitializer.h>
#include <ParticlePlugin/Streams/ParticleStream.h>
#include <ParticlePlugin/WorldModule/ParticleWorldModule.h>

bool ezParticleSystemInstance::HasActiveParticles() const
{
  return m_StreamGroup.GetNumActiveElements() > 0;
}

bool ezParticleSystemInstance::IsEmitterConfigEqual(const ezParticleSystemDescriptor* pTemplate) const
{
  const auto& factories = pTemplate->GetEmitterFactories();

  if (factories.GetCount() != m_Emitters.GetCount())
    return false;

  for (ezUInt32 i = 0; i < factories.GetCount(); ++i)
  {
    if (factories[i]->GetEmitterType() != m_Emitters[i]->GetDynamicRTTI())
      return false;
  }

  return true;
}

bool ezParticleSystemInstance::IsInitializerConfigEqual(const ezParticleSystemDescriptor* pTemplate) const
{
  const auto& factories = pTemplate->GetInitializerFactories();

  if (factories.GetCount() != m_Initializers.GetCount())
    return false;

  for (ezUInt32 i = 0; i < factories.GetCount(); ++i)
  {
    if (factories[i]->GetInitializerType() != m_Initializers[i]->GetDynamicRTTI())
      return false;
  }

  return true;
}

bool ezParticleSystemInstance::IsBehaviorConfigEqual(const ezParticleSystemDescriptor* pTemplate) const
{
  const auto& factories = pTemplate->GetBehaviorFactories();

  if (factories.GetCount() != m_Behaviors.GetCount())
    return false;

  for (ezUInt32 i = 0; i < factories.GetCount(); ++i)
  {
    if (factories[i]->GetBehaviorType() != m_Behaviors[i]->GetDynamicRTTI())
      return false;
  }

  return true;
}

bool ezParticleSystemInstance::IsTypeConfigEqual(const ezParticleSystemDescriptor* pTemplate) const
{
  const auto& factories = pTemplate->GetTypeFactories();

  if (factories.GetCount() != m_Types.GetCount())
    return false;

  for (ezUInt32 i = 0; i < factories.GetCount(); ++i)
  {
    if (factories[i]->GetTypeType() != m_Types[i]->GetDynamicRTTI())
      return false;
  }

  return true;
}


void ezParticleSystemInstance::ConfigureFromTemplate(const ezParticleSystemDescriptor* pTemplate)
{
  m_bVisible = pTemplate->m_bVisible;

  for (auto& info : m_StreamInfo)
  {
    info.m_bGetsInitialized = false;
    info.m_bInUse = false;
  }

  const bool allProcessorsEqual =
    IsEmitterConfigEqual(pTemplate) &&
    IsInitializerConfigEqual(pTemplate) &&
    IsBehaviorConfigEqual(pTemplate) &&
    IsTypeConfigEqual(pTemplate);

  if (!allProcessorsEqual)
  {
    // recreate emitters, initializers and behaviors
    CreateStreamProcessors(pTemplate);
  }
  else
  {
    // just re-initialize the emitters with the new properties
    ReinitializeStreamProcessors(pTemplate);
  }

  // setup stream initializers for all streams that have none yet
  CreateStreamZeroInitializers();
}

void ezParticleSystemInstance::CreateStreamProcessors(const ezParticleSystemDescriptor* pTemplate)
{
  // all spawners get cleared, so clear this as well
  // this has to be done before any streams get created
  //for (auto& info : m_StreamInfo)
  //{
  //  info.m_pZeroInitializer = nullptr;
  //}


  const ezUInt64 uiMaxParticles = m_StreamGroup.GetNumElements();
  m_StreamGroup.Clear();
  m_StreamGroup.SetSize(uiMaxParticles);
  m_StreamInfo.Clear();

  // emitters
  {
    m_Emitters.Clear();

    for (const auto pFactory : pTemplate->GetEmitterFactories())
    {
      ezParticleEmitter* pEmitter = pFactory->CreateEmitter(this);
      m_StreamGroup.AddProcessor(pEmitter);
      m_Emitters.PushBack(pEmitter);
    }
  }

  // initializers
  {
    m_Initializers.Clear();

    for (const auto pFactory : pTemplate->GetInitializerFactories())
    {
      ezParticleInitializer* pInitializer = pFactory->CreateInitializer(this);
      m_StreamGroup.AddProcessor(pInitializer);
      m_Initializers.PushBack(pInitializer);
    }
  }

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

void ezParticleSystemInstance::ReinitializeStreamProcessors(const ezParticleSystemDescriptor* pTemplate)
{
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

void ezParticleSystemInstance::Construct(ezUInt32 uiMaxParticles, ezWorld* pWorld, ezUInt64 uiRandomSeed, ezParticleEffectInstance* pOwnerEffect)
{
  m_pOwnerEffect = pOwnerEffect;
  m_bEmitterEnabled = true;
  m_bVisible = true;
  m_pWorld = pWorld;

  m_StreamInfo.Clear();
  m_StreamGroup.SetSize(uiMaxParticles);

  if (uiRandomSeed == 0)
    m_Random.InitializeFromCurrentTime();
  else
    m_Random.Initialize(uiRandomSeed);
}

void ezParticleSystemInstance::Destruct()
{
  m_StreamGroup.Clear();
  m_Emitters.Clear();
  m_Initializers.Clear();
  m_Behaviors.Clear();
  m_Types.Clear();

  m_StreamInfo.Clear();
}

ezParticleSystemState::Enum ezParticleSystemInstance::Update(const ezTime& tDiff)
{
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
          m_StreamGroup.InitializeElements(uiSpawn);
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
          m_StreamGroup.InitializeElements(uiSpawn);
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
  fullName.AppendFormat("({0})", (int)Type);

  return m_StreamGroup.GetStreamByName(fullName);
}

void ezParticleSystemInstance::CreateStream(const char* szName, ezProcessingStream::DataType Type, ezProcessingStream** ppStream, ezParticleStreamBinding& binding, bool bWillInitializeElements)
{
  EZ_ASSERT_DEV(ppStream != nullptr, "The pointer to the stream pointer must not be null");

  ezStringBuilder fullName;
  ezParticleStreamFactory::GetFullStreamName(szName, Type, fullName);

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
  if (bWillInitializeElements)
    pInfo->m_bGetsInitialized = true;

  EZ_ASSERT_DEV(pStream != nullptr, "Stream creation failed ('{0}' -> '{1}')", szName, fullName.GetData());
  *ppStream = pStream;

  {
    auto& bind = binding.m_Bindings.ExpandAndGetRef();
    bind.m_ppStream = ppStream;
    bind.m_sName = fullName;
  }
}

void ezParticleSystemInstance::CreateStreamZeroInitializers()
{
  for (ezUInt32 i = 0; i < m_StreamInfo.GetCount(); )
  {
    auto& info = m_StreamInfo[i];

    if ((!info.m_bInUse || info.m_bGetsInitialized) && info.m_pDefaultInitializer)
    {
      m_StreamGroup.RemoveProcessor(info.m_pDefaultInitializer);
      info.m_pDefaultInitializer = nullptr;
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

    if (info.m_pDefaultInitializer == nullptr)
    {
      ezParticleStream* pStream = GetOwnerWorldModule()->CreateStreamDefaultInitializer(this, info.m_sName);

      if (pStream == nullptr)
      {
        ezLog::Warning("Particle stream '{0}' is zero-initialized.", info.m_sName.GetData());

        ezProcessingStreamSpawnerZeroInitialized* pZeroInit = EZ_DEFAULT_NEW(ezProcessingStreamSpawnerZeroInitialized);
        pZeroInit->SetStreamName(info.m_sName);

        info.m_pDefaultInitializer = pZeroInit;
      }
      else
      {
        //ezLog::Debug("Particle stream '{0}' is default-initialized.", info.m_sName.GetData());
        info.m_pDefaultInitializer = pStream;
      }

      m_StreamGroup.AddProcessor(info.m_pDefaultInitializer);
    }
  }
}

void ezParticleStreamBinding::UpdateBindings(const ezProcessingStreamGroup* pGroup) const
{
  for (const auto& bind : m_Bindings)
  {
    ezProcessingStream* pStream = pGroup->GetStreamByName(bind.m_sName);
    EZ_ASSERT_DEV(pStream != nullptr, "Stream binding '{0}' is invalid now", bind.m_sName.GetData());

    *bind.m_ppStream = pStream;
  }
}

ezParticleSystemInstance::StreamInfo::StreamInfo()
{
  m_pDefaultInitializer = nullptr;
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

ezParticleWorldModule* ezParticleSystemInstance::GetOwnerWorldModule() const
{
  return m_pOwnerEffect->GetOwnerWorldModule();
}

void ezParticleSystemInstance::ExtractSystemRenderData(const ezView& view, ezExtractedRenderData* pExtractedRenderData, const ezTransform& instanceTransform, ezUInt64 uiExtractedFrame) const
{
  for (auto pType : m_Types)
  {
    pType->ExtractTypeRenderData(view, pExtractedRenderData, instanceTransform, uiExtractedFrame);
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



EZ_STATICLINK_FILE(ParticlePlugin, ParticlePlugin_System_ParticleSystemInstance);

