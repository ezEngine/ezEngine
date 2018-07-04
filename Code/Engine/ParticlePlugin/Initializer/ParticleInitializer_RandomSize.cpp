#include <PCH.h>
#include <ParticlePlugin/Initializer/ParticleInitializer_RandomSize.h>
#include <Foundation/DataProcessing/Stream/ProcessingStreamGroup.h>
#include <Foundation/Math/Random.h>
#include <ParticlePlugin/System/ParticleSystemInstance.h>
#include <GameEngine/Curves/Curve1DResource.h>
#include <Core/WorldSerializer/ResourceHandleReader.h>
#include <Core/WorldSerializer/ResourceHandleWriter.h>
#include <Foundation/Profiling/Profiling.h>
#include <Foundation/Math/Float16.h>

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezParticleInitializerFactory_RandomSize, 1, ezRTTIDefaultAllocator<ezParticleInitializerFactory_RandomSize>)
{
  EZ_BEGIN_PROPERTIES
  {
    EZ_MEMBER_PROPERTY("Size", m_Size)->AddAttributes(new ezDefaultValueAttribute(1.0f), new ezClampValueAttribute(0.0f, ezVariant())),
    EZ_ACCESSOR_PROPERTY("SizeCurve", GetSizeCurveFile, SetSizeCurveFile)->AddAttributes(new ezAssetBrowserAttribute("Curve1D")),
  }
  EZ_END_PROPERTIES;
}
EZ_END_DYNAMIC_REFLECTED_TYPE;

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezParticleInitializer_RandomSize, 1, ezRTTIDefaultAllocator<ezParticleInitializer_RandomSize>)
EZ_END_DYNAMIC_REFLECTED_TYPE;

const ezRTTI* ezParticleInitializerFactory_RandomSize::GetInitializerType() const
{
  return ezGetStaticRTTI<ezParticleInitializer_RandomSize>();
}

void ezParticleInitializerFactory_RandomSize::CopyInitializerProperties(ezParticleInitializer* pInitializer0) const
{
  ezParticleInitializer_RandomSize* pInitializer = static_cast<ezParticleInitializer_RandomSize*>(pInitializer0);

  pInitializer->m_hCurve = m_hCurve;
  pInitializer->m_Size = m_Size;
}

void ezParticleInitializerFactory_RandomSize::SetSizeCurveFile(const char* szFile)
{
  ezCurve1DResourceHandle hResource;

  if (!ezStringUtils::IsNullOrEmpty(szFile))
  {
    hResource = ezResourceManager::LoadResource<ezCurve1DResource>(szFile);
  }

  m_hCurve = hResource;
}


const char* ezParticleInitializerFactory_RandomSize::GetSizeCurveFile() const
{
  if (!m_hCurve.IsValid())
    return "";

  return m_hCurve.GetResourceID();
}

void ezParticleInitializerFactory_RandomSize::Save(ezStreamWriter& stream) const
{
  const ezUInt8 uiVersion = 2;
  stream << uiVersion;

  stream << m_hCurve;
  stream << m_Size.m_Value;
  stream << m_Size.m_fVariance;
}

void ezParticleInitializerFactory_RandomSize::Load(ezStreamReader& stream)
{
  ezUInt8 uiVersion = 0;
  stream >> uiVersion;

  stream >> m_hCurve;
  stream >> m_Size.m_Value;
  stream >> m_Size.m_fVariance;
}


void ezParticleInitializer_RandomSize::CreateRequiredStreams()
{
  CreateStream("Size", ezProcessingStream::DataType::Half, &m_pStreamSize, true);
}

void ezParticleInitializer_RandomSize::InitializeElements(ezUInt64 uiStartIndex, ezUInt64 uiNumElements)
{
  EZ_PROFILE("PFX: Random Size");

  ezFloat16* pSize = m_pStreamSize->GetWritableData<ezFloat16>();

  ezRandom& rng = GetRNG();

  if (!m_hCurve.IsValid())
  {
    for (ezUInt64 i = uiStartIndex; i < uiStartIndex + uiNumElements; ++i)
    {
      pSize[i] = (float)rng.DoubleVariance(m_Size.m_Value, m_Size.m_fVariance);
    }
  }
  else
  {
    ezResourceLock<ezCurve1DResource> pResource(m_hCurve, ezResourceAcquireMode::NoFallback);

    const ezCurve1D& curve = pResource->GetDescriptor().m_Curves[0];

    double fMinX, fMaxX;
    curve.QueryExtents(fMinX, fMaxX);

    for (ezUInt64 i = uiStartIndex; i < uiStartIndex + uiNumElements; ++i)
    {
      const double f = rng.DoubleMinMax(fMinX, fMaxX);

      double val = curve.Evaluate(f);
      val = curve.NormalizeValue(val);

      pSize[i] = (float)(val * rng.DoubleVariance(m_Size.m_Value, m_Size.m_fVariance));
    }
  }
}



EZ_STATICLINK_FILE(ParticlePlugin, ParticlePlugin_Initializer_ParticleInitializer_RandomSize);

