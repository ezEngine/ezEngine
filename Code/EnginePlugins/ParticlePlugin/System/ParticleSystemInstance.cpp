#include <ParticlePlugin/ParticlePluginPCH.h>

#include <Core/Interfaces/PhysicsWorldModule.h>
#include <Core/World/World.h>
#include <Foundation/DataProcessing/Stream/ProcessingStreamIterator.h>
#include <Foundation/DataProcessing/Stream/ProcessingStreamProcessor.h>
#include <Foundation/Profiling/Profiling.h>
#include <ParticlePlugin/Behavior/ParticleBehavior.h>
#include <ParticlePlugin/Effect/ParticleEffectInstance.h>
#include <ParticlePlugin/Emitter/ParticleEmitter.h>
#include <ParticlePlugin/Finalizer/ParticleFinalizer.h>
#include <ParticlePlugin/Initializer/ParticleInitializer.h>
#include <ParticlePlugin/Module/ParticleModule.h>
#include <ParticlePlugin/System/ParticleSystemDescriptor.h>
#include <ParticlePlugin/System/ParticleSystemInstance.h>
#include <ParticlePlugin/Type/ParticleType.h>
#include <ParticlePlugin/WorldModule/ParticleWorldModule.h>
#include <RendererCore/RenderWorld/RenderWorld.h>

namespace
{
  EZ_ALWAYS_INLINE const ezParticleEmitter* GetEmitter(const ezParticleModuleInstance& module)
  {
    return static_cast<const ezParticleEmitter*>(module.m_pModule);
  }

  EZ_ALWAYS_INLINE const ezParticleType* GetType(const ezParticleModuleInstance& module)
  {
    return static_cast<const ezParticleType*>(module.m_pModule);
  }

  EZ_ALWAYS_INLINE const ezParticleInitializer* GetInitializer(const ezParticleModuleInstance& module)
  {
    return static_cast<const ezParticleInitializer*>(module.m_pModule);
  }

  template <typename INSTANCES, typename MODULES>
  bool AreModuleTypesEqual(const INSTANCES& instances, const MODULES& modules)
  {
    if (modules.GetCount() != instances.GetCount())
      return false;

    for (ezUInt32 i = 0; i < modules.GetCount(); ++i)
    {
      if (modules[i]->GetDynamicRTTI() != instances[i].m_pModule->GetDynamicRTTI())
        return false;
    }

    return true;
  }
} // namespace

template <typename FUNC>
void ezParticleSystemInstance::ForEachModule(FUNC func)
{
  for (ezParticleModuleInstance& module : m_Emitters)
  {
    func(module);
  }

  for (ezParticleModuleInstance& module : m_Initializers)
  {
    func(module);
  }

  for (ezParticleModuleInstance& module : m_Behaviors)
  {
    func(module);
  }

  for (ezParticleModuleInstance& module : m_Finalizers)
  {
    func(module);
  }

  for (ezParticleModuleInstance& module : m_Types)
  {
    func(module);
  }
}

bool ezParticleSystemInstance::HasActiveParticles() const
{
  return m_StreamGroup.GetNumActiveElements() > 0;
}

void ezParticleSystemInstance::ConfigureFromTemplate(const ezParticleSystemDescriptor* pTemplate)
{
  m_bVisible = pTemplate->m_bVisible;

  for (auto& info : m_StreamInfo)
  {
    info.m_bGetsInitialized = false;
    info.m_bInUse = false;
  }

  const bool allProcessorsEqual = AreModuleTypesEqual(m_Emitters, pTemplate->GetEmitters()) && AreModuleTypesEqual(m_Initializers, pTemplate->GetInitializers()) && AreModuleTypesEqual(m_Behaviors, pTemplate->GetBehaviors()) && AreModuleTypesEqual(m_Types, pTemplate->GetTypes()) && AreModuleTypesEqual(m_Finalizers, pTemplate->GetFinalizers());

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
  // this also removes the streams that are not in use anymore, so it has to happen before the modules query for optional streams
  CreateStreamDefaultInitializers();

  SetupOptionalStreams();
}


void ezParticleSystemInstance::Finalize()
{
  m_WorldUpdateModules.Clear();
  m_OwnerMessageModules.Clear();
  m_ParticleDeathModules.Clear();

  ForEachModule([&](ezParticleModuleInstance& module)
    {
      module.m_pModule->OnFinalize(module.m_Context);

      if (module.m_pModule->GetUpdateFromWorld())
      {
        m_WorldUpdateModules.PushBack(&module);
      }

      if (module.m_pModule->GetReceiveOwnerMessages())
      {
        m_OwnerMessageModules.PushBack(&module);
      }

      if (module.m_pModule->GetReceiveParticleDeath())
      {
        m_ParticleDeathModules.PushBack(&module);
      } });
}

void ezParticleSystemInstance::UpdateModulesFromWorld(const ezWorld& world, ezGameObjectHandle hOwnerObject)
{
  for (ezParticleModuleInstance* pInstance : m_WorldUpdateModules)
  {
    pInstance->m_pModule->UpdateFromWorld(pInstance->m_Context, world, hOwnerObject);
  }
}

bool ezParticleSystemInstance::PrepareToSpawn()
{
  bool bReady = true;

  // no early out, so that all initializers can start loading their resources
  for (const ezParticleModuleInstance& initializer : m_Initializers)
  {
    if (!GetInitializer(initializer)->PrepareToSpawn(initializer.m_Context))
    {
      bReady = false;
    }
  }

  if (!bReady)
    return false;

  if (!m_bInitializerSpawnCountMultiplierChecked)
  {
    m_bInitializerSpawnCountMultiplierChecked = true;

    float fMultiplier = 1.0f;
    for (const ezParticleModuleInstance& initializer : m_Initializers)
    {
      fMultiplier *= GetInitializer(initializer)->GetInstanceSpawnCountMultiplier(initializer.m_Context);
    }

    fMultiplier = ezMath::Max(0.0f, fMultiplier);

    if (ezMath::Abs(fMultiplier - m_fInitializerSpawnCountMultiplier) > 0.001f * ezMath::Max(fMultiplier, m_fInitializerSpawnCountMultiplier))
    {
      // the emitters copy their spawn counts during configuration and the max particle count may change as well
      m_fRequiredInitializerSpawnCountMultiplier = fMultiplier;
      GetOwnerWorldModule()->QueueEffectReconfigure(m_pOwnerEffect);
    }
  }

  // waiting for the reconfigure, see SetSpawnCountMultiplier()
  return m_fRequiredInitializerSpawnCountMultiplier == m_fInitializerSpawnCountMultiplier;
}

void ezParticleSystemInstance::SetSpawnCountMultiplier(float fMultiplier, float fInitializerMultiplier, bool bInitializerMultiplierChecked)
{
  m_fSpawnCountMultiplier = fMultiplier;
  m_fInitializerSpawnCountMultiplier = fInitializerMultiplier;
  m_fRequiredInitializerSpawnCountMultiplier = fInitializerMultiplier;
  m_bInitializerSpawnCountMultiplierChecked = bInitializerMultiplierChecked;
}

void ezParticleSystemInstance::OnOwnerMessage(const ezMessage& msg)
{
  for (ezParticleModuleInstance* pInstance : m_OwnerMessageModules)
  {
    pInstance->m_pModule->OnOwnerMessage(pInstance->m_Context, msg);
  }
}

void ezParticleSystemInstance::CreateModuleInstanceData(const ezParticleSystemDescriptor* pTemplate)
{
  const ezInstanceDataAllocator& allocator = pTemplate->GetInstanceDataAllocator();

  if (allocator.GetTotalDataSize() == 0)
  {
    m_InstanceData.Clear();
  }
  else
  {
    // keeps the allocation, if a system from the free list needs the same size again
    m_InstanceData.SetCountUninitialized(allocator.GetTotalDataSize());
    m_InstanceData.ZeroFill();
  }

  allocator.Construct(m_InstanceData.GetByteBlobPtr());
  m_pInstanceDataTemplate = pTemplate;
}

void ezParticleSystemInstance::DestroyModuleInstanceData()
{
  if (m_pInstanceDataTemplate == nullptr)
    return;

  ForEachModule([](ezParticleModuleInstance& module)
    { module.m_pModule->OnDeinitialize(module.m_Context); });

  m_pInstanceDataTemplate->GetInstanceDataAllocator().Destruct(m_InstanceData.GetByteBlobPtr());
  m_pInstanceDataTemplate = nullptr;
}

void ezParticleSystemInstance::OnElementRemoved(const ezStreamGroupElementRemovedEvent& e)
{
  for (ezParticleModuleInstance* pInstance : m_ParticleDeathModules)
  {
    pInstance->m_pModule->OnParticleDeath(pInstance->m_Context, e.m_uiElementIndex);
  }
}

void ezParticleSystemInstance::CreateStreamProcessors(const ezParticleSystemDescriptor* pTemplate)
{
  // the modules may need the streams for this, so it has to happen before the stream group gets cleared
  DestroyModuleInstanceData();
  CreateModuleInstanceData(pTemplate);

  const ezUInt64 uiMaxParticles = m_StreamGroup.GetNumElements();
  m_StreamGroup.Clear();
  m_pModuleProcessor = nullptr;
  m_StreamGroup.SetSize(uiMaxParticles);
  m_StreamInfo.Clear();

  auto createModules = [&](auto& ref_modules, const auto& modules)
  {
    ref_modules.Clear();

    for (const auto pModule : modules)
    {
      ezParticleModuleInstance& module = ref_modules.ExpandAndGetRef();
      module.m_pModule = pModule;
      module.m_Context.m_pSystem = this;
      module.m_Context.m_pInstanceData = ezInstanceDataAllocator::GetInstanceData(m_InstanceData.GetByteBlobPtr(), pModule->GetInstanceDataOffset());

      pModule->CreateRequiredStreams(module.m_Context);
    }
  };

  createModules(m_Emitters, pTemplate->GetEmitters());
  createModules(m_Initializers, pTemplate->GetInitializers());
  createModules(m_Behaviors, pTemplate->GetBehaviors());
  createModules(m_Finalizers, pTemplate->GetFinalizers());
  createModules(m_Types, pTemplate->GetTypes());

  ezHybridArray<const ezParticleModuleInstance*, 16> instances;
  ForEachModule([&](ezParticleModuleInstance& module)
    { instances.PushBack(&module); });

  m_pModuleProcessor = ezGetStaticRTTI<ezParticleModuleProcessor>()->GetAllocator()->Allocate<ezParticleModuleProcessor>();
  m_pModuleProcessor->SetModules(this, instances);
  m_StreamGroup.AddProcessor(m_pModuleProcessor);
}


void ezParticleSystemInstance::SetupOptionalStreams()
{
  ForEachModule([](ezParticleModuleInstance& module)
    { module.m_pModule->QueryOptionalStreams(module.m_Context); });
}

void ezParticleSystemInstance::SetTransform(const ezTransform& transform, const ezVec3& vParticleStartVelocity)
{
  m_Transform = transform;
  m_vParticleStartVelocity = vParticleStartVelocity;
}

void ezParticleSystemInstance::ReinitializeStreamProcessors(const ezParticleSystemDescriptor* pTemplate)
{
  // all modules have the same type, so the instance data stays valid, even if these are new modules after a resource reload
  EZ_ASSERT_DEBUG(m_pInstanceDataTemplate->GetInstanceDataAllocator().GetTotalDataSize() == pTemplate->GetInstanceDataAllocator().GetTotalDataSize(), "Instance data layout must only depend on the module types");
  m_pInstanceDataTemplate = pTemplate;

  auto reinitializeModules = [](auto& ref_modules, const auto& modules)
  {
    for (ezUInt32 i = 0; i < modules.GetCount(); ++i)
    {
      ref_modules[i].m_pModule = modules[i];
      modules[i]->CreateRequiredStreams(ref_modules[i].m_Context);
    }
  };

  reinitializeModules(m_Emitters, pTemplate->GetEmitters());
  reinitializeModules(m_Initializers, pTemplate->GetInitializers());
  reinitializeModules(m_Behaviors, pTemplate->GetBehaviors());
  reinitializeModules(m_Finalizers, pTemplate->GetFinalizers());
  reinitializeModules(m_Types, pTemplate->GetTypes());
}

ezParticleSystemInstance::ezParticleSystemInstance()
{
  m_BoundingVolume = ezBoundingBoxSphere::MakeInvalid();
  m_StreamGroup.m_ElementRemovedEvent.AddEventHandler(ezMakeDelegate(&ezParticleSystemInstance::OnElementRemoved, this));
}

ezParticleSystemInstance::~ezParticleSystemInstance()
{
  m_StreamGroup.m_ElementRemovedEvent.RemoveEventHandler(ezMakeDelegate(&ezParticleSystemInstance::OnElementRemoved, this));
}

void ezParticleSystemInstance::Construct(ezUInt32 uiMaxParticles, ezWorld* pWorld, ezParticleEffectInstance* pOwnerEffect, float fSpawnCountMultiplier)
{
  m_BoundingVolume = ezBoundingBoxSphere::MakeInvalid();
  m_Transform.SetIdentity();
  m_pOwnerEffect = pOwnerEffect;
  m_bEmitterEnabled = true;
  m_bVisible = true;
  m_pWorld = pWorld;
  m_fSpawnCountMultiplier = fSpawnCountMultiplier;
  m_fInitializerSpawnCountMultiplier = 1.0f;
  m_fRequiredInitializerSpawnCountMultiplier = 1.0f;
  m_bInitializerSpawnCountMultiplierChecked = false;

  m_StreamInfo.Clear();
  m_StreamGroup.SetSize(uiMaxParticles);
}

void ezParticleSystemInstance::Destruct()
{
  DestroyModuleInstanceData();

  m_StreamGroup.Clear();
  m_pModuleProcessor = nullptr;
  m_Emitters.Clear();
  m_Initializers.Clear();
  m_Behaviors.Clear();
  m_Finalizers.Clear();
  m_Types.Clear();
  m_WorldUpdateModules.Clear();
  m_OwnerMessageModules.Clear();
  m_ParticleDeathModules.Clear();

  m_StreamInfo.Clear();
}

ezParticleSystemState::Enum ezParticleSystemInstance::Update(const ezTime& diff)
{
  EZ_PROFILE_SCOPE("PFX: System Update");

  m_UpdateTimeDiff = diff;

  const bool bInitializersReady = PrepareToSpawn();

  if (m_bEmitterEnabled && bInitializersReady)
  {
    // if all emitters are finished, we deactivate this on the whole system
    bool bAllEmittersInactive = true;

    for (const ezParticleModuleInstance& emitter : m_Emitters)
    {
      if (GetEmitter(emitter)->IsFinished(emitter.m_Context) == ezParticleEmitterState::Active)
      {
        bAllEmittersInactive = false;
        const ezUInt32 uiSpawn = GetEmitter(emitter)->ComputeSpawnCount(emitter.m_Context, diff);

        if (uiSpawn > 0)
        {
          EZ_PROFILE_SCOPE("PFX: System Emit");
          m_StreamGroup.InitializeElements(uiSpawn);
        }
      }
    }

    if (bAllEmittersInactive)
    {
      // there is a race condition writing this variable when an effect is used by a ezParticleTypeEffect and should be disabled
      // therefore we must never set this variable to 'true' here, but we can set it to 'false' once we are done
      m_bEmitterEnabled = false;
    }
  }

  bool bHasReactingEmitters = false;

  // always check reactive emitters, as long as there are particles alive, they might produce more
  if (bInitializersReady)
  {
    for (const ezParticleModuleInstance& emitter : m_Emitters)
    {
      if (GetEmitter(emitter)->IsFinished(emitter.m_Context) == ezParticleEmitterState::OnlyReacting)
      {
        bHasReactingEmitters = true;

        const ezUInt32 uiSpawn = GetEmitter(emitter)->ComputeSpawnCount(emitter.m_Context, diff);

        if (uiSpawn > 0)
        {
          EZ_PROFILE_SCOPE("PFX: System Emit (React)");
          m_StreamGroup.InitializeElements(uiSpawn);
        }
      }
    }
  }

  {
    EZ_PROFILE_SCOPE("PFX: System Process");
    m_StreamGroup.Process();
  }

  if (m_bEmitterEnabled)
    return ezParticleSystemState::Active;

  // all emitters are done, but some particles are still alive
  if (HasActiveParticles())
    return ezParticleSystemState::EmittersFinished;

  return bHasReactingEmitters ? ezParticleSystemState::OnlyReacting : ezParticleSystemState::Inactive;
}

ezProcessingStream* ezParticleSystemInstance::QueryStream(ezTempHashedString sName, ezProcessingStream::DataType type) const
{
  return m_StreamGroup.GetStreamByName(sName);
}

void ezParticleSystemInstance::CreateStream(const ezHashedString& sName, ezProcessingStream::DataType type, ezProcessingStream** pStream, bool bWillInitializeElements)
{
  EZ_ASSERT_DEV(pStream != nullptr, "The pointer to the stream pointer must not be null");

  StreamInfo* pInfo = nullptr;

  ezProcessingStream* pSubStream = m_StreamGroup.GetStreamByName(sName);
  if (pSubStream == nullptr)
  {
    pSubStream = m_StreamGroup.AddStream(sName, type);

    pInfo = &m_StreamInfo.ExpandAndGetRef();
    pInfo->m_sName = sName;
  }
  else
  {
    EZ_ASSERT_DEV(pSubStream->GetDataType() == type, "Particle stream '{}' already exists with data type {} (!= {}).", sName, (int)pSubStream->GetDataType(), (int)type);

    for (auto& info : m_StreamInfo)
    {
      if (info.m_sName == sName)
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

  EZ_ASSERT_DEV(pSubStream != nullptr, "Stream creation failed ('{0}')", sName);
  *pStream = pSubStream;
}

void ezParticleSystemInstance::CreateStreamDefaultInitializers()
{
  for (ezUInt32 i = 0; i < m_StreamInfo.GetCount();)
  {
    auto& info = m_StreamInfo[i];

    if ((!info.m_bInUse || info.m_bGetsInitialized) && info.m_bHasDefaultInitializer)
    {
      m_pModuleProcessor->RemoveStreamInitializer(m_StreamGroup.GetStreamByName(info.m_sName));
      info.m_bHasDefaultInitializer = false;
    }

    if (!info.m_bInUse)
    {
      m_StreamGroup.RemoveStreamByName(info.m_sName);
      m_StreamInfo.RemoveAtAndSwap(i);
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

    if (!info.m_bHasDefaultInitializer)
    {
      const ezParticleStream* pParticleStream = GetOwnerWorldModule()->GetParticleStream(info.m_sName);

      if (pParticleStream == nullptr)
      {
        ezLog::Warning("Particle stream '{0}' is zero-initialized.", info.m_sName);
      }

      m_pModuleProcessor->AddStreamInitializer(pParticleStream, m_StreamGroup.GetStreamByName(info.m_sName));
      info.m_bHasDefaultInitializer = true;
    }
  }
}

void ezParticleSystemInstance::ProcessEventQueue(ezParticleEventQueue queue)
{
  for (const ezParticleModuleInstance& emitter : m_Emitters)
  {
    GetEmitter(emitter)->ProcessEventQueue(emitter.m_Context, queue);
  }
}

ezParticleWorldModule* ezParticleSystemInstance::GetOwnerWorldModule() const
{
  return m_pOwnerEffect->GetOwnerWorldModule();
}

void ezParticleSystemInstance::ExtractSystemRenderData(ezMsgExtractRenderData& ref_msg, const ezTransform& instanceTransform) const
{
  for (const ezParticleModuleInstance& type : m_Types)
  {
    GetType(type)->ExtractTypeRenderData(type.m_Context, ref_msg, instanceTransform);
  }
}

void ezParticleSystemInstance::SetBoundingVolume(const ezBoundingBoxSphere& volume, float fMaxParticleSize)
{
  m_BoundingVolume = volume;

  float fExpand = 0;
  for (const ezParticleModuleInstance& type : m_Types)
  {
    fExpand = ezMath::Max(fExpand, GetType(type)->GetMaxParticleRadius(type.m_Context, fMaxParticleSize));
  }

  m_BoundingVolume.m_vBoxHalfExtents += ezVec3(fExpand);
  m_BoundingVolume.m_fSphereRadius += fExpand;
}

bool ezParticleSystemInstance::IsContinuous() const
{
  for (const ezParticleModuleInstance& emitter : m_Emitters)
  {
    if (GetEmitter(emitter)->IsContinuous())
      return true;
  }

  return false;
}
