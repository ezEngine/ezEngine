#include <ParticlePlugin/ParticlePluginPCH.h>

#include <Foundation/DataProcessing/Stream/ProcessingStreamIterator.h>
#include <Foundation/Math/Color16f.h>
#include <Foundation/Profiling/Profiling.h>
#include <ParticlePlugin/Behavior/ParticleBehavior_Opacity.h>
#include <ParticlePlugin/Effect/ParticleEffectInstance.h>

// clang-format off
EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezParticleBehaviorFactory_Opacity, 2, ezRTTIDefaultAllocator<ezParticleBehaviorFactory_Opacity>)
{
  EZ_BEGIN_PROPERTIES
  {
    EZ_ENUM_MEMBER_PROPERTY("ChangeOpacityWith", ezCurveSource, m_CurveSource),
    EZ_MEMBER_PROPERTY("OpacityCurve", m_Curve),
    EZ_RESOURCE_MEMBER_PROPERTY("SharedOpacityCurve", m_hSharedCurve)->AddAttributes(new ezAssetBrowserAttribute("CompatibleAsset_Data_Curve")),
  }
  EZ_END_PROPERTIES;
}
EZ_END_DYNAMIC_REFLECTED_TYPE;

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezParticleBehavior_Opacity, 1, ezRTTIDefaultAllocator<ezParticleBehavior_Opacity>)
EZ_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

const ezRTTI* ezParticleBehaviorFactory_Opacity::GetBehaviorType() const
{
  return ezGetStaticRTTI<ezParticleBehavior_Opacity>();
}

void ezParticleBehaviorFactory_Opacity::CopyBehaviorProperties(ezParticleBehavior* pObject, bool bFirstTime) const
{
  ezParticleBehavior_Opacity* pBehavior = static_cast<ezParticleBehavior_Opacity*>(pObject);

  pBehavior->m_pCurve = &m_RuntimeCurve;
}

void ezParticleBehaviorFactory_Opacity::Save(ezStreamWriter& inout_stream) const
{
  const ezUInt8 uiVersion = 1;
  inout_stream << uiVersion;

  inout_stream << m_CurveSource;
  inout_stream << m_hSharedCurve;

  m_Curve.ConvertToRuntimeData(m_RuntimeCurve);
  m_RuntimeCurve.SortControlPoints();
  m_RuntimeCurve.Save(inout_stream);
}

void ezParticleBehaviorFactory_Opacity::Load(ezStreamReader& inout_stream, const ezParticleEffectDescriptor& ownerEffectDescriptor, const ezParticleSystemDescriptor& ownerSystemDescriptor)
{
  ezUInt8 uiVersion = 0;
  inout_stream >> uiVersion;

  inout_stream >> m_CurveSource;
  inout_stream >> m_hSharedCurve;

  m_RuntimeCurve.Load(inout_stream);
  m_RuntimeCurve.SortControlPoints(); // also updates the aabb
  m_RuntimeCurve.CreateLinearApproximation();

  if (m_CurveSource == ezCurveSource::SharedCurve && m_hSharedCurve.IsValid())
  {
    ezResourceLock<ezCurve1DResource> pCurveResource(m_hSharedCurve, ezResourceAcquireMode::BlockTillLoaded);
    if (pCurveResource.GetAcquireResult() == ezResourceAcquireResult::Final && !pCurveResource->GetDescriptor().m_Curves.IsEmpty())
    {
      m_RuntimeCurve = pCurveResource->GetDescriptor().m_Curves[0];
    }
  }
}

void ezParticleBehavior_Opacity::CreateRequiredStreams()
{
  CreateStream("LifeTime", ezProcessingStream::DataType::Half2, &m_pStreamLifeTime, false);
  CreateStream("Color", ezProcessingStream::DataType::Half4, &m_pStreamColor, false);
}

void ezParticleBehavior_Opacity::Process(ezUInt64 uiNumElements)
{
  if (!GetOwnerEffect()->IsVisible())
  {
    // When invisible, don't update at all. Set the interval to 1 so that once
    // the effect becomes visible, all particles get fully updated on the next frame.
    m_uiCurrentUpdateInterval = 1;
    m_uiFirstToUpdate = 0;
    return;
  }

  if (m_pCurve == nullptr || m_pCurve->IsEmpty())
    return;

  EZ_PROFILE_SCOPE("PFX: Opacity");

  ezProcessingStreamIterator<ezFloat16Vec2> itLifeTime(m_pStreamLifeTime, uiNumElements, 0);
  ezProcessingStreamIterator<ezColorLinear16f> itColor(m_pStreamColor, uiNumElements, 0);

  double fMinX, fMaxX;
  m_pCurve->QueryExtents(fMinX, fMaxX);

  // make sure the curve has a length of at least 1
  fMinX = ezMath::Min(fMinX, 0.0);
  fMaxX = ezMath::Max(fMaxX, 1.0);

  // skip the first n particles
  itLifeTime.Advance(m_uiFirstToUpdate);
  itColor.Advance(m_uiFirstToUpdate);

  while (!itLifeTime.HasReachedEnd())
  {
    // particle age: 0 at birth, increases to 1 at death
    const float fInvParticleAge = itLifeTime.Current().x * itLifeTime.Current().y;

    const double evalPos = ezMath::Lerp(fMaxX, fMinX, fInvParticleAge);
    const float fOpacity = (float)m_pCurve->Evaluate(evalPos);

    itColor.Current().a = ezMath::Clamp(fOpacity, 0.0f, 1.0f);

    // skip the next n items
    // this is to reduce the number of particles that need to be fully evaluated,
    // since sampling the curve is expensive
    itLifeTime.Advance(m_uiCurrentUpdateInterval);
    itColor.Advance(m_uiCurrentUpdateInterval);
  }

  // adjust which index is the first to update
  {
    ++m_uiFirstToUpdate;
    if (m_uiFirstToUpdate >= m_uiCurrentUpdateInterval)
      m_uiFirstToUpdate = 0;
  }

  // reset the update interval to the default
  m_uiCurrentUpdateInterval = 2;
}



EZ_STATICLINK_FILE(ParticlePlugin, ParticlePlugin_Behavior_ParticleBehavior_Opacity);
