#include <ParticlePlugin/ParticlePluginPCH.h>

#include <Foundation/DataProcessing/Stream/ProcessingStreamIterator.h>
#include <Foundation/Math/Color16f.h>
#include <Foundation/Profiling/Profiling.h>
#include <ParticlePlugin/Behavior/ParticleBehavior_FadeOut.h>
#include <ParticlePlugin/Effect/ParticleEffectInstance.h>

// clang-format off
EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezParticleBehavior_FadeOut, 1, ezRTTIDefaultAllocator<ezParticleBehavior_FadeOut>)
{
  EZ_BEGIN_PROPERTIES
  {
    EZ_MEMBER_PROPERTY("StartAlpha", m_fStartAlpha)->AddAttributes(new ezDefaultValueAttribute(1.0f), new ezClampValueAttribute(0.0f, ezVariant())),
    EZ_MEMBER_PROPERTY("Exponent", m_fExponent)->AddAttributes(new ezDefaultValueAttribute(1.0f)),
  }
  EZ_END_PROPERTIES;
}
EZ_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

void ezParticleBehavior_FadeOut::Save(ezStreamWriter& inout_stream) const
{
  const ezUInt8 uiVersion = 1;
  inout_stream << uiVersion;

  inout_stream << m_fStartAlpha;
  inout_stream << m_fExponent;
}

void ezParticleBehavior_FadeOut::Load(ezStreamReader& inout_stream, const ezParticleEffectDescriptor& ownerEffectDescriptor, const ezParticleSystemDescriptor& ownerSystemDescriptor)
{
  ezUInt8 uiVersion = 0;
  inout_stream >> uiVersion;

  inout_stream >> m_fStartAlpha;
  inout_stream >> m_fExponent;
}

bool ezParticleBehavior_FadeOut::GetInstanceDataDesc(ezInstanceDataDesc& out_desc) const
{
  out_desc.FillFromType<InstanceData>();
  return true;
}

void ezParticleBehavior_FadeOut::CreateRequiredStreams(const ezParticleModuleContext& ctx) const
{
  InstanceData& data = ctx.GetInstanceData<InstanceData>();

  ctx.CreateStream(ezParticleStreamNames::s_sLifeTime, ezProcessingStream::DataType::Half2, &data.m_pStreamLifeTime, false);
  ctx.CreateStream(ezParticleStreamNames::s_sColor, ezProcessingStream::DataType::Half4, &data.m_pStreamColor, false);
}

void ezParticleBehavior_FadeOut::Process(const ezParticleModuleContext& ctx, ezUInt64 uiNumElements) const
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

  EZ_PROFILE_SCOPE("PFX: Fade Out");

  ezProcessingStreamIterator<ezFloat16Vec2> itLifeTime(data.m_pStreamLifeTime, uiNumElements, 0);
  ezProcessingStreamIterator<ezColorLinear16f> itColor(data.m_pStreamColor, uiNumElements, 0);

  // skip the first n particles
  itLifeTime.Advance(data.m_uiFirstToUpdate);
  itColor.Advance(data.m_uiFirstToUpdate);

  if (m_fStartAlpha <= 1.0f)
  {
    while (!itLifeTime.HasReachedEnd())
    {
      const float fLifeTimeFraction = itLifeTime.Current().x * itLifeTime.Current().y;
      itColor.Current().a = m_fStartAlpha * ezMath::Pow(fLifeTimeFraction, m_fExponent);

      // skip the next n items
      // this is to reduce the number of particles that need to be fully evaluated
      itLifeTime.Advance(data.m_uiCurrentUpdateInterval);
      itColor.Advance(data.m_uiCurrentUpdateInterval);
    }
  }
  else
  {
    // this case has to clamp alpha to 1
    while (!itLifeTime.HasReachedEnd())
    {
      const float fLifeTimeFraction = itLifeTime.Current().x * itLifeTime.Current().y;
      itColor.Current().a = ezMath::Min(1.0f, m_fStartAlpha * ezMath::Pow(fLifeTimeFraction, m_fExponent));

      // skip the next n items
      // this is to reduce the number of particles that need to be fully evaluated
      itLifeTime.Advance(data.m_uiCurrentUpdateInterval);
      itColor.Advance(data.m_uiCurrentUpdateInterval);
    }
  }

  // adjust which index is the first to update
  {
    ++data.m_uiFirstToUpdate;
    if (data.m_uiFirstToUpdate >= data.m_uiCurrentUpdateInterval)
      data.m_uiFirstToUpdate = 0;
  }

  /// \todo Use level of detail to reduce the update interval further
  /// up close, with a high interval, animations appear choppy, especially when fading stuff out at the end

  // reset the update interval to the default
  data.m_uiCurrentUpdateInterval = 2;
}



EZ_STATICLINK_FILE(ParticlePlugin, ParticlePlugin_Behavior_ParticleBehavior_FadeOut);
