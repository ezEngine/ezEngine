#include <ParticlePlugin/Effect/ParticleEffectController.h>
#include <ParticlePlugin/WorldModule/ParticleWorldModule.h>

ezParticleEffectController::ezParticleEffectController()
{
  m_uiInstanceIdentifier = 0xFFFFFFFF;
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

void ezParticleEffectController::Create(const ezParticleEffectResourceHandle& hEffectResource, ezParticleWorldModule* pModule, ezUInt64 uiRandomSeed, const char* szSharedName, ezUInt32 uiInstanceIdentifier)
{
  if (ezStringUtils::IsNullOrEmpty(szSharedName))
    m_uiInstanceIdentifier = 0xFFFFFFFF;
  else
    m_uiInstanceIdentifier = uiInstanceIdentifier;

  // first get the new effect, to potentially increase a refcount to the same effect instance, before we decrease the refcount of our current one
  ezParticleEffectHandle hNewEffect;
  if (pModule != nullptr && hEffectResource.IsValid())
  {
    hNewEffect = pModule->CreateParticleEffectInstance(hEffectResource, uiRandomSeed, szSharedName, m_uiInstanceIdentifier);
  }

  Invalidate();

  m_hEffect = hNewEffect;

    if (!m_hEffect.IsInvalidated())
      m_pModule = pModule;
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

  // shared effects are always simulated at the origin
  if (pEffect)
  {
    pEffect->SetTransform(m_uiInstanceIdentifier, t);
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
    m_pModule->DestroyParticleEffectInstance(m_hEffect, true, m_uiInstanceIdentifier);

    m_pModule = nullptr;
    m_hEffect.Invalidate();
  }
}

void ezParticleEffectController::Invalidate()
{
  if (m_pModule)
  {
    m_pModule->DestroyParticleEffectInstance(m_hEffect, false, m_uiInstanceIdentifier);

    m_pModule = nullptr;
    m_hEffect.Invalidate();
  }
}

