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
#include <Foundation/Time/Clock.h>

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
  m_UpdateBVolumeTime.SetZero();
  m_LastBVolumeUpdate.SetZero();
  m_BoundingVolume = ezBoundingSphere(ezVec3::ZeroVector(), 0);
  m_ElapsedTimeSinceUpdate.SetZero();
  m_EffectIsVisible.SetZero();
  m_iMinSimStepsToDo = 4;
  m_Transform.SetIdentity();
  m_vVelocity.SetZero();

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

bool ezParticleEffectInstance::IsContinuous() const
{
  for (ezUInt32 i = 0; i < m_ParticleSystems.GetCount(); ++i)
  {
    if (m_ParticleSystems[i]->IsContinuous())
      return true;
  }

  return false;
}

void ezParticleEffectInstance::PreSimulate()
{
  // Pre-simulate the effect, if desired, to get it into a 'good looking' state

  // simulate in large steps to get close
  {
    const ezTime tDiff = ezTime::Seconds(0.5);
    while (m_PreSimulateDuration.GetSeconds() > 10.0)
    {
      StepSimulation(tDiff);
      m_PreSimulateDuration -= tDiff;
    }
  }

  // finer steps
  {
    const ezTime tDiff = ezTime::Seconds(0.2);
    while (m_PreSimulateDuration.GetSeconds() > 5.0)
    {
      StepSimulation(tDiff);
      m_PreSimulateDuration -= tDiff;
    }
  }

  // even finer
  {
    const ezTime tDiff = ezTime::Seconds(0.1);
    while (m_PreSimulateDuration.GetSeconds() >= 0.1)
    {
      StepSimulation(tDiff);
      m_PreSimulateDuration -= tDiff;
    }
  }

  // final step if necessary
  if (m_PreSimulateDuration.GetSeconds() > 0.0)
  {
    StepSimulation(m_PreSimulateDuration);
    m_PreSimulateDuration = ezTime::Seconds(0);
  }
}

void ezParticleEffectInstance::SetIsVisible() const
{
  // if it is visible this frame, also render it the next few frames
  // this has multiple purposes:
  // 1) it fixes the transition when handing off an effect from a
  //    ezParticleComponent to a ezParticleFinisherComponent
  //    though this would only need one frame overlap
  // 2) The bounding volume for culling is only computed every couple of frames
  //    so it may be too small 100ms and culling could be imprecise
  //    by just rendering it the next 100ms, no matter what, the bounding volume
  //    does not need to be updated so frequently
  m_EffectIsVisible = ezClock::GetGlobalClock()->GetAccumulatedTime() + ezTime::Seconds(0.1);
}


bool ezParticleEffectInstance::IsVisible() const
{
  return m_EffectIsVisible >= ezClock::GetGlobalClock()->GetAccumulatedTime();
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

  m_Transform.SetIdentity();
  m_vVelocity.SetZero();
  m_fApplyInstanceVelocity = desc.m_fApplyInstanceVelocity;
  m_bSimulateInLocalSpace = desc.m_bSimulateInLocalSpace;
  m_InvisibleUpdateRate = desc.m_InvisibleUpdateRate;

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

  // parameters
  {
    m_FloatParameters.Clear();
    m_ColorParameters.Clear();

    for (auto it = desc.m_FloatParameters.GetIterator(); it.IsValid(); ++it)
    {
      SetParameter(ezTempHashedString::ComputeHash(it.Key().GetData()), it.Value());
    }

    for (auto it = desc.m_ColorParameters.GetIterator(); it.IsValid(); ++it)
    {
      SetParameter(ezTempHashedString::ComputeHash(it.Key().GetData()), it.Value());
    }
  }

  const ezVec3 vStartVelocity = m_vVelocity * m_fApplyInstanceVelocity;

  for (ezUInt32 i = 0; i < m_ParticleSystems.GetCount(); ++i)
  {
    m_ParticleSystems[i]->ConfigureFromTemplate(systems[i]);
    m_ParticleSystems[i]->SetTransform(m_Transform, vStartVelocity);
    m_ParticleSystems[i]->SetEmitterEnabled(m_bEmitterEnabled);
    m_ParticleSystems[i]->Finalize();
  }
}

bool ezParticleEffectInstance::Update(const ezTime& tDiff)
{
  EZ_PROFILE("PFX: Effect Update");

  ezTime tMinStep = ezTime::Seconds(0);

  if (!IsVisible() && m_iMinSimStepsToDo == 0)
  {
    // shared effects always get paused when they are invisible
    if (IsSharedEffect())
      return true;

    switch (m_InvisibleUpdateRate)
    {
    case ezEffectInvisibleUpdateRate::FullUpdate:
      tMinStep = ezTime::Seconds(1.0 / 60.0);
      break;

    case ezEffectInvisibleUpdateRate::Max20fps:
      tMinStep = ezTime::Milliseconds(50);
      break;

    case ezEffectInvisibleUpdateRate::Max10fps:
      tMinStep = ezTime::Milliseconds(100);
      break;

    case ezEffectInvisibleUpdateRate::Max5fps:
      tMinStep = ezTime::Milliseconds(200);
      break;

    case ezEffectInvisibleUpdateRate::Pause:
      return m_uiReviveTimeout > 0;

    case ezEffectInvisibleUpdateRate::Discard:
      Interrupt();
      return false;
    }
  }

  m_ElapsedTimeSinceUpdate += tDiff;
  PassTransformToSystems();

  // if the time step is too big, iterate multiple times
  {
    const ezTime tMaxTimeStep = ezTime::Milliseconds(200); // in sync with Max5fps
    while (m_ElapsedTimeSinceUpdate > tMaxTimeStep)
    {
      m_ElapsedTimeSinceUpdate -= tMaxTimeStep;

      if (!StepSimulation(tMaxTimeStep))
        return false;
    }
  }

  if (m_ElapsedTimeSinceUpdate < tMinStep)
    return m_uiReviveTimeout > 0;

  // do the remainder
  const ezTime tUpdateDiff = m_ElapsedTimeSinceUpdate;
  m_ElapsedTimeSinceUpdate.SetZero();

  return StepSimulation(tUpdateDiff);
}

bool ezParticleEffectInstance::StepSimulation(const ezTime& tDiff)
{
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
    CombineSystemBoundingVolumes();
  }

  m_iMinSimStepsToDo = ezMath::Max<ezInt8>(m_iMinSimStepsToDo - 1, 0);

  --m_uiReviveTimeout;
  return m_uiReviveTimeout > 0;
}

void ezParticleEffectInstance::SetTransform(const ezTransform& transform, const ezVec3& vParticleStartVelocity,
                                            const void* pSharedInstanceOwner)
{
  if (pSharedInstanceOwner == nullptr)
  {
    m_Transform = transform;
    m_vVelocity = vParticleStartVelocity;
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


void ezParticleEffectInstance::PassTransformToSystems()
{
  if (!m_bSimulateInLocalSpace)
  {
    const ezVec3 vStartVel = m_vVelocity * m_fApplyInstanceVelocity;

    for (ezUInt32 i = 0; i < m_ParticleSystems.GetCount(); ++i)
    {
      if (m_ParticleSystems[i] != nullptr)
      {
        m_ParticleSystems[i]->SetTransform(m_Transform, vStartVel);
      }
    }
  }
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
  return m_UpdateBVolumeTime <= ezClock::GetGlobalClock()->GetAccumulatedTime();
}

void ezParticleEffectInstance::CombineSystemBoundingVolumes()
{
  ezBoundingBoxSphere effectVolume;
  effectVolume.SetInvalid();

  ezBoundingBoxSphere systemVolume;

  for (ezUInt32 i = 0; i < m_ParticleSystems.GetCount(); ++i)
  {
    if (m_ParticleSystems[i])
    {
      systemVolume.SetInvalid();

      m_ParticleSystems[i]->GetBoundingVolume(systemVolume);

      if (systemVolume.IsValid())
      {
        effectVolume.ExpandToInclude(systemVolume);
      }
    }
  }

  if (!effectVolume.IsValid())
  {
    effectVolume = ezBoundingSphere(ezVec3::ZeroVector(), 0.25f);
  }
  else if (!m_bSimulateInLocalSpace)
  {
    // transform the bounding volume to local space, unless it was already created there

    const ezTransform invTrans = GetTransform().GetInverse();
    effectVolume.Transform(invTrans.GetAsMat4());
  }

  const ezTime tNow = ezClock::GetGlobalClock()->GetAccumulatedTime();

  m_BoundingVolume = effectVolume;
  m_LastBVolumeUpdate = tNow;
  m_UpdateBVolumeTime = tNow + ezTime::Seconds(0.1);
}

ezTime ezParticleEffectInstance::GetBoundingVolume(ezBoundingBoxSphere& volume) const
{
  volume = m_BoundingVolume;
  return m_LastBVolumeUpdate;
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

void ezParticleEffectInstance::SetParameter(const ezTempHashedString& name, float value)
{
  for (ezUInt32 i = 0; i < m_FloatParameters.GetCount(); ++i)
  {
    if (m_FloatParameters[i].m_uiNameHash == name.GetHash())
    {
      m_FloatParameters[i].m_fValue = value;
      return;
    }
  }

  auto& ref = m_FloatParameters.ExpandAndGetRef();
  ref.m_uiNameHash = name.GetHash();
  ref.m_fValue = value;
}

void ezParticleEffectInstance::SetParameter(const ezTempHashedString& name, const ezColor& value)
{
  for (ezUInt32 i = 0; i < m_ColorParameters.GetCount(); ++i)
  {
    if (m_ColorParameters[i].m_uiNameHash == name.GetHash())
    {
      m_ColorParameters[i].m_Value = value;
      return;
    }
  }

  auto& ref = m_ColorParameters.ExpandAndGetRef();
  ref.m_uiNameHash = name.GetHash();
  ref.m_Value = value;
}

ezInt32 ezParticleEffectInstance::FindFloatParameter(const ezTempHashedString& name) const
{
  for (ezUInt32 i = 0; i < m_FloatParameters.GetCount(); ++i)
  {
    if (m_FloatParameters[i].m_uiNameHash == name.GetHash())
      return i;
  }

  return -1;
}

float ezParticleEffectInstance::GetFloatParameter(const ezTempHashedString& name, float defaultValue) const
{
  if (name.GetHash() == 0)
    return defaultValue;

  for (ezUInt32 i = 0; i < m_FloatParameters.GetCount(); ++i)
  {
    if (m_FloatParameters[i].m_uiNameHash == name.GetHash())
      return m_FloatParameters[i].m_fValue;
  }

  return defaultValue;
}

ezInt32 ezParticleEffectInstance::FindColorParameter(const ezTempHashedString& name) const
{
  for (ezUInt32 i = 0; i < m_ColorParameters.GetCount(); ++i)
  {
    if (m_ColorParameters[i].m_uiNameHash == name.GetHash())
      return i;
  }

  return -1;
}

const ezColor& ezParticleEffectInstance::GetColorParameter(const ezTempHashedString& name, const ezColor& defaultValue) const
{
  if (name.GetHash() == 0)
    return defaultValue;

  for (ezUInt32 i = 0; i < m_ColorParameters.GetCount(); ++i)
  {
    if (m_ColorParameters[i].m_uiNameHash == name.GetHash())
      return m_ColorParameters[i].m_Value;
  }

  return defaultValue;
}

EZ_STATICLINK_FILE(ParticlePlugin, ParticlePlugin_Effect_ParticleEffectInstance);

