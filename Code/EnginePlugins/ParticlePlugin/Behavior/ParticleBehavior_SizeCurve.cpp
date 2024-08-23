#include <ParticlePlugin/ParticlePluginPCH.h>

#include <Foundation/DataProcessing/Stream/ProcessingStreamIterator.h>
#include <Foundation/Math/Float16.h>
#include <Foundation/Profiling/Profiling.h>
#include <ParticlePlugin/Behavior/ParticleBehavior_SizeCurve.h>
#include <ParticlePlugin/Effect/ParticleEffectInstance.h>

// clang-format off
EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezParticleBehaviorFactory_SizeCurve, 1, ezRTTIDefaultAllocator<ezParticleBehaviorFactory_SizeCurve>)
{
  EZ_BEGIN_PROPERTIES
  {
    EZ_RESOURCE_MEMBER_PROPERTY("SizeCurve", m_hCurve)->AddAttributes(new ezAssetBrowserAttribute("CompatibleAsset_Data_Curve")),
    EZ_MEMBER_PROPERTY("BaseSize", m_fBaseSize)->AddAttributes(new ezDefaultValueAttribute(0.0f), new ezClampValueAttribute(0.0f, ezVariant())),
    EZ_MEMBER_PROPERTY("CurveScale", m_fCurveScale)->AddAttributes(new ezDefaultValueAttribute(1.0f), new ezClampValueAttribute(0.0f, ezVariant())),
  }
  EZ_END_PROPERTIES;
}
EZ_END_DYNAMIC_REFLECTED_TYPE;

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezParticleBehavior_SizeCurve, 1, ezRTTIDefaultAllocator<ezParticleBehavior_SizeCurve>)
EZ_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

const ezRTTI* ezParticleBehaviorFactory_SizeCurve::GetBehaviorType() const
{
  return ezGetStaticRTTI<ezParticleBehavior_SizeCurve>();
}

void ezParticleBehaviorFactory_SizeCurve::CopyBehaviorProperties(ezParticleBehavior* pObject, bool bFirstTime) const
{
  ezParticleBehavior_SizeCurve* pBehavior = static_cast<ezParticleBehavior_SizeCurve*>(pObject);

  pBehavior->m_hCurve = m_hCurve;
  pBehavior->m_fBaseSize = m_fBaseSize;
  pBehavior->m_fCurveScale = m_fCurveScale;
}

void ezParticleBehaviorFactory_SizeCurve::Save(ezStreamWriter& inout_stream) const
{
  const ezUInt8 uiVersion = 1;
  inout_stream << uiVersion;

  inout_stream << m_hCurve;
  inout_stream << m_fBaseSize;
  inout_stream << m_fCurveScale;
}

void ezParticleBehaviorFactory_SizeCurve::Load(ezStreamReader& inout_stream)
{
  ezUInt8 uiVersion = 0;
  inout_stream >> uiVersion;

  inout_stream >> m_hCurve;
  inout_stream >> m_fBaseSize;
  inout_stream >> m_fCurveScale;
}

void ezParticleBehavior_SizeCurve::CreateRequiredStreams()
{
  CreateStream("LifeTime", ezProcessingStream::DataType::Half2, &m_pStreamLifeTime, false);
  CreateStream("Size", ezProcessingStream::DataType::Half, &m_pStreamSize, false);
}


void ezParticleBehavior_SizeCurve::InitializeElements(ezUInt64 uiStartIndex, ezUInt64 uiNumElements)
{
  ezProcessingStreamIterator<ezFloat16> itSize(m_pStreamSize, uiNumElements, uiStartIndex);
  while (!itSize.HasReachedEnd())
  {
    itSize.Current() = m_fBaseSize;
    itSize.Advance();
  }
}

void ezParticleBehavior_SizeCurve::Process(ezUInt64 uiNumElements)
{
  if (!GetOwnerEffect()->IsVisible())
  {
    // reduce the update interval when the effect is not visible
    m_uiCurrentUpdateInterval = 32;
  }
  else
  {
    m_uiCurrentUpdateInterval = 2;
  }

  if (!m_hCurve.IsValid())
    return;

  EZ_PROFILE_SCOPE("PFX: Size Curve");

  ezProcessingStreamIterator<ezFloat16Vec2> itLifeTime(m_pStreamLifeTime, uiNumElements, 0);
  ezProcessingStreamIterator<ezFloat16> itSize(m_pStreamSize, uiNumElements, 0);

  ezResourceLock<ezCurve1DResource> pCurve(m_hCurve, ezResourceAcquireMode::BlockTillLoaded);

  if (pCurve.GetAcquireResult() == ezResourceAcquireResult::MissingFallback)
    return;

  if (pCurve->GetDescriptor().m_Curves.IsEmpty())
    return;

  auto& curve = pCurve->GetDescriptor().m_Curves[0];

  double fMinX, fMaxX;
  curve.QueryExtents(fMinX, fMaxX);

  // skip the first n particles
  {
    for (ezUInt32 i = 0; i < m_uiFirstToUpdate; ++i)
    {
      itLifeTime.Advance();
      itSize.Advance();
    }

    ++m_uiFirstToUpdate;
    if (m_uiFirstToUpdate >= m_uiCurrentUpdateInterval)
      m_uiFirstToUpdate = 0;
  }

  while (!itLifeTime.HasReachedEnd())
  {
    // if (itLifeTime.Current().y > 0)
    {
      const float fLifeTimeFraction = 1.0f - (itLifeTime.Current().x * itLifeTime.Current().y);

      const double evalPos = curve.ConvertNormalizedPos(fLifeTimeFraction);
      double val = curve.Evaluate(evalPos);
      val = curve.NormalizeValue(val);

      itSize.Current() = m_fBaseSize + (float)val * m_fCurveScale;
    }

    // skip the next n items
    // this is to reduce the number of particles that need to be fully evaluated,
    // since sampling the curve is expensive
    for (ezUInt32 i = 0; i < m_uiCurrentUpdateInterval; ++i)
    {
      itLifeTime.Advance();
      itSize.Advance();
    }
  }
}



EZ_STATICLINK_FILE(ParticlePlugin, ParticlePlugin_Behavior_ParticleBehavior_SizeCurve);
