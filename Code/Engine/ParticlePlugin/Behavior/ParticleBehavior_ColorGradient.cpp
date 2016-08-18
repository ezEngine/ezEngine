#include <ParticlePlugin/PCH.h>
#include <ParticlePlugin/Behavior/ParticleBehavior_ColorGradient.h>
#include <CoreUtils/DataProcessing/Stream/StreamElementIterator.h>

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezParticleBehaviorFactory_ColorGradient, 1, ezRTTIDefaultAllocator<ezParticleBehaviorFactory_ColorGradient>)
{
  EZ_BEGIN_PROPERTIES
  {
    EZ_ACCESSOR_PROPERTY("Gradient", GetColorGradientFile, SetColorGradientFile)->AddAttributes(new ezAssetBrowserAttribute("ColorGradient")),
  }
  EZ_END_PROPERTIES
}
EZ_END_DYNAMIC_REFLECTED_TYPE

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezParticleBehavior_ColorGradient, 1, ezRTTINoAllocator)
EZ_END_DYNAMIC_REFLECTED_TYPE

ezParticleBehaviorFactory_ColorGradient::ezParticleBehaviorFactory_ColorGradient()
{

}

ezParticleBehavior* ezParticleBehaviorFactory_ColorGradient::CreateBehavior(ezParticleSystemInstance* pOwner) const
{
  ezParticleBehavior_ColorGradient* pBehavior = EZ_DEFAULT_NEW(ezParticleBehavior_ColorGradient, pOwner);

  // Copy Properties
  {
    pBehavior->m_hGradient = m_hGradient;

  }

  return pBehavior;
}

void ezParticleBehaviorFactory_ColorGradient::Save(ezStreamWriter& stream) const
{
  const ezUInt8 uiVersion = 1;
  stream << uiVersion;

  stream << GetColorGradientFile();
}

void ezParticleBehaviorFactory_ColorGradient::Load(ezStreamReader& stream)
{
  ezUInt8 uiVersion = 0;
  stream >> uiVersion;

  ezStringBuilder sGradient;
  stream >> sGradient;
  SetColorGradientFile(sGradient);
}

void ezParticleBehaviorFactory_ColorGradient::SetColorGradientFile(const char* szFile)
{
  ezColorGradientResourceHandle m_hGradient;

  if (!ezStringUtils::IsNullOrEmpty(szFile))
  {
    m_hGradient = ezResourceManager::LoadResource<ezColorGradientResource>(szFile);
  }

  SetColorGradient(m_hGradient);
}


const char* ezParticleBehaviorFactory_ColorGradient::GetColorGradientFile() const
{
  if (!m_hGradient.IsValid())
    return "";

  return m_hGradient.GetResourceID();
}

ezParticleBehavior_ColorGradient::ezParticleBehavior_ColorGradient(ezParticleSystemInstance* pOwner)
  : ezParticleBehavior(pOwner)
{

}

void ezParticleBehavior_ColorGradient::Process(ezUInt64 uiNumElements)
{
  if (!m_hGradient.IsValid())
    return;

  ezStreamElementIterator<ezVec3> itLifeTime(m_pStreamLifeTime, uiNumElements);
  ezStreamElementIterator<ezColor> itColor(m_pStreamColor, uiNumElements);

  ezResourceLock<ezColorGradientResource> pGradient(m_hGradient, ezResourceAcquireMode::NoFallback);

  if (pGradient->IsMissingResource())
    return;

  while (!itLifeTime.HasReachedEnd())
  {
    const float fLifeTimeFraction = itLifeTime.Current().x / itLifeTime.Current().y;

    ezColorGammaUB rgba;
    float intensity;
    pGradient->GetDescriptor().m_Gradient.Evaluate(1.0f - fLifeTimeFraction, rgba, intensity);

    itColor.Current() = rgba;

    itLifeTime.Advance();
    itColor.Advance();
  }
}

void ezParticleBehavior_ColorGradient::StepParticleSystem()
{
}
