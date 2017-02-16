#include <PCH.h>
#include <ParticlePlugin/Behavior/ParticleBehavior_SizeCurve.h>
#include <Foundation/DataProcessing/Stream/ProcessingStreamIterator.h>
#include <Core/WorldSerializer/ResourceHandleReader.h>
#include <Core/WorldSerializer/ResourceHandleWriter.h>

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezParticleBehaviorFactory_SizeCurve, 1, ezRTTIDefaultAllocator<ezParticleBehaviorFactory_SizeCurve>)
{
  EZ_BEGIN_PROPERTIES
  {
    EZ_ACCESSOR_PROPERTY("SizeCurve", GetSizeCurveFile, SetSizeCurveFile)->AddAttributes(new ezAssetBrowserAttribute("Curve1D")),
    EZ_MEMBER_PROPERTY("BaseSize", m_fBaseSize)->AddAttributes(new ezDefaultValueAttribute(0.0f), new ezClampValueAttribute(0.0f, ezVariant())),
    EZ_MEMBER_PROPERTY("CurveScale", m_fCurveScale)->AddAttributes(new ezDefaultValueAttribute(1.0f), new ezClampValueAttribute(0.0f, ezVariant())),
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
  pBehavior->m_fBaseSize = m_fBaseSize;
  pBehavior->m_fCurveScale = m_fCurveScale;
}

void ezParticleBehaviorFactory_SizeCurve::Save(ezStreamWriter& stream) const
{
  const ezUInt8 uiVersion = 1;
  stream << uiVersion;

  stream << m_hCurve;
  stream << m_fBaseSize;
  stream << m_fCurveScale;
}

void ezParticleBehaviorFactory_SizeCurve::Load(ezStreamReader& stream)
{
  ezUInt8 uiVersion = 0;
  stream >> uiVersion;

  stream >> m_hCurve;
  stream >> m_fBaseSize;
  stream >> m_fCurveScale;
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
  CreateStream("LifeTime", ezProcessingStream::DataType::Float2, &m_pStreamLifeTime, false);
  CreateStream("Size", ezProcessingStream::DataType::Float, &m_pStreamSize, false);
}

void ezParticleBehavior_SizeCurve::Process(ezUInt64 uiNumElements)
{
  if (!m_hCurve.IsValid())
    return;

  ezProcessingStreamIterator<ezVec3> itLifeTime(m_pStreamLifeTime, uiNumElements, 0);
  ezProcessingStreamIterator<float> itSize(m_pStreamSize, uiNumElements, 0);

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

      itSize.Current() = m_fBaseSize + val * m_fCurveScale;
    }

    itLifeTime.Advance();
    itSize.Advance();
  }
}



EZ_STATICLINK_FILE(ParticlePlugin, ParticlePlugin_Behavior_ParticleBehavior_SizeCurve);

