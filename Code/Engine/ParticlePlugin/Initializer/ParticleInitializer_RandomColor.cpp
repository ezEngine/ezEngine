#include <PCH.h>
#include <ParticlePlugin/Initializer/ParticleInitializer_RandomColor.h>
#include <Foundation/DataProcessing/Stream/ProcessingStreamGroup.h>
#include <Foundation/Math/Random.h>
#include <ParticlePlugin/System/ParticleSystemInstance.h>
#include <GameEngine/Curves/ColorGradientResource.h>
#include <Core/WorldSerializer/ResourceHandleReader.h>
#include <Core/WorldSerializer/ResourceHandleWriter.h>
#include <Foundation/Profiling/Profiling.h>

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezParticleInitializerFactory_RandomColor, 1, ezRTTIDefaultAllocator<ezParticleInitializerFactory_RandomColor>)
{
  EZ_BEGIN_PROPERTIES
  {
    EZ_ACCESSOR_PROPERTY("Gradient", GetColorGradientFile, SetColorGradientFile)->AddAttributes(new ezAssetBrowserAttribute("ColorGradient")),
    EZ_MEMBER_PROPERTY("Color1", m_Color1)->AddAttributes(new ezDefaultValueAttribute(ezColor::White)),
    EZ_MEMBER_PROPERTY("Color2", m_Color2)->AddAttributes(new ezDefaultValueAttribute(ezColor::White)),
  }
  EZ_END_PROPERTIES;
}
EZ_END_DYNAMIC_REFLECTED_TYPE;

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezParticleInitializer_RandomColor, 1, ezRTTIDefaultAllocator<ezParticleInitializer_RandomColor>)
EZ_END_DYNAMIC_REFLECTED_TYPE;

const ezRTTI* ezParticleInitializerFactory_RandomColor::GetInitializerType() const
{
  return ezGetStaticRTTI<ezParticleInitializer_RandomColor>();
}

void ezParticleInitializerFactory_RandomColor::CopyInitializerProperties(ezParticleInitializer* pInitializer0) const
{
  ezParticleInitializer_RandomColor* pInitializer = static_cast<ezParticleInitializer_RandomColor*>(pInitializer0);

  pInitializer->m_hGradient = m_hGradient;
  pInitializer->m_Color1 = m_Color1;
  pInitializer->m_Color2 = m_Color2;
}

void ezParticleInitializerFactory_RandomColor::SetColorGradientFile(const char* szFile)
{
  ezColorGradientResourceHandle m_hGradient;

  if (!ezStringUtils::IsNullOrEmpty(szFile))
  {
    m_hGradient = ezResourceManager::LoadResource<ezColorGradientResource>(szFile);
  }

  SetColorGradient(m_hGradient);
}


const char* ezParticleInitializerFactory_RandomColor::GetColorGradientFile() const
{
  if (!m_hGradient.IsValid())
    return "";

  return m_hGradient.GetResourceID();
}

void ezParticleInitializerFactory_RandomColor::Save(ezStreamWriter& stream) const
{
  const ezUInt8 uiVersion = 1;
  stream << uiVersion;

  stream << m_hGradient;
  stream << m_Color1;
  stream << m_Color2;
}

void ezParticleInitializerFactory_RandomColor::Load(ezStreamReader& stream)
{
  ezUInt8 uiVersion = 0;
  stream >> uiVersion;

  stream >> m_hGradient;
  stream >> m_Color1;
  stream >> m_Color2;
}


void ezParticleInitializer_RandomColor::CreateRequiredStreams()
{
  CreateStream("Color", ezProcessingStream::DataType::Float4, &m_pStreamColor, true);
}

void ezParticleInitializer_RandomColor::InitializeElements(ezUInt64 uiStartIndex, ezUInt64 uiNumElements)
{
  EZ_PROFILE("PFX: Random Color");

  ezColor* pColor = m_pStreamColor->GetWritableData<ezColor>();

  ezRandom& rng = GetRNG();

  if (!m_hGradient.IsValid())
  {
    for (ezUInt64 i = uiStartIndex; i < uiStartIndex + uiNumElements; ++i)
    {
      const float f = (float)rng.DoubleZeroToOneInclusive();

      pColor[i] = ezMath::Lerp(m_Color1, m_Color2, f);
    }
  }
  else
  {
    ezResourceLock<ezColorGradientResource> pResource(m_hGradient, ezResourceAcquireMode::NoFallback);

    double fMinValue, fMaxValue;
    const ezColorGradient& gradient = pResource->GetDescriptor().m_Gradient;
    gradient.GetExtents(fMinValue, fMaxValue);

    ezColorGammaUB color;
    float intensity;

    for (ezUInt64 i = uiStartIndex; i < uiStartIndex + uiNumElements; ++i)
    {
      const double f = rng.DoubleMinMax(fMinValue, fMaxValue);

      gradient.Evaluate(f, color, intensity);

      ezColor result = color;
      result.r *= intensity;
      result.g *= intensity;
      result.b *= intensity;

      pColor[i] = result;
    }
  }
}



EZ_STATICLINK_FILE(ParticlePlugin, ParticlePlugin_Initializer_ParticleInitializer_RandomColor);

