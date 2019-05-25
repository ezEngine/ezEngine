#include <GameEnginePCH.h>

#include <Core/WorldSerializer/WorldReader.h>
#include <Core/WorldSerializer/WorldWriter.h>
#include <GameEngine/Components/ColorAnimationComponent.h>
#include <RendererCore/Messages/SetColorMessage.h>

// clang-format off
EZ_BEGIN_COMPONENT_TYPE(ezColorAnimationComponent, 1, ezComponentMode::Dynamic)
{
  EZ_BEGIN_PROPERTIES
  {
    EZ_ACCESSOR_PROPERTY("Gradient", GetColorGradientFile, SetColorGradientFile)->AddAttributes(new ezAssetBrowserAttribute("ColorGradient")),
    EZ_MEMBER_PROPERTY("Duration", m_Duration),
  }
  EZ_END_PROPERTIES;
  EZ_BEGIN_ATTRIBUTES
  {
    new ezCategoryAttribute("Animation"),
  }
  EZ_END_ATTRIBUTES;
}
EZ_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

ezColorAnimationComponent::ezColorAnimationComponent() {}

void ezColorAnimationComponent::SerializeComponent(ezWorldWriter& stream) const
{
  SUPER::SerializeComponent(stream);
  auto& s = stream.GetStream();

  s << m_hGradient;
  s << m_Duration;
}

void ezColorAnimationComponent::DeserializeComponent(ezWorldReader& stream)
{
  SUPER::DeserializeComponent(stream);
  // const ezUInt32 uiVersion = stream.GetComponentTypeVersion(GetStaticRTTI());
  auto& s = stream.GetStream();

  s >> m_hGradient;
  s >> m_Duration;
}

void ezColorAnimationComponent::SetColorGradientFile(const char* szFile)
{
  ezColorGradientResourceHandle hResource;

  if (!ezStringUtils::IsNullOrEmpty(szFile))
  {
    hResource = ezResourceManager::LoadResource<ezColorGradientResource>(szFile);
  }

  SetColorGradient(hResource);
}

const char* ezColorAnimationComponent::GetColorGradientFile() const
{
  if (!m_hGradient.IsValid())
    return "";

  return m_hGradient.GetResourceID();
}

void ezColorAnimationComponent::SetColorGradient(const ezColorGradientResourceHandle& hResource)
{
  m_hGradient = hResource;
}

void ezColorAnimationComponent::Update()
{
  if (!m_hGradient.IsValid() || m_Duration <= ezTime::Zero())
    return;

  ezTime tDiff = GetWorld()->GetClock().GetTimeDiff();

  m_CurAnimTime += tDiff;

  // currently just loop the animation
  if (m_CurAnimTime >= m_Duration)
    m_CurAnimTime -= m_Duration;

  ezResourceLock<ezColorGradientResource> pGradient(m_hGradient, ezResourceAcquireMode::AllowFallback);

  if (pGradient.GetAcquireResult() != ezResourceAcquireResult::Final)
    return;

  ezMsgSetColor msg;
  msg.m_Color = pGradient->Evaluate(m_CurAnimTime.GetSeconds() / m_Duration.GetSeconds());
  msg.m_Mode = ezSetColorMode::SetRGBA; // could be a parameter

  // not recursive (could be a parameter)
  GetOwner()->SendMessage(msg);
}




EZ_STATICLINK_FILE(GameEngine, GameEngine_Components_Implementation_ColorAnimationComponent);

