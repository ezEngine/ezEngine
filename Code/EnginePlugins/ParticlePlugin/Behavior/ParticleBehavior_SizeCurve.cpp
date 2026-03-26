#include <ParticlePlugin/ParticlePluginPCH.h>

#include <Foundation/DataProcessing/Stream/ProcessingStreamIterator.h>
#include <Foundation/Math/Float16.h>
#include <Foundation/Profiling/Profiling.h>
#include <ParticlePlugin/Behavior/ParticleBehavior_SizeCurve.h>
#include <ParticlePlugin/Effect/ParticleEffectInstance.h>

// clang-format off
EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezParticleBehaviorFactory_SizeCurve, 2, ezRTTIDefaultAllocator<ezParticleBehaviorFactory_SizeCurve>)
{
  EZ_BEGIN_PROPERTIES
  {
    EZ_ENUM_MEMBER_PROPERTY("ChangeSizeWith", ezCurveSource, m_CurveSource),
    EZ_MEMBER_PROPERTY("SizeCurve", m_Curve),
    EZ_RESOURCE_MEMBER_PROPERTY("SharedSizeCurve", m_hSharedCurve)->AddAttributes(new ezAssetBrowserAttribute("CompatibleAsset_Data_Curve")),
    EZ_MEMBER_PROPERTY("SizeCurveOffset", m_fSizeCurveOffset)->AddAttributes(new ezDefaultValueAttribute(0.0f), new ezClampValueAttribute(0.0f, ezVariant())),
    EZ_MEMBER_PROPERTY("SizeCurveScale", m_fSizeCurveScale)->AddAttributes(new ezDefaultValueAttribute(1.0f), new ezClampValueAttribute(0.0f, ezVariant())),
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

  pBehavior->m_fSizeCurveOffset = m_fSizeCurveOffset;
  pBehavior->m_fSizeCurveScale = m_fSizeCurveScale;
  pBehavior->m_pCurve = &m_RuntimeCurve;
}

void ezParticleBehaviorFactory_SizeCurve::Save(ezStreamWriter& inout_stream) const
{
  const ezUInt8 uiVersion = 2;
  inout_stream << uiVersion;

  inout_stream << m_CurveSource;
  inout_stream << m_hSharedCurve;
  inout_stream << m_fSizeCurveOffset;
  inout_stream << m_fSizeCurveScale;

  m_Curve.ConvertToRuntimeData(m_RuntimeCurve);
  m_RuntimeCurve.SortControlPoints();
  m_RuntimeCurve.Save(inout_stream);
}

void ezParticleBehaviorFactory_SizeCurve::Load(ezStreamReader& inout_stream, const ezParticleEffectDescriptor& ownerEffectDescriptor, const ezParticleSystemDescriptor& ownerSystemDescriptor)
{
  ezUInt8 uiVersion = 0;
  inout_stream >> uiVersion;

  if (uiVersion >= 2)
  {
    inout_stream >> m_CurveSource;
    inout_stream >> m_hSharedCurve;
  }
  else
  {
    // Version 1: had m_hCurve as the shared curve resource
    ezCurve1DResourceHandle hOldCurve;
    inout_stream >> hOldCurve;

    // Migrate to version 2: default to Shared mode with the old curve
    m_CurveSource = ezCurveSource::SharedCurve;
    m_hSharedCurve = hOldCurve;
  }

  inout_stream >> m_fSizeCurveOffset;
  inout_stream >> m_fSizeCurveScale;

  if (uiVersion >= 2)
  {
    m_RuntimeCurve.Load(inout_stream);
    m_RuntimeCurve.SortControlPoints(); // also updates the aabb
    m_RuntimeCurve.CreateLinearApproximation();
  }

  if (m_CurveSource == ezCurveSource::SharedCurve && m_hSharedCurve.IsValid())
  {
    ezResourceLock<ezCurve1DResource> pCurveResource(m_hSharedCurve, ezResourceAcquireMode::BlockTillLoaded);
    if (pCurveResource.GetAcquireResult() == ezResourceAcquireResult::Final && !pCurveResource->GetDescriptor().m_Curves.IsEmpty())
    {
      m_RuntimeCurve = pCurveResource->GetDescriptor().m_Curves[0];
    }
  }
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
    itSize.Current() = m_fSizeCurveOffset;
    itSize.Advance();
  }
}

void ezParticleBehavior_SizeCurve::Process(ezUInt64 uiNumElements)
{
  if (!GetOwnerEffect()->IsVisible())
  {
    // When the effect is not visible, drastically reduce the update frequency
    // to save CPU, but still update occasionally since size changes can affect
    // visibility culling.
    m_uiCurrentUpdateInterval = 32;
  }
  else
  {
    m_uiCurrentUpdateInterval = 2;
  }

  if (m_pCurve == nullptr || m_pCurve->IsEmpty())
    return;

  EZ_PROFILE_SCOPE("PFX: Size Curve");

  ezProcessingStreamIterator<ezFloat16Vec2> itLifeTime(m_pStreamLifeTime, uiNumElements, 0);
  ezProcessingStreamIterator<ezFloat16> itSize(m_pStreamSize, uiNumElements, 0);

  double fMinX, fMaxX;
  m_pCurve->QueryExtents(fMinX, fMaxX);

  // make sure the curve has a length of at least 1
  fMinX = ezMath::Min(fMinX, 0.0);
  fMaxX = ezMath::Max(fMaxX, 1.0);

  // skip the first n particles
  itLifeTime.Advance(m_uiFirstToUpdate);
  itSize.Advance(m_uiFirstToUpdate);

  while (!itLifeTime.HasReachedEnd())
  {
    // if (itLifeTime.Current().y > 0)
    {
      const float fLifeTimeFraction = itLifeTime.Current().x * itLifeTime.Current().y;

      const double evalPos = ezMath::Lerp(fMaxX, fMinX, fLifeTimeFraction);
      const float val = (float)m_pCurve->Evaluate(evalPos);

      itSize.Current() = m_fSizeCurveOffset + val * m_fSizeCurveScale;
    }

    // skip the next n items
    // this is to reduce the number of particles that need to be fully evaluated,
    // since sampling the curve is expensive
    itLifeTime.Advance(m_uiCurrentUpdateInterval);
    itSize.Advance(m_uiCurrentUpdateInterval);
  }

  // adjust which index is the first to update
  {
    ++m_uiFirstToUpdate;
    if (m_uiFirstToUpdate >= m_uiCurrentUpdateInterval)
      m_uiFirstToUpdate = 0;
  }
}



EZ_STATICLINK_FILE(ParticlePlugin, ParticlePlugin_Behavior_ParticleBehavior_SizeCurve);
