#include <ParticlePlugin/ParticlePluginPCH.h>

#include <Foundation/DataProcessing/Stream/ProcessingStreamIterator.h>
#include <Foundation/Math/Color16f.h>
#include <Foundation/Profiling/Profiling.h>
#include <ParticlePlugin/Behavior/ParticleBehavior_FadeOut.h>
#include <ParticlePlugin/Effect/ParticleEffectInstance.h>

// clang-format off
EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezParticleBehaviorFactory_FadeOut, 1, ezRTTIDefaultAllocator<ezParticleBehaviorFactory_FadeOut>)
{
  EZ_BEGIN_PROPERTIES
  {
    EZ_MEMBER_PROPERTY("StartAlpha", m_fStartAlpha)->AddAttributes(new ezDefaultValueAttribute(1.0f), new ezClampValueAttribute(0.0f, ezVariant())),
    EZ_MEMBER_PROPERTY("Exponent", m_fExponent)->AddAttributes(new ezDefaultValueAttribute(1.0f)),
  }
  EZ_END_PROPERTIES;
}
EZ_END_DYNAMIC_REFLECTED_TYPE;

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezParticleBehavior_FadeOut, 1, ezRTTIDefaultAllocator<ezParticleBehavior_FadeOut>)
EZ_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

const ezRTTI* ezParticleBehaviorFactory_FadeOut::GetBehaviorType() const
{
  return ezGetStaticRTTI<ezParticleBehavior_FadeOut>();
}

void ezParticleBehaviorFactory_FadeOut::CopyBehaviorProperties(ezParticleBehavior* pObject, bool bFirstTime) const
{
  ezParticleBehavior_FadeOut* pBehavior = static_cast<ezParticleBehavior_FadeOut*>(pObject);

  pBehavior->m_fStartAlpha = m_fStartAlpha;
  pBehavior->m_fExponent = m_fExponent;
}

void ezParticleBehaviorFactory_FadeOut::Save(ezStreamWriter& inout_stream) const
{
  const ezUInt8 uiVersion = 1;
  inout_stream << uiVersion;

  inout_stream << m_fStartAlpha;
  inout_stream << m_fExponent;
}

void ezParticleBehaviorFactory_FadeOut::Load(ezStreamReader& inout_stream)
{
  ezUInt8 uiVersion = 0;
  inout_stream >> uiVersion;

  inout_stream >> m_fStartAlpha;
  inout_stream >> m_fExponent;
}

void ezParticleBehavior_FadeOut::CreateRequiredStreams()
{
  CreateStream("LifeTime", ezProcessingStream::DataType::Half2, &m_pStreamLifeTime, false);
  CreateStream("Color", ezProcessingStream::DataType::Half4, &m_pStreamColor, false);
}

void ezParticleBehavior_FadeOut::Process(ezUInt64 uiNumElements)
{
  if (!GetOwnerEffect()->IsVisible())
  {
    // When invisible, don't update at all. Set the interval to 1 so that once
    // the effect becomes visible, all particles get fully updated on the next frame.
    m_uiCurrentUpdateInterval = 1;
    m_uiFirstToUpdate = 0;
    return;
  }

  EZ_PROFILE_SCOPE("PFX: Fade Out");

  ezProcessingStreamIterator<ezFloat16Vec2> itLifeTime(m_pStreamLifeTime, uiNumElements, 0);
  ezProcessingStreamIterator<ezColorLinear16f> itColor(m_pStreamColor, uiNumElements, 0);

  // skip the first n particles
  itLifeTime.Advance(m_uiFirstToUpdate);
  itColor.Advance(m_uiFirstToUpdate);

  if (m_fStartAlpha <= 1.0f)
  {
    while (!itLifeTime.HasReachedEnd())
    {
      const float fLifeTimeFraction = itLifeTime.Current().x * itLifeTime.Current().y;
      itColor.Current().a = m_fStartAlpha * ezMath::Pow(fLifeTimeFraction, m_fExponent);

      // skip the next n items
      // this is to reduce the number of particles that need to be fully evaluated
      itLifeTime.Advance(m_uiCurrentUpdateInterval);
      itColor.Advance(m_uiCurrentUpdateInterval);
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
      itLifeTime.Advance(m_uiCurrentUpdateInterval);
      itColor.Advance(m_uiCurrentUpdateInterval);
    }
  }

  // adjust which index is the first to update
  {
    ++m_uiFirstToUpdate;
    if (m_uiFirstToUpdate >= m_uiCurrentUpdateInterval)
      m_uiFirstToUpdate = 0;
  }

  /// \todo Use level of detail to reduce the update interval further
  /// up close, with a high interval, animations appear choppy, especially when fading stuff out at the end

  // reset the update interval to the default
  m_uiCurrentUpdateInterval = 2;
}



EZ_STATICLINK_FILE(ParticlePlugin, ParticlePlugin_Behavior_ParticleBehavior_FadeOut);
