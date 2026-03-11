#include <ParticlePlugin/ParticlePluginPCH.h>

#include <Core/Curves/ColorGradientResource.h>
#include <Foundation/DataProcessing/Stream/ProcessingStreamGroup.h>
#include <Foundation/Math/Color16f.h>
#include <Foundation/Math/Random.h>
#include <Foundation/Profiling/Profiling.h>
#include <ParticlePlugin/Initializer/ParticleInitializer_RandomColor.h>
#include <ParticlePlugin/System/ParticleSystemInstance.h>

// clang-format off
EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezParticleInitializerFactory_RandomColor, 3, ezRTTIDefaultAllocator<ezParticleInitializerFactory_RandomColor>)
{
  EZ_BEGIN_PROPERTIES
  {
    EZ_ENUM_MEMBER_PROPERTY("GradientSource", ezGradientSource, m_GradientSource),
    EZ_MEMBER_PROPERTY("Gradient", m_Gradient),
    EZ_RESOURCE_MEMBER_PROPERTY("SharedGradient", m_hSharedGradient)->AddAttributes(new ezAssetBrowserAttribute("CompatibleAsset_Data_Gradient")),
    EZ_MEMBER_PROPERTY("Color1", m_Color1)->AddAttributes(new ezDefaultValueAttribute(ezColor::White), new ezExposeColorAlphaAttribute()),
    EZ_MEMBER_PROPERTY("Color2", m_Color2)->AddAttributes(new ezDefaultValueAttribute(ezColor::White), new ezExposeColorAlphaAttribute()),
  }
  EZ_END_PROPERTIES;
}
EZ_END_DYNAMIC_REFLECTED_TYPE;

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezParticleInitializer_RandomColor, 1, ezRTTIDefaultAllocator<ezParticleInitializer_RandomColor>)
EZ_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

const ezRTTI* ezParticleInitializerFactory_RandomColor::GetInitializerType() const
{
  return ezGetStaticRTTI<ezParticleInitializer_RandomColor>();
}

void ezParticleInitializerFactory_RandomColor::CopyInitializerProperties(ezParticleInitializer* pInitializer0, bool bFirstTime) const
{
  ezParticleInitializer_RandomColor* pInitializer = static_cast<ezParticleInitializer_RandomColor*>(pInitializer0);

  pInitializer->m_pGradient = &m_Gradient;
  pInitializer->m_Color1 = m_Color1;
  pInitializer->m_Color2 = m_Color2;
}

void ezParticleInitializerFactory_RandomColor::Save(ezStreamWriter& inout_stream) const
{
  const ezUInt8 uiVersion = 2;
  inout_stream << uiVersion;

  inout_stream << m_Color1;
  inout_stream << m_Color2;
  inout_stream << m_GradientSource;
  inout_stream << m_hSharedGradient;
  m_Gradient.Save(inout_stream);
}

void ezParticleInitializerFactory_RandomColor::Load(ezStreamReader& inout_stream)
{
  ezUInt8 uiVersion = 0;
  inout_stream >> uiVersion;

  if (uiVersion == 1)
  {
    // Old version: read the gradient handle
    ezColorGradientResourceHandle hGradient;
    inout_stream >> hGradient;

    // Convert to new format using shared gradient
    m_GradientSource = ezGradientSource::SharedGradient;
    m_hSharedGradient = hGradient;
  }

  inout_stream >> m_Color1;
  inout_stream >> m_Color2;

  if (uiVersion >= 2)
  {
    inout_stream >> m_GradientSource;
    inout_stream >> m_hSharedGradient;
    m_Gradient.Load(inout_stream);
  }

  if (m_GradientSource == ezGradientSource::SharedGradient && m_hSharedGradient.IsValid())
  {
    ezResourceLock<ezColorGradientResource> pGradientResource(m_hSharedGradient, ezResourceAcquireMode::BlockTillLoaded);
    if (pGradientResource.GetAcquireResult() == ezResourceAcquireResult::Final)
    {
      m_Gradient = pGradientResource->GetDescriptor().m_Gradient;
    }
  }
}


void ezParticleInitializer_RandomColor::CreateRequiredStreams()
{
  CreateStream("Color", ezProcessingStream::DataType::Half4, &m_pStreamColor, true);
}

void ezParticleInitializer_RandomColor::InitializeElements(ezUInt64 uiStartIndex, ezUInt64 uiNumElements)
{
  EZ_PROFILE_SCOPE("PFX: Random Color");

  ezColorLinear16f* pColor = m_pStreamColor->GetWritableData<ezColorLinear16f>();

  ezRandom& rng = GetRNG();

  if (m_pGradient == nullptr || m_pGradient->IsEmpty())
  {
    for (ezUInt64 i = uiStartIndex; i < uiStartIndex + uiNumElements; ++i)
    {
      const float f = (float)rng.DoubleZeroToOneInclusive();
      pColor[i] = ezMath::Lerp(m_Color1, m_Color2, f);
    }
  }
  else
  {
    ezColorGammaUB color;
    float intensity;

    const bool bMulColor = (m_Color1 != ezColor::White) || (m_Color2 != ezColor::White);

    for (ezUInt64 i = uiStartIndex; i < uiStartIndex + uiNumElements; ++i)
    {
      m_pGradient->Evaluate(rng.DoubleZeroToOneInclusive(), color, intensity);

      ezColor result = color;
      result.ScaleRGB(intensity);

      if (bMulColor)
      {
        const float f2 = (float)rng.DoubleZeroToOneInclusive();
        result *= ezMath::Lerp(m_Color1, m_Color2, f2);
      }

      pColor[i] = result;
    }
  }
}



EZ_STATICLINK_FILE(ParticlePlugin, ParticlePlugin_Initializer_ParticleInitializer_RandomColor);
