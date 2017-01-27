#include <ParticlePlugin/PCH.h>
#include <ParticlePlugin/Behavior/ParticleBehavior_ColorGradient.h>
#include <CoreUtils/DataProcessing/Stream/ProcessingStreamIterator.h>
#include <Core/WorldSerializer/ResourceHandleReader.h>
#include <Core/WorldSerializer/ResourceHandleWriter.h>

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezParticleBehaviorFactory_ColorGradient, 1, ezRTTIDefaultAllocator<ezParticleBehaviorFactory_ColorGradient>)
{
  EZ_BEGIN_PROPERTIES
  {
    EZ_ACCESSOR_PROPERTY("Gradient", GetColorGradientFile, SetColorGradientFile)->AddAttributes(new ezAssetBrowserAttribute("ColorGradient")),
  }
  EZ_END_PROPERTIES
}
EZ_END_DYNAMIC_REFLECTED_TYPE

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezParticleBehavior_ColorGradient, 1, ezRTTIDefaultAllocator<ezParticleBehavior_ColorGradient>)
EZ_END_DYNAMIC_REFLECTED_TYPE

const ezRTTI* ezParticleBehaviorFactory_ColorGradient::GetBehaviorType() const
{
  return ezGetStaticRTTI<ezParticleBehavior_ColorGradient>();
}

void ezParticleBehaviorFactory_ColorGradient::CopyBehaviorProperties(ezParticleBehavior* pObject) const
{
  ezParticleBehavior_ColorGradient* pBehavior = static_cast<ezParticleBehavior_ColorGradient*>(pObject);

  pBehavior->m_hGradient = m_hGradient;
}

void ezParticleBehaviorFactory_ColorGradient::Save(ezStreamWriter& stream) const
{
  const ezUInt8 uiVersion = 2;
  stream << uiVersion;

  stream << m_hGradient;
}

void ezParticleBehaviorFactory_ColorGradient::Load(ezStreamReader& stream)
{
  ezUInt8 uiVersion = 0;
  stream >> uiVersion;

  stream >> m_hGradient;
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

void ezParticleBehavior_ColorGradient::CreateRequiredStreams()
{
  CreateStream("LifeTime", ezProcessingStream::DataType::Float2, &m_pStreamLifeTime, false);
  CreateStream("Color", ezProcessingStream::DataType::Float4, &m_pStreamColor, false);
}

void ezParticleBehavior_ColorGradient::Process(ezUInt64 uiNumElements)
{
  if (!m_hGradient.IsValid())
    return;

  ezProcessingStreamIterator<ezVec3> itLifeTime(m_pStreamLifeTime, uiNumElements, 0);
  ezProcessingStreamIterator<ezColor> itColor(m_pStreamColor, uiNumElements, 0);

  ezResourceLock<ezColorGradientResource> pGradient(m_hGradient, ezResourceAcquireMode::NoFallback);

  if (pGradient->IsMissingResource())
    return;

  while (!itLifeTime.HasReachedEnd())
  {
    if (itLifeTime.Current().y > 0)
    {
      const float fLifeTimeFraction = itLifeTime.Current().x / itLifeTime.Current().y;

      ezColorGammaUB rgba;
      float intensity;
      pGradient->GetDescriptor().m_Gradient.Evaluate(1.0f - fLifeTimeFraction, rgba, intensity);

      itColor.Current() = rgba;
    }

    itLifeTime.Advance();
    itColor.Advance();
  }
}

