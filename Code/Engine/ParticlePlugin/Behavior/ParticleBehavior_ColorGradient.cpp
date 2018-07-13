#include <PCH.h>

#include <Core/WorldSerializer/ResourceHandleReader.h>
#include <Core/WorldSerializer/ResourceHandleWriter.h>
#include <Foundation/DataProcessing/Stream/ProcessingStreamIterator.h>
#include <Foundation/Math/Color16f.h>
#include <Foundation/Profiling/Profiling.h>
#include <ParticlePlugin/Behavior/ParticleBehavior_ColorGradient.h>
#include <ParticlePlugin/Effect/ParticleEffectInstance.h>

// clang-format off
EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezParticleBehaviorFactory_ColorGradient, 1, ezRTTIDefaultAllocator<ezParticleBehaviorFactory_ColorGradient>)
{
  EZ_BEGIN_PROPERTIES
  {
    EZ_ACCESSOR_PROPERTY("Gradient", GetColorGradientFile, SetColorGradientFile)->AddAttributes(new ezAssetBrowserAttribute("ColorGradient")),
    EZ_ENUM_MEMBER_PROPERTY("ColorGradientMode", ezParticleColorGradientMode, m_GradientMode),
    EZ_MEMBER_PROPERTY("GradientMaxSpeed", m_fMaxSpeed)->AddAttributes(new ezDefaultValueAttribute(1.0f), new ezClampValueAttribute(0.0f, 100.0f)),
  }
  EZ_END_PROPERTIES;
}
EZ_END_DYNAMIC_REFLECTED_TYPE;

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezParticleBehavior_ColorGradient, 1, ezRTTIDefaultAllocator<ezParticleBehavior_ColorGradient>)
EZ_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

const ezRTTI* ezParticleBehaviorFactory_ColorGradient::GetBehaviorType() const
{
  return ezGetStaticRTTI<ezParticleBehavior_ColorGradient>();
}

void ezParticleBehaviorFactory_ColorGradient::CopyBehaviorProperties(ezParticleBehavior* pObject) const
{
  ezParticleBehavior_ColorGradient* pBehavior = static_cast<ezParticleBehavior_ColorGradient*>(pObject);

  pBehavior->m_hGradient = m_hGradient;
  pBehavior->m_GradientMode = m_GradientMode;
  pBehavior->m_fMaxSpeed = m_fMaxSpeed;
}

void ezParticleBehaviorFactory_ColorGradient::Save(ezStreamWriter& stream) const
{
  const ezUInt8 uiVersion = 3;
  stream << uiVersion;

  stream << m_hGradient;

  // version 3
  stream << m_GradientMode;
  stream << m_fMaxSpeed;
}

void ezParticleBehaviorFactory_ColorGradient::Load(ezStreamReader& stream)
{
  ezUInt8 uiVersion = 0;
  stream >> uiVersion;

  stream >> m_hGradient;

  if (uiVersion >= 3)
  {
    stream >> m_GradientMode;
    stream >> m_fMaxSpeed;
  }
}

void ezParticleBehaviorFactory_ColorGradient::SetColorGradientFile(const char* szFile)
{
  ezColorGradientResourceHandle m_hGradient;

  if (!ezStringUtils::IsNullOrEmpty(szFile))
  {
    m_hGradient = ezResourceManager::LoadResource<ezColorGradientResource>(szFile);
  }

  SetColorGradient(m_hGradient);
}

const char* ezParticleBehaviorFactory_ColorGradient::GetColorGradientFile() const
{
  if (!m_hGradient.IsValid())
    return "";

  return m_hGradient.GetResourceID();
}

void ezParticleBehavior_ColorGradient::AfterPropertiesConfigured(bool bFirstTime)
{
  // the gradient resource may not be specified yet, so defer evaluation until an element is created
  m_InitColor = ezColor::RebeccaPurple;
}

void ezParticleBehavior_ColorGradient::CreateRequiredStreams()
{
  m_pStreamColor = nullptr;
  m_pStreamVelocity = nullptr;

  CreateStream("Color", ezProcessingStream::DataType::Half4, &m_pStreamColor, false);

  if (m_GradientMode == ezParticleColorGradientMode::Age)
  {
    CreateStream("LifeTime", ezProcessingStream::DataType::Half2, &m_pStreamLifeTime, false);
  }
  else if (m_GradientMode == ezParticleColorGradientMode::Speed)
  {
    CreateStream("Velocity", ezProcessingStream::DataType::Float3, &m_pStreamVelocity, false);
  }
}

void ezParticleBehavior_ColorGradient::InitializeElements(ezUInt64 uiStartIndex, ezUInt64 uiNumElements)
{
  if (!m_hGradient.IsValid())
    return;

  EZ_PROFILE("PFX: Color Gradient Init");

  // query the init color from the gradient
  if (m_InitColor == ezColor::RebeccaPurple)
  {
    m_InitColor = ezColor::White;

    ezResourceLock<ezColorGradientResource> pGradient(m_hGradient, ezResourceAcquireMode::NoFallback);

    if (!pGradient->IsMissingResource())
    {
      const ezColorGradient& gradient = pGradient->GetDescriptor().m_Gradient;

      ezColor rgba;
      ezUInt8 alpha;
      gradient.EvaluateColor(0, rgba);
      gradient.EvaluateAlpha(0, alpha);
      rgba.a = ezMath::ColorByteToFloat(alpha);

      m_InitColor = rgba;
    }
  }

  const ezColorLinear16f initCol16 = m_InitColor;

  ezProcessingStreamIterator<ezColorLinear16f> itColor(m_pStreamColor, uiNumElements, uiStartIndex);
  while (!itColor.HasReachedEnd())
  {
    itColor.Current() = initCol16;
    itColor.Advance();
  }
}

void ezParticleBehavior_ColorGradient::Process(ezUInt64 uiNumElements)
{
  if (!GetOwnerEffect()->IsVisible())
  {
    // set the update interval such that once the effect becomes visible,
    // all particles get fully updated
    m_uiCurrentUpdateInterval = 1;
    m_uiFirstToUpdate = 0;
    return;
  }

  if (!m_hGradient.IsValid())
    return;

  EZ_PROFILE("PFX: Color Gradient");

  ezResourceLock<ezColorGradientResource> pGradient(m_hGradient, ezResourceAcquireMode::NoFallback);

  if (pGradient->IsMissingResource())
    return;

  const ezColorGradient& gradient = pGradient->GetDescriptor().m_Gradient;

  ezProcessingStreamIterator<ezColorLinear16f> itColor(m_pStreamColor, uiNumElements, 0);

  // skip the first n particles
  itColor.Advance(m_uiFirstToUpdate);

  if (m_GradientMode == ezParticleColorGradientMode::Age)
  {
    ezProcessingStreamIterator<ezFloat16Vec2> itLifeTime(m_pStreamLifeTime, uiNumElements, 0);

    // skip the first n particles
    itLifeTime.Advance(m_uiFirstToUpdate);

    while (!itLifeTime.HasReachedEnd())
    {
      // if (itLifeTime.Current().y > 0)
      {
        const float fLifeTimeFraction = itLifeTime.Current().x * itLifeTime.Current().y;
        const float posx = 1.0f - fLifeTimeFraction;

        ezColor rgba;
        ezUInt8 alpha;
        gradient.EvaluateColor(posx, rgba);
        gradient.EvaluateAlpha(posx, alpha);
        rgba.a = ezMath::ColorByteToFloat(alpha);

        itColor.Current() = rgba;
      }

      // skip the next n items
      // this is to reduce the number of particles that need to be fully evaluated,
      // since sampling the color gradient is pretty expensive
      itLifeTime.Advance(m_uiCurrentUpdateInterval);
      itColor.Advance(m_uiCurrentUpdateInterval);
    }
  }
  else if (m_GradientMode == ezParticleColorGradientMode::Speed)
  {
    ezProcessingStreamIterator<ezVec3> itVelocity(m_pStreamVelocity, uiNumElements, 0);

    // skip the first n particles
    itVelocity.Advance(m_uiFirstToUpdate);

    while (!itVelocity.HasReachedEnd())
    {
      // if (itLifeTime.Current().y > 0)
      {
        const float fSpeed = itVelocity.Current().GetLength();
        const float posx = fSpeed / m_fMaxSpeed; // no need to clamp the range, the color lookup will already do that

        ezColor rgba;
        ezUInt8 alpha;
        gradient.EvaluateColor(posx, rgba);
        gradient.EvaluateAlpha(posx, alpha);
        rgba.a = ezMath::ColorByteToFloat(alpha);

        itColor.Current() = rgba;
      }

      // skip the next n items
      // this is to reduce the number of particles that need to be fully evaluated,
      // since sampling the color gradient is pretty expensive
      itVelocity.Advance(m_uiCurrentUpdateInterval);
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



EZ_STATICLINK_FILE(ParticlePlugin, ParticlePlugin_Behavior_ParticleBehavior_ColorGradient);
