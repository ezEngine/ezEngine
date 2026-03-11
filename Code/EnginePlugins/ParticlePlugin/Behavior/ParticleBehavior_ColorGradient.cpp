#include <ParticlePlugin/ParticlePluginPCH.h>

#include <Foundation/DataProcessing/Stream/ProcessingStreamIterator.h>
#include <Foundation/Math/Color16f.h>
#include <Foundation/Profiling/Profiling.h>
#include <ParticlePlugin/Behavior/ParticleBehavior_ColorGradient.h>
#include <ParticlePlugin/Effect/ParticleEffectInstance.h>

// clang-format off
EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezParticleBehaviorFactory_ColorGradient, 3, ezRTTIDefaultAllocator<ezParticleBehaviorFactory_ColorGradient>)
{
  EZ_BEGIN_PROPERTIES
  {
    EZ_ENUM_MEMBER_PROPERTY("GradientSource", ezGradientSource, m_GradientSource),
    EZ_MEMBER_PROPERTY("Gradient", m_Gradient),
    EZ_RESOURCE_MEMBER_PROPERTY("SharedGradient", m_hSharedGradient)->AddAttributes(new ezAssetBrowserAttribute("CompatibleAsset_Data_Gradient")),
    EZ_MEMBER_PROPERTY("TintColor", m_TintColor)->AddAttributes(new ezExposeColorAlphaAttribute()),
    EZ_ENUM_MEMBER_PROPERTY("ColorGradientMode", ezParticleColorGradientMode, m_GradientMode),
    EZ_MEMBER_PROPERTY("GradientMaxSpeed", m_fMaxSpeed)->AddAttributes(new ezDefaultValueAttribute(1.0f), new ezClampValueAttribute(0.0f, 100.0f)),
    EZ_MEMBER_PROPERTY("ApplyAlpha", m_bApplyAlpha)->AddAttributes(new ezDefaultValueAttribute(true)),
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

void ezParticleBehaviorFactory_ColorGradient::CopyBehaviorProperties(ezParticleBehavior* pObject, bool bFirstTime) const
{
  ezParticleBehavior_ColorGradient* pBehavior = static_cast<ezParticleBehavior_ColorGradient*>(pObject);

  pBehavior->m_pGradient = &m_Gradient;
  pBehavior->m_GradientMode = m_GradientMode;
  pBehavior->m_fMaxSpeed = m_fMaxSpeed;
  pBehavior->m_TintColor = m_TintColor;
  pBehavior->m_bApplyAlpha = m_bApplyAlpha;

  // the gradient resource may not be specified yet, so defer evaluation until an element is created
  pBehavior->m_InitColor = ezColor::RebeccaPurple;
}

void ezParticleBehaviorFactory_ColorGradient::Save(ezStreamWriter& inout_stream) const
{
  const ezUInt8 uiVersion = 6;
  inout_stream << uiVersion;

  // version 3
  inout_stream << m_GradientMode;
  inout_stream << m_fMaxSpeed;

  // Version 4
  inout_stream << m_TintColor;

  // Version 5
  inout_stream << m_bApplyAlpha;

  // Version 6
  inout_stream << m_GradientSource;
  inout_stream << m_hSharedGradient;
  m_Gradient.Save(inout_stream);
}

void ezParticleBehaviorFactory_ColorGradient::Load(ezStreamReader& inout_stream)
{
  ezUInt8 uiVersion = 0;
  inout_stream >> uiVersion;

  if (uiVersion < 6)
  {
    // Old version: read the gradient handle
    ezColorGradientResourceHandle hGradient;
    inout_stream >> hGradient;

    // Convert to new format using shared gradient
    m_GradientSource = ezGradientSource::SharedGradient;
    m_hSharedGradient = hGradient;
  }

  if (uiVersion >= 3)
  {
    inout_stream >> m_GradientMode;
    inout_stream >> m_fMaxSpeed;
  }

  if (uiVersion >= 4)
  {
    inout_stream >> m_TintColor;
  }

  if (uiVersion >= 5)
  {
    inout_stream >> m_bApplyAlpha;
  }

  if (uiVersion >= 6)
  {
    inout_stream >> m_GradientSource;
    inout_stream >> m_hSharedGradient;
    m_Gradient.Load(inout_stream);
  }

  if (m_GradientSource == ezGradientSource::SharedGradient && m_hSharedGradient.IsValid())
  {
    ezResourceLock<ezColorGradientResource> pGradientResource(m_hSharedGradient, ezResourceAcquireMode::BlockTillLoaded);
    if (pGradientResource.GetAcquireResult() == ezResourceAcquireResult::Final)
    {
      m_Gradient = pGradientResource->GetDescriptor().m_Gradient;
    }
  }
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
    CreateStream("Velocity", ezProcessingStream::DataType::Half4, &m_pStreamVelocity, false);
  }
}

void ezParticleBehavior_ColorGradient::InitializeElements(ezUInt64 uiStartIndex, ezUInt64 uiNumElements)
{
  EZ_PROFILE_SCOPE("PFX: Color Gradient Init");

  // query the init color from the gradient
  if (m_InitColor == ezColor::RebeccaPurple)
  {
    m_InitColor = m_TintColor;

    if (m_pGradient != nullptr && !m_pGradient->IsEmpty())
    {
      ezColor rgba;
      m_pGradient->EvaluateColor(0, rgba);

      float fIntensity = 1.0f;
      m_pGradient->EvaluateIntensity(0, fIntensity);
      rgba.ScaleRGB(fIntensity);

      if (m_bApplyAlpha)
      {
        ezUInt8 alpha;
        m_pGradient->EvaluateAlpha(0, alpha);
        rgba.a = ezMath::ColorByteToFloat(alpha);
      }

      m_InitColor = m_TintColor * rgba;
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
    // When invisible, don't update at all. Set the interval to 1 so that once
    // the effect becomes visible, all particles get fully updated on the next frame.
    m_uiCurrentUpdateInterval = 1;
    m_uiFirstToUpdate = 0;
    return;
  }

  if (m_pGradient == nullptr || m_pGradient->IsEmpty())
    return;

  EZ_PROFILE_SCOPE("PFX: Color Gradient");

  ezProcessingStreamIterator<ezColorLinear16f> itColor(m_pStreamColor, uiNumElements, 0);

  // Staggered update: skip to the first particle to update this frame
  itColor.Advance(m_uiFirstToUpdate);

  if (m_GradientMode == ezParticleColorGradientMode::Age)
  {
    ezProcessingStreamIterator<ezFloat16Vec2> itLifeTime(m_pStreamLifeTime, uiNumElements, 0);

    itLifeTime.Advance(m_uiFirstToUpdate);

    while (!itLifeTime.HasReachedEnd())
    {
      // if (itLifeTime.Current().y > 0)
      {
        const float fLifeTimeFraction = itLifeTime.Current().x * itLifeTime.Current().y;
        const float posx = 1.0f - fLifeTimeFraction;

        ezColor rgba;
        m_pGradient->EvaluateColor(posx, rgba);

        float fIntensity = 1.0f;
        m_pGradient->EvaluateIntensity(posx, fIntensity);
        rgba.ScaleRGB(fIntensity);

        if (m_bApplyAlpha)
        {
          ezUInt8 alpha = 0;
          m_pGradient->EvaluateAlpha(posx, alpha);
          rgba.a = ezMath::ColorByteToFloat(alpha);
        }
        else
        {
          rgba.a = itColor.Current().a;
        }

        itColor.Current() = rgba * m_TintColor;
      }

      // Staggered update: skip to the next particle to update
      itLifeTime.Advance(m_uiCurrentUpdateInterval);
      itColor.Advance(m_uiCurrentUpdateInterval);
    }
  }
  else if (m_GradientMode == ezParticleColorGradientMode::Speed)
  {
    ezProcessingStreamIterator<const ezFloat16Vec4> itVelocity(m_pStreamVelocity, uiNumElements, 0);

    itVelocity.Advance(m_uiFirstToUpdate);

    while (!itVelocity.HasReachedEnd())
    {
      // if (itLifeTime.Current().y > 0)
      {
        const ezVec4 vel = itVelocity.Current();
        const float fSpeed = vel.w;
        const float posx = fSpeed / m_fMaxSpeed; // no need to clamp the range, the color lookup will already do that

        ezColor rgba;
        m_pGradient->EvaluateColor(posx, rgba);

        float fIntensity = 1.0f;
        m_pGradient->EvaluateIntensity(posx, fIntensity);
        rgba.ScaleRGB(fIntensity);

        if (m_bApplyAlpha)
        {
          ezUInt8 alpha = 0;
          m_pGradient->EvaluateAlpha(posx, alpha);
          rgba.a = ezMath::ColorByteToFloat(alpha);
        }
        else
        {
          rgba.a = itColor.Current().a;
        }

        itColor.Current() = rgba * m_TintColor;
      }

      // Staggered update: skip to the next particle to update
      itVelocity.Advance(m_uiCurrentUpdateInterval);
      itColor.Advance(m_uiCurrentUpdateInterval);
    }
  }

  // Advance to the next starting index for the next frame
  ++m_uiFirstToUpdate;
  if (m_uiFirstToUpdate >= m_uiCurrentUpdateInterval)
    m_uiFirstToUpdate = 0;

  /// \todo Use level of detail to reduce the update interval further
  /// up close, with a high interval, animations appear choppy, especially when fading stuff out at the end

  // reset the update interval to the default
  m_uiCurrentUpdateInterval = 2;
}



EZ_STATICLINK_FILE(ParticlePlugin, ParticlePlugin_Behavior_ParticleBehavior_ColorGradient);
