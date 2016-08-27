#include <ParticlePlugin/Effect/ParticleEffectController.h>
#include <ParticlePlugin/WorldModule/ParticleWorldModule.h>

ezParticleEffectController::ezParticleEffectController()
{
  m_pModule = nullptr;
  m_hEffect.Invalidate();
}

ezParticleEffectController::ezParticleEffectController(const ezParticleEffectController& rhs)
{
  m_pModule = rhs.m_pModule;
  m_hEffect = rhs.m_hEffect;
}

ezParticleEffectController::ezParticleEffectController(ezParticleWorldModule* pModule, ezParticleEffectHandle hEffect)
{
  m_pModule = pModule;
  m_hEffect = hEffect;
}

void ezParticleEffectController::operator=(const ezParticleEffectController& rhs)
{
  m_pModule = rhs.m_pModule;
  m_hEffect = rhs.m_hEffect;
}

ezParticleEffectInstance* ezParticleEffectController::GetInstance() const
{
  if (m_pModule == nullptr)
    return nullptr;

  ezParticleEffectInstance* pEffect = nullptr;
  m_pModule->TryGetEffect(m_hEffect, pEffect);
  return pEffect;
}

void ezParticleEffectController::Create(const ezParticleEffectResourceHandle& hEffectResource, ezParticleWorldModule* pModule, ezUInt64 uiRandomSeed)
{
  Invalidate();

  if (pModule != nullptr && hEffectResource.IsValid())
  {
    m_hEffect = pModule->CreateParticleEffectInstance(hEffectResource, uiRandomSeed);

    if (!m_hEffect.IsInvalidated())
      m_pModule = pModule;
  }
}

bool ezParticleEffectController::IsValid() const
{
  return (m_pModule != nullptr && !m_hEffect.IsInvalidated());
}

bool ezParticleEffectController::IsAlive() const
{
  ezParticleEffectInstance* pEffect = GetInstance();
  return pEffect != nullptr;
}

void ezParticleEffectController::SetTransform(const ezTransform& t) const
{
  ezParticleEffectInstance* pEffect = GetInstance();

  if (pEffect)
  {
    pEffect->SetTransform(t);
  }
}

void ezParticleEffectController::Tick(const ezTime& tDiff) const
{
  ezParticleEffectInstance* pEffect = GetInstance();

  if (pEffect)
  {
    pEffect->Update(tDiff);
  }
}

void ezParticleEffectController::StopImmediate()
{
  if (m_pModule)
  {
    m_pModule->DestroyParticleEffectInstance(m_hEffect, true);

    m_pModule = nullptr;
    m_hEffect.Invalidate();
  }
}

void ezParticleEffectController::Invalidate()
{
  if (m_pModule)
  {
    m_pModule->DestroyParticleEffectInstance(m_hEffect, false);

    m_pModule = nullptr;
    m_hEffect.Invalidate();
  }
}

