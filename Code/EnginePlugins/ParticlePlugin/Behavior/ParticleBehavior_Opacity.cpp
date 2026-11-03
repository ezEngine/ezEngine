#include <ParticlePlugin/ParticlePluginPCH.h>

#include <Foundation/DataProcessing/Stream/ProcessingStreamIterator.h>
#include <Foundation/Math/Color16f.h>
#include <Foundation/Profiling/Profiling.h>
#include <ParticlePlugin/Behavior/ParticleBehavior_Opacity.h>
#include <ParticlePlugin/Effect/ParticleEffectInstance.h>

// clang-format off
EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezParticleBehavior_Opacity, 2, ezRTTIDefaultAllocator<ezParticleBehavior_Opacity>)
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
// clang-format on

void ezParticleBehavior_Opacity::Save(ezStreamWriter& inout_stream) const
{
  const ezUInt8 uiVersion = 1;
  inout_stream << uiVersion;

  inout_stream << m_CurveSource;
  inout_stream << m_hSharedCurve;

  m_Curve.ConvertToRuntimeData(m_RuntimeCurve);
  m_RuntimeCurve.SortControlPoints();
  m_RuntimeCurve.Save(inout_stream);
}

void ezParticleBehavior_Opacity::Load(ezStreamReader& inout_stream, const ezParticleEffectDescriptor& ownerEffectDescriptor, const ezParticleSystemDescriptor& ownerSystemDescriptor)
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

bool ezParticleBehavior_Opacity::GetInstanceDataDesc(ezInstanceDataDesc& out_desc) const
{
  out_desc.FillFromType<InstanceData>();
  return true;
}

void ezParticleBehavior_Opacity::CreateRequiredStreams(const ezParticleModuleContext& ctx) const
{
  InstanceData& data = ctx.GetInstanceData<InstanceData>();

  ctx.CreateStream(ezParticleStreamNames::s_sLifeTime, ezProcessingStream::DataType::Half2, &data.m_pStreamLifeTime, false);
  ctx.CreateStream(ezParticleStreamNames::s_sColor, ezProcessingStream::DataType::Half4, &data.m_pStreamColor, false);
}

void ezParticleBehavior_Opacity::Process(const ezParticleModuleContext& ctx, ezUInt64 uiNumElements) const
{
  InstanceData& data = ctx.GetInstanceData<InstanceData>();

  if (!ctx.GetEffect()->IsVisible())
  {
    // When invisible, don't update at all. Set the interval to 1 so that once
    // the effect becomes visible, all particles get fully updated on the next frame.
    data.m_uiCurrentUpdateInterval = 1;
    data.m_uiFirstToUpdate = 0;
    return;
  }

  if (m_RuntimeCurve.IsEmpty())
    return;

  EZ_PROFILE_SCOPE("PFX: Opacity");

  ezProcessingStreamIterator<ezFloat16Vec2> itLifeTime(data.m_pStreamLifeTime, uiNumElements, 0);
  ezProcessingStreamIterator<ezColorLinear16f> itColor(data.m_pStreamColor, uiNumElements, 0);

  double fMinX, fMaxX;
  m_RuntimeCurve.QueryExtents(fMinX, fMaxX);

  // make sure the curve has a length of at least 1
  fMinX = ezMath::Min(fMinX, 0.0);
  fMaxX = ezMath::Max(fMaxX, 1.0);

  // skip the first n particles
  itLifeTime.Advance(data.m_uiFirstToUpdate);
  itColor.Advance(data.m_uiFirstToUpdate);

  while (!itLifeTime.HasReachedEnd())
  {
    // particle age: 0 at birth, increases to 1 at death
    const float fInvParticleAge = itLifeTime.Current().x * itLifeTime.Current().y;

    const double evalPos = ezMath::Lerp(fMaxX, fMinX, fInvParticleAge);
    const float fOpacity = (float)m_RuntimeCurve.Evaluate(evalPos);

    itColor.Current().a = ezMath::Clamp(fOpacity, 0.0f, 1.0f);

    // skip the next n items
    // this is to reduce the number of particles that need to be fully evaluated,
    // since sampling the curve is expensive
    itLifeTime.Advance(data.m_uiCurrentUpdateInterval);
    itColor.Advance(data.m_uiCurrentUpdateInterval);
  }

  // adjust which index is the first to update
  {
    ++data.m_uiFirstToUpdate;
    if (data.m_uiFirstToUpdate >= data.m_uiCurrentUpdateInterval)
      data.m_uiFirstToUpdate = 0;
  }

  // reset the update interval to the default
  data.m_uiCurrentUpdateInterval = 2;
}



EZ_STATICLINK_FILE(ParticlePlugin, ParticlePlugin_Behavior_ParticleBehavior_Opacity);
