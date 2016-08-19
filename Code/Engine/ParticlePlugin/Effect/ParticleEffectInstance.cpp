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
  m_Transform.SetIdentity();
  m_bEmitterEnabled = true;
  m_pWorld = nullptr;
}

ezParticleEffectInstance::~ezParticleEffectInstance()
{
  ClearParticleSystems();
}


void ezParticleEffectInstance::SetEmitterEnabled(bool enable)
{
  m_bEmitterEnabled = enable;

  for (ezUInt32 i = 0; i < m_ParticleSystems.GetCount(); ++i)
  {
    m_ParticleSystems[i]->SetEmitterEnabled(m_bEmitterEnabled);
  }
}


bool ezParticleEffectInstance::HasActiveParticles() const
{
  for (ezUInt32 i = 0; i < m_ParticleSystems.GetCount(); ++i)
  {
    if (m_ParticleSystems[i]->HasActiveParticles())
      return true;
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

void ezParticleEffectInstance::Configure(const ezParticleEffectResourceHandle& hResource, ezWorld* pWorld)
{
  m_pWorld = pWorld;
  m_hResource = hResource;

  Reconfigure();
}


void ezParticleEffectInstance::Reconfigure()
{
  ezResourceLock<ezParticleEffectResource> pResource(m_hResource, ezResourceAcquireMode::NoFallback);

  const auto& systems = pResource->GetDescriptor().m_Effect.GetParticleSystems();

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
        m_ParticleSystems[i] = ezParticleEffectManager::GetSingleton()->CreateParticleSystemInstance(systems[i]->m_uiMaxParticles, m_pWorld);
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

void ezParticleEffectInstance::SetTransform(const ezTransform& transform)
{
  m_Transform = transform;

  for (ezUInt32 i = 0; i < m_ParticleSystems.GetCount(); ++i)
  {
    m_ParticleSystems[i]->SetTransform(m_Transform);
  }
}

void ezParticleEffectInstance::Update()
{
  for (ezUInt32 i = 0; i < m_ParticleSystems.GetCount(); ++i)
  {
    m_ParticleSystems[i]->Update();
  }
}
