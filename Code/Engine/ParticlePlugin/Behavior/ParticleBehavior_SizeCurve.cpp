#include <ParticlePlugin/PCH.h>
#include <ParticlePlugin/Behavior/ParticleBehavior_SizeCurve.h>
#include <CoreUtils/DataProcessing/Stream/ProcessingStreamIterator.h>
#include <Core/WorldSerializer/ResourceHandleReader.h>
#include <Core/WorldSerializer/ResourceHandleWriter.h>

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezParticleBehaviorFactory_SizeCurve, 1, ezRTTIDefaultAllocator<ezParticleBehaviorFactory_SizeCurve>)
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

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezParticleBehavior_SizeCurve, 1, ezRTTIDefaultAllocator<ezParticleBehavior_SizeCurve>)
EZ_END_DYNAMIC_REFLECTED_TYPE

const ezRTTI* ezParticleBehaviorFactory_SizeCurve::GetBehaviorType() const
{
  return ezGetStaticRTTI<ezParticleBehavior_SizeCurve>();
}

void ezParticleBehaviorFactory_SizeCurve::CopyBehaviorProperties(ezParticleBehavior* pObject) const
{
  ezParticleBehavior_SizeCurve* pBehavior = static_cast<ezParticleBehavior_SizeCurve*>(pObject);

  pBehavior->m_hCurve = m_hCurve;
  pBehavior->m_fMinSize = m_fMinSize;
  pBehavior->m_fSizeRange = m_fSizeRange;
}

void ezParticleBehaviorFactory_SizeCurve::Save(ezStreamWriter& stream) const
{
  const ezUInt8 uiVersion = 1;
  stream << uiVersion;

  stream << m_hCurve;
  stream << m_fMinSize;
  stream << m_fSizeRange;
}

void ezParticleBehaviorFactory_SizeCurve::Load(ezStreamReader& stream)
{
  ezUInt8 uiVersion = 0;
  stream >> uiVersion;

  stream >> m_hCurve;
  stream >> m_fMinSize;
  stream >> m_fSizeRange;
}

void ezParticleBehaviorFactory_SizeCurve::SetSizeCurveFile(const char* szFile)
{
  ezCurve1DResourceHandle hResource;

  if (!ezStringUtils::IsNullOrEmpty(szFile))
  {
    hResource = ezResourceManager::LoadResource<ezCurve1DResource>(szFile);
  }

  m_hCurve = hResource;
}


const char* ezParticleBehaviorFactory_SizeCurve::GetSizeCurveFile() const
{
  if (!m_hCurve.IsValid())
    return "";

  return m_hCurve.GetResourceID();
}

void ezParticleBehavior_SizeCurve::CreateRequiredStreams()
{
  CreateStream("LifeTime", ezProcessingStream::DataType::Float2, &m_pStreamLifeTime);
  CreateStream("Size", ezProcessingStream::DataType::Float, &m_pStreamSize);
}

void ezParticleBehavior_SizeCurve::Process(ezUInt64 uiNumElements)
{
  if (!m_hCurve.IsValid())
    return;

  ezProcessingStreamIterator<ezVec3> itLifeTime(m_pStreamLifeTime, uiNumElements);
  ezProcessingStreamIterator<float> itSize(m_pStreamSize, uiNumElements);

  ezResourceLock<ezCurve1DResource> pCurve(m_hCurve, ezResourceAcquireMode::NoFallback);

  if (pCurve->IsMissingResource())
    return;

  if (pCurve->GetDescriptor().m_Curves.IsEmpty())
    return;

  auto& curve = pCurve->GetDescriptor().m_Curves[0];

  float fMinX, fMaxX;
  curve.QueryExtents(fMinX, fMaxX);

  while (!itLifeTime.HasReachedEnd())
  {
    if (itLifeTime.Current().y > 0)
    {
      const float fLifeTimeFraction = 1.0f - (itLifeTime.Current().x / itLifeTime.Current().y);

      const float evalPos = curve.ConvertNormalizedPos(fLifeTimeFraction);
      float val = curve.Evaluate(evalPos);
      val = curve.NormalizeValue(val);

      itSize.Current() = m_fMinSize + val * m_fSizeRange;
    }

    itLifeTime.Advance();
    itSize.Advance();
  }
}

