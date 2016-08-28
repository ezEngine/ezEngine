#include <ParticlePlugin/PCH.h>
#include <ParticlePlugin/Effect/ParticleEffectInstance.h>
#include <ParticlePlugin/System/ParticleSystemInstance.h>
#include <ParticlePlugin/Effect/ParticleEffectDescriptor.h>
#include <ParticlePlugin/Manager/ParticleEffectManager.h>
#include <ParticlePlugin/Resources/ParticleEffectResource.h>
#include <ParticlePlugin/System/ParticleSystemDescriptor.h>
#include <Core/ResourceManager/ResourceManager.h>

ezParticleEffectInstance::ezParticleEffectInstance()
{
  Clear();
}

ezParticleEffectInstance::~ezParticleEffectInstance()
{
  ClearParticleSystems();
}


void ezParticleEffectInstance::Clear()
{
  Interrupt();

  m_Transform.SetIdentity();
  m_bEmitterEnabled = true;
  m_bIsShared = false;
  m_pWorld = nullptr;
  m_hResource.Invalidate();
  m_hHandle.Invalidate();
}


void ezParticleEffectInstance::Interrupt()
{
  ClearParticleSystems();
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
    ezParticleEffectManager::GetSingleton()->DestroyParticleSystemInstance(m_ParticleSystems[index]);
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

void ezParticleEffectInstance::Configure(const ezParticleEffectResourceHandle& hResource, ezWorld* pWorld, ezUInt64 uiRandomSeed, bool bIsShared)
{
  m_pWorld = pWorld;
  m_hResource = hResource;
  m_bIsShared = bIsShared;

  Reconfigure(uiRandomSeed, true);
}


void ezParticleEffectInstance::PreSimulate()
{
  // Pre-simulate the effect, if desired, to get it into a 'good looking' state

  {
    ezTime tDiff = ezTime::Seconds(0.5);
    while (m_PreSimulateDuration.GetSeconds() > 5.0)
    {
      Update(tDiff);
      m_PreSimulateDuration -= tDiff;
    }
  }

  {
    ezTime tDiff = ezTime::Seconds(0.2);
    while (m_PreSimulateDuration.GetSeconds() > 1.0)
    {
      Update(tDiff);
      m_PreSimulateDuration -= tDiff;
    }
  }

  {
    ezTime tDiff = ezTime::Seconds(0.1);
    while (m_PreSimulateDuration.GetSeconds() >= 0.1)
    {
      Update(tDiff);
      m_PreSimulateDuration -= tDiff;
    }
  }
}

void ezParticleEffectInstance::Reconfigure(ezUInt64 uiRandomSeed, bool bFirstTime)
{
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
        m_ParticleSystems[i] = ezParticleEffectManager::GetSingleton()->CreateParticleSystemInstance(systems[i]->m_uiMaxParticles, m_pWorld, uiRandomSeed);
      }
    }
  }

  for (ezUInt32 i = 0; i < m_ParticleSystems.GetCount(); ++i)
  {
    EZ_LOCK(m_ParticleSystems[i]->m_Mutex);

    m_ParticleSystems[i]->ConfigureFromTemplate(systems[i]);
    m_ParticleSystems[i]->SetTransform(m_Transform);
    m_ParticleSystems[i]->SetEmitterEnabled(m_bEmitterEnabled);
  }
}

bool ezParticleEffectInstance::Update(const ezTime& tDiff)
{
  bool bAnyActive = false;

  for (ezUInt32 i = 0; i < m_ParticleSystems.GetCount(); ++i)
  {
    if (m_ParticleSystems[i] != nullptr)
    {
      bAnyActive = true;

      if (m_ParticleSystems[i]->Update(tDiff) == ezParticleSystemState::Inactive)
      {
        ClearParticleSystem(i);
      }
    }
  }

  return bAnyActive;
}

void ezParticleEffectInstance::SetTransform(ezUInt32 uiSharedInstanceIdentifier, const ezTransform& transform)
{
  if (uiSharedInstanceIdentifier == 0xFFFFFFFF)
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

    return;
  }

  for (auto& info : m_SharedInstances)
  {
    if (info.m_uiIdentifier == uiSharedInstanceIdentifier)
    {
      info.m_Transform = transform;
      return;
    }
  }
}

const ezTransform& ezParticleEffectInstance::GetTransform(ezUInt32 uiSharedInstanceIdentifier) const
{
  if (uiSharedInstanceIdentifier == 0xFFFFFFFF)
    return m_Transform;

  for (auto& info : m_SharedInstances)
  {
    if (info.m_uiIdentifier == uiSharedInstanceIdentifier)
    {
      return info.m_Transform;
    }
  }

  return m_Transform;
}

void ezParticleEffectInstance::AddSharedInstance(ezUInt32 uiSharedInstanceIdentifier)
{
  for (auto& info : m_SharedInstances)
  {
    if (info.m_uiIdentifier == uiSharedInstanceIdentifier)
      return;
  }

  auto& info = m_SharedInstances.ExpandAndGetRef();
  info.m_uiIdentifier = uiSharedInstanceIdentifier;
  info.m_Transform.SetIdentity();
}

void ezParticleEffectInstance::RemoveSharedInstance(ezUInt32 uiSharedInstanceIdentifier)
{
  for (ezUInt32 i = 0; i < m_SharedInstances.GetCount(); ++i)
  {
    if (m_SharedInstances[i].m_uiIdentifier == uiSharedInstanceIdentifier)
    {
      m_SharedInstances.RemoveAtSwap(i);
      return;
    }
  }
}
