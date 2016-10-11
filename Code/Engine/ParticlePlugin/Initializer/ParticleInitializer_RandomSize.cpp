#include <ParticlePlugin/PCH.h>
#include <ParticlePlugin/Initializer/ParticleInitializer_RandomSize.h>
#include <CoreUtils/DataProcessing/Stream/ProcessingStreamGroup.h>
#include <Foundation/Math/Random.h>
#include <ParticlePlugin/System/ParticleSystemInstance.h>
#include <GameUtils/Curves/Curve1DResource.h>
#include <Core/WorldSerializer/ResourceHandleReader.h>
#include <Core/WorldSerializer/ResourceHandleWriter.h>

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezParticleInitializerFactory_RandomSize, 1, ezRTTIDefaultAllocator<ezParticleInitializerFactory_RandomSize>)
{
  EZ_BEGIN_PROPERTIES
  {
    EZ_ACCESSOR_PROPERTY("Size Curve", GetSizeCurveFile, SetSizeCurveFile)->AddAttributes(new ezAssetBrowserAttribute("Curve1D")),
    EZ_MEMBER_PROPERTY("Min Size", m_fMinSize)->AddAttributes(new ezDefaultValueAttribute(0.0f), new ezClampValueAttribute(0.0f, ezVariant())),
    EZ_MEMBER_PROPERTY("Size Range", m_fSizeRange)->AddAttributes(new ezDefaultValueAttribute(1.0f), new ezClampValueAttribute(0.0f, ezVariant())),
  }
  EZ_END_PROPERTIES
}
EZ_END_DYNAMIC_REFLECTED_TYPE

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezParticleInitializer_RandomSize, 1, ezRTTIDefaultAllocator<ezParticleInitializer_RandomSize>)
EZ_END_DYNAMIC_REFLECTED_TYPE

const ezRTTI* ezParticleInitializerFactory_RandomSize::GetInitializerType() const
{
  return ezGetStaticRTTI<ezParticleInitializer_RandomSize>();
}

void ezParticleInitializerFactory_RandomSize::CopyInitializerProperties(ezParticleInitializer* pInitializer0) const
{
  ezParticleInitializer_RandomSize* pInitializer = static_cast<ezParticleInitializer_RandomSize*>(pInitializer0);

  pInitializer->m_hCurve = m_hCurve;
  pInitializer->m_fMinSize = m_fMinSize;
  pInitializer->m_fSizeRange = m_fSizeRange;
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
  const ezUInt8 uiVersion = 1;
  stream << uiVersion;

  stream << m_hCurve;
  stream << m_fMinSize;
  stream << m_fSizeRange;
}

void ezParticleInitializerFactory_RandomSize::Load(ezStreamReader& stream)
{
  ezUInt8 uiVersion = 0;
  stream >> uiVersion;

  stream >> m_hCurve;
  stream >> m_fMinSize;
  stream >> m_fSizeRange;
}


void ezParticleInitializer_RandomSize::CreateRequiredStreams()
{
  CreateStream("Size", ezProcessingStream::DataType::Float, &m_pStreamSize, true);
}

void ezParticleInitializer_RandomSize::InitializeElements(ezUInt64 uiStartIndex, ezUInt64 uiNumElements)
{
  float* pSize = m_pStreamSize->GetWritableData<float>();

  ezRandom& rng = GetRNG();

  if (!m_hCurve.IsValid())
  {
    for (ezUInt64 i = uiStartIndex; i < uiStartIndex + uiNumElements; ++i)
    {
      pSize[i] = (float)rng.DoubleInRange(m_fMinSize, m_fSizeRange);
    }
  }
  else
  {
    ezResourceLock<ezCurve1DResource> pResource(m_hCurve, ezResourceAcquireMode::NoFallback);

    const ezCurve1D& curve = pResource->GetDescriptor().m_Curves[0];

    float fMinX, fMaxX;
    curve.QueryExtents(fMinX, fMaxX);

    for (ezUInt64 i = uiStartIndex; i < uiStartIndex + uiNumElements; ++i)
    {
      const float f = (float)rng.DoubleMinMax(fMinX, fMaxX);

      float val = curve.Evaluate(f);
      val = curve.NormalizeValue(val);

      pSize[i] = val * (float)rng.DoubleInRange(m_fMinSize, m_fSizeRange);
    }
  }
}
