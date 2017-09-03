#include <PCH.h>
#include <ParticlePlugin/Behavior/ParticleBehavior_ColorGradient.h>
#include <Foundation/DataProcessing/Stream/ProcessingStreamIterator.h>
#include <Core/WorldSerializer/ResourceHandleReader.h>
#include <Core/WorldSerializer/ResourceHandleWriter.h>
#include <ParticlePlugin/Effect/ParticleEffectInstance.h>
#include <Foundation/Profiling/Profiling.h>

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezParticleBehaviorFactory_ColorGradient, 1, ezRTTIDefaultAllocator<ezParticleBehaviorFactory_ColorGradient>)
{
  EZ_BEGIN_PROPERTIES
  {
    EZ_ACCESSOR_PROPERTY("Gradient", GetColorGradientFile, SetColorGradientFile)->AddAttributes(new ezAssetBrowserAttribute("ColorGradient")),
  }
  EZ_END_PROPERTIES
}
EZ_END_DYNAMIC_REFLECTED_TYPE

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezParticleBehavior_ColorGradient, 1, ezRTTIDefaultAllocator<ezParticleBehavior_ColorGradient>)
EZ_END_DYNAMIC_REFLECTED_TYPE

const ezRTTI* ezParticleBehaviorFactory_ColorGradient::GetBehaviorType() const
{
  return ezGetStaticRTTI<ezParticleBehavior_ColorGradient>();
}

void ezParticleBehaviorFactory_ColorGradient::CopyBehaviorProperties(ezParticleBehavior* pObject) const
{
  ezParticleBehavior_ColorGradient* pBehavior = static_cast<ezParticleBehavior_ColorGradient*>(pObject);

  pBehavior->m_hGradient = m_hGradient;
}

void ezParticleBehaviorFactory_ColorGradient::Save(ezStreamWriter& stream) const
{
  const ezUInt8 uiVersion = 2;
  stream << uiVersion;

  stream << m_hGradient;
}

void ezParticleBehaviorFactory_ColorGradient::Load(ezStreamReader& stream)
{
  ezUInt8 uiVersion = 0;
  stream >> uiVersion;

  stream >> m_hGradient;
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
  CreateStream("LifeTime", ezProcessingStream::DataType::Float2, &m_pStreamLifeTime, false);
  CreateStream("Color", ezProcessingStream::DataType::Float4, &m_pStreamColor, false);
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

  ezProcessingStreamIterator<ezColor> itColor(m_pStreamColor, uiNumElements, uiStartIndex);
  while (!itColor.HasReachedEnd())
  {
    itColor.Current() = m_InitColor;
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

  ezProcessingStreamIterator<ezVec2> itLifeTime(m_pStreamLifeTime, uiNumElements, 0);
  ezProcessingStreamIterator<ezColor> itColor(m_pStreamColor, uiNumElements, 0);

  // skip the first n particles
  {
    for (ezUInt32 i = 0; i < m_uiFirstToUpdate; ++i)
    {
      itLifeTime.Advance();
      itColor.Advance();
    }

    ++m_uiFirstToUpdate;
    if (m_uiFirstToUpdate >= m_uiCurrentUpdateInterval)
      m_uiFirstToUpdate = 0;
  }

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
    for (ezUInt32 i = 0; i < m_uiCurrentUpdateInterval; ++i)
    {
      itLifeTime.Advance();
      itColor.Advance();
    }
  }

  /// \todo Use level of detail to reduce the update interval further
  /// up close, with a high interval, animations appear choppy, especially when fading stuff out at the end

  // reset the update interval to the default
  m_uiCurrentUpdateInterval = 2;
}



EZ_STATICLINK_FILE(ParticlePlugin, ParticlePlugin_Behavior_ParticleBehavior_ColorGradient);

