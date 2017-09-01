#include <PCH.h>
#include <ParticlePlugin/Effect/ParticleEffectInstance.h>
#include <ParticlePlugin/System/ParticleSystemInstance.h>
#include <ParticlePlugin/Effect/ParticleEffectDescriptor.h>
#include <ParticlePlugin/Resources/ParticleEffectResource.h>
#include <ParticlePlugin/System/ParticleSystemDescriptor.h>
#include <Core/ResourceManager/ResourceManager.h>
#include <ParticlePlugin/WorldModule/ParticleWorldModule.h>
#include <RendererCore/RenderWorld/RenderWorld.h>
#include <Foundation/Profiling/Profiling.h>

ezParticleEffectInstance::ezParticleEffectInstance()
  : m_Task(this)
{
  m_pOwnerModule = nullptr;
  m_Task.SetTaskName("Particle Effect Update");

  Destruct();
}

ezParticleEffectInstance::~ezParticleEffectInstance()
{
  Destruct();
}

void ezParticleEffectInstance::Construct(ezParticleEffectHandle hEffectHandle, const ezParticleEffectResourceHandle& hResource, ezWorld* pWorld, ezParticleWorldModule* pOwnerModule, ezUInt64 uiRandomSeed, bool bIsShared)
{
  m_hEffectHandle = hEffectHandle;
  m_pWorld = pWorld;
  m_pOwnerModule = pOwnerModule;
  m_hResource = hResource;
  m_bIsSharedEffect = bIsShared;
  m_bEmitterEnabled = true;
  m_bIsFinishing = false;
  m_uiUpdateBVolumeCounter = 255;
  m_BoundingVolume = ezBoundingSphere(ezVec3::ZeroVector(), 0);

  Reconfigure(uiRandomSeed, true);
}

void ezParticleEffectInstance::Destruct()
{
  Interrupt();

  m_SharedInstances.Clear();
  m_hEffectHandle.Invalidate();

  m_Transform.SetIdentity();
  m_bIsSharedEffect = false;
  m_pWorld = nullptr;
  m_hResource.Invalidate();
  m_hEffectHandle.Invalidate();
  m_uiReviveTimeout = 5;
}

void ezParticleEffectInstance::Interrupt()
{
  ClearParticleSystems();
  DestroyEventQueues();
  m_bEmitterEnabled = false;
}

void ezParticleEffectInstance::SetEmitterEnabled(bool enable)
{
  m_bEmitterEnabled = enable;

  for (ezUInt32 i = 0; i < m_ParticleSystems.GetCount(); ++i)
  {
    if (m_ParticleSystems[i])
    {
      m_ParticleSystems[i]->SetEmitterEnabled(m_bEmitterEnabled);
    }
  }
}


bool ezParticleEffectInstance::HasActiveParticles() const
{
  for (ezUInt32 i = 0; i < m_ParticleSystems.GetCount(); ++i)
  {
    if (m_ParticleSystems[i])
    {
      if (m_ParticleSystems[i]->HasActiveParticles())
        return true;
    }
  }

  return false;
}


void ezParticleEffectInstance::ClearParticleSystem(ezUInt32 index)
{
  if (m_ParticleSystems[index])
  {
    m_pOwnerModule->DestroySystemInstance(m_ParticleSystems[index]);
    m_ParticleSystems[index] = nullptr;
  }
}

void ezParticleEffectInstance::ClearParticleSystems()
{
  for (ezUInt32 i = 0; i < m_ParticleSystems.GetCount(); ++i)
  {
    ClearParticleSystem(i);
  }

  m_ParticleSystems.Clear();
}

void ezParticleEffectInstance::PreSimulate()
{
  // Pre-simulate the effect, if desired, to get it into a 'good looking' state

  // simulate in large steps to get close
  {
    const ezTime tDiff = ezTime::Seconds(0.5);
    while (m_PreSimulateDuration.GetSeconds() > 5.0)
    {
      Update(tDiff);
      m_PreSimulateDuration -= tDiff;
    }
  }

  // finer steps
  {
    const ezTime tDiff = ezTime::Seconds(0.2);
    while (m_PreSimulateDuration.GetSeconds() > 1.0)
    {
      Update(tDiff);
      m_PreSimulateDuration -= tDiff;
    }
  }

  // even finer
  {
    const ezTime tDiff = ezTime::Seconds(0.1);
    while (m_PreSimulateDuration.GetSeconds() >= 0.1)
    {
      Update(tDiff);
      m_PreSimulateDuration -= tDiff;
    }
  }

  // final step if necessary
  if (m_PreSimulateDuration.GetSeconds() > 0.0)
  {
    Update(m_PreSimulateDuration);
    m_PreSimulateDuration = ezTime::Seconds(0);
  }
}

void ezParticleEffectInstance::SetIsInView() const
{
  // if it is visible this frame, also render it the next few frames
  // this has multiple purposes:
  // 1) it fixes the transition when handing off an effect from a
  //    ezParticleComponent to a ezParticleFinisherComponent
  //    though this would only need one frame overlap
  // 2) The bounding volume for culling is only computed every 8 updates
  //    so it may be too small for 7 frames and culling could be imprecise
  //    by just rendering it the next 8 frames, no matter what, the bounding volume
  //    does not need to be updated so frequently
  m_uiEffectIsInView = ezRenderWorld::GetFrameCounter() + 8;
}

void ezParticleEffectInstance::Reconfigure(ezUInt64 uiRandomSeed, bool bFirstTime)
{
  if (!m_hResource.IsValid())
  {
    ezLog::Error("Effect Reconfigure: Effect Resource is invalid");
    return;
  }

  ezResourceLock<ezParticleEffectResource> pResource(m_hResource, ezResourceAcquireMode::NoFallback);

  const auto& desc = pResource->GetDescriptor().m_Effect;
  const auto& systems = desc.GetParticleSystems();

  m_bSimulateInLocalSpace = desc.m_bSimulateInLocalSpace;

  if (bFirstTime)
  {
    m_PreSimulateDuration = desc.m_PreSimulateDuration;
  }

  // TODO Check max number of particles etc. to reset

  if (m_ParticleSystems.GetCount() != systems.GetCount())
  {
    // reset everything
    ClearParticleSystems();
  }

  m_ParticleSystems.SetCount(systems.GetCount());

  // delete all that have important changes
  {
    for (ezUInt32 i = 0; i < m_ParticleSystems.GetCount(); ++i)
    {
      if (m_ParticleSystems[i] != nullptr)
      {
        if (m_ParticleSystems[i]->GetMaxParticles() != systems[i]->m_uiMaxParticles)
          ClearParticleSystem(i);
      }
    }
  }

  // recreate where necessary
  {
    for (ezUInt32 i = 0; i < m_ParticleSystems.GetCount(); ++i)
    {
      if (m_ParticleSystems[i] == nullptr)
      {
        m_ParticleSystems[i] = m_pOwnerModule->CreateSystemInstance(systems[i]->m_uiMaxParticles, m_pWorld, uiRandomSeed, this);
      }
    }
  }

  for (ezUInt32 i = 0; i < m_ParticleSystems.GetCount(); ++i)
  {
    m_ParticleSystems[i]->ConfigureFromTemplate(systems[i]);
    m_ParticleSystems[i]->SetTransform(m_Transform);
    m_ParticleSystems[i]->SetEmitterEnabled(m_bEmitterEnabled);
  }
}

bool ezParticleEffectInstance::Update(const ezTime& tDiff)
{
  EZ_PROFILE("PFX: Effect Update");
  ++m_uiUpdateBVolumeCounter;

  for (ezUInt32 i = 0; i < m_ParticleSystems.GetCount(); ++i)
  {
    if (m_ParticleSystems[i] != nullptr)
    {
      auto state = m_ParticleSystems[i]->Update(tDiff);

      if (state == ezParticleSystemState::Inactive)
      {
        ClearParticleSystem(i);
      }
      else if (state != ezParticleSystemState::OnlyReacting)
      {
        // this is used to delay particle effect death by a couple of frames
        // that way, if an event is in the pipeline that might trigger a reacting emitter,
        // or particles are in the spawn queue, but not yet created, we don't kill the effect too early
        m_uiReviveTimeout = 3;
      }
    }
  }

  ProcessEventQueues();

  if (NeedsBoundingVolumeUpdate())
  {
    EZ_PROFILE("PFX: BVol Update");
    CombineSystemBoundingVolumes();
    m_uiUpdateBVolumeCounter = 0;
  }

  --m_uiReviveTimeout;
  return m_uiReviveTimeout > 0;
}

void ezParticleEffectInstance::SetTransform(const ezTransform& transform, const void* pSharedInstanceOwner)
{
  if (pSharedInstanceOwner == nullptr)
  {
    m_Transform = transform;

    if (!m_bSimulateInLocalSpace)
    {
      for (ezUInt32 i = 0; i < m_ParticleSystems.GetCount(); ++i)
      {
        if (m_ParticleSystems[i] != nullptr)
        {
          m_ParticleSystems[i]->SetTransform(m_Transform);
        }
      }
    }
  }
  else
  {
    for (auto& info : m_SharedInstances)
    {
      if (info.m_pSharedInstanceOwner == pSharedInstanceOwner)
      {
        info.m_Transform = transform;
        return;
      }
    }
  }
}

const ezTransform& ezParticleEffectInstance::GetTransform(const void* pSharedInstanceOwner) const
{
  if (pSharedInstanceOwner == nullptr)
    return m_Transform;

  for (auto& info : m_SharedInstances)
  {
    if (info.m_pSharedInstanceOwner == pSharedInstanceOwner)
    {
      return info.m_Transform;
    }
  }

  return m_Transform;
}

void ezParticleEffectInstance::AddSharedInstance(const void* pSharedInstanceOwner)
{
  for (auto& info : m_SharedInstances)
  {
    if (info.m_pSharedInstanceOwner == pSharedInstanceOwner)
      return;
  }

  auto& info = m_SharedInstances.ExpandAndGetRef();
  info.m_pSharedInstanceOwner = pSharedInstanceOwner;
  info.m_Transform.SetIdentity();
}

void ezParticleEffectInstance::RemoveSharedInstance(const void* pSharedInstanceOwner)
{
  for (ezUInt32 i = 0; i < m_SharedInstances.GetCount(); ++i)
  {
    if (m_SharedInstances[i].m_pSharedInstanceOwner == pSharedInstanceOwner)
    {
      m_SharedInstances.RemoveAtSwap(i);
      return;
    }
  }
}


ezParticleEventQueue* ezParticleEffectInstance::GetEventQueue(const ezTempHashedString& EventType)
{
  for (ezUInt32 i = 0; i < m_EventQueues.GetCount(); ++i)
  {
    if (m_EventQueues[i].m_EventTypeHash == EventType.GetHash())
      return m_EventQueues[i].m_pQueue;
  }

  auto& queue = m_EventQueues.ExpandAndGetRef();

  queue.m_pQueue = m_pOwnerModule->GetEventQueueManager().CreateEventQueue(EventType.GetHash());
  queue.m_EventTypeHash = EventType.GetHash();

  return queue.m_pQueue;
}


bool ezParticleEffectInstance::ShouldBeUpdated() const
{
  if (m_hEffectHandle.IsInvalidated())
    return false;

  // do not update shared instances when there is no one watching
  if (m_bIsSharedEffect && m_SharedInstances.GetCount() == 0)
    return false;

  return true;
}


bool ezParticleEffectInstance::NeedsBoundingVolumeUpdate() const
{
  return m_uiUpdateBVolumeCounter > 7;
}

void ezParticleEffectInstance::CombineSystemBoundingVolumes()
{
  ezBoundingBoxSphere effectVolume;
  effectVolume.SetInvalid();
  float effectMaxSize = 0;

  ezBoundingBoxSphere systemVolume;
  float systemMaxSize = 0;

  for (ezUInt32 i = 0; i < m_ParticleSystems.GetCount(); ++i)
  {
    if (m_ParticleSystems[i])
    {
      systemVolume.SetInvalid();
      systemMaxSize = 0.0f;

      m_ParticleSystems[i]->GetBoundingVolume(systemVolume, systemMaxSize);

      if (systemVolume.IsValid())
      {
        effectVolume.ExpandToInclude(systemVolume);
        effectMaxSize = ezMath::Max(effectMaxSize, systemMaxSize);
      }
    }
  }

  if (effectVolume.IsValid())
  {
    effectVolume.m_fSphereRadius += effectMaxSize;
    effectVolume.m_vBoxHalfExtends += ezVec3(effectMaxSize);
  }
  else
  {
    effectVolume = ezBoundingSphere(ezVec3::ZeroVector(), 0.25f);
  }

  m_BoundingVolume = effectVolume;
  m_uiLastBVolumeUpdate = ezRenderWorld::GetFrameCounter();
}

ezUInt64 ezParticleEffectInstance::GetBoundingVolume(ezBoundingBoxSphere& volume)
{
  volume = m_BoundingVolume;
  return m_uiLastBVolumeUpdate;
}

void ezParticleEffectInstance::DestroyEventQueues()
{
  for (auto& queue : m_EventQueues)
  {
    m_pOwnerModule->GetEventQueueManager().DestroyEventQueue(queue.m_pQueue);
  }

  m_EventQueues.Clear();
}

void ezParticleEffectInstance::ProcessEventQueues()
{
  EZ_PROFILE("PFX: Effect Event Queue");
  for (ezUInt32 i = 0; i < m_ParticleSystems.GetCount(); ++i)
  {
    if (m_ParticleSystems[i])
    {
      for (const auto& queue : m_EventQueues)
      {
        m_ParticleSystems[i]->ProcessEventQueue(queue.m_pQueue);
      }
    }
  }

  for (auto& queue : m_EventQueues)
  {
    queue.m_pQueue->Clear();
  }
}

ezParticleffectUpdateTask::ezParticleffectUpdateTask(ezParticleEffectInstance* pEffect)
{
  m_pEffect = pEffect;
  m_UpdateDiff.SetZero();
}

void ezParticleffectUpdateTask::Execute()
{
  if (HasBeenCanceled())
    return;

  if (m_UpdateDiff.GetSeconds() != 0.0)
  {
    m_pEffect->PreSimulate();

    if (!m_pEffect->Update(m_UpdateDiff))
    {
      const ezParticleEffectHandle hEffect = m_pEffect->GetHandle();
      EZ_ASSERT_DEBUG(!hEffect.IsInvalidated(), "Invalid particle effect handle");

      m_pEffect->GetOwnerWorldModule()->DestroyEffectInstance(hEffect, false, nullptr);
    }
  }
}



EZ_STATICLINK_FILE(ParticlePlugin, ParticlePlugin_Effect_ParticleEffectInstance);

