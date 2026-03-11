#include <GameEngine/GameEnginePCH.h>

#include <Core/Messages/SetColorMessage.h>
#include <Core/WorldSerializer/WorldReader.h>
#include <Core/WorldSerializer/WorldWriter.h>
#include <GameEngine/Animation/ColorAnimationComponent.h>

// clang-format off
EZ_BEGIN_COMPONENT_TYPE(ezColorAnimationComponent, 2, ezComponentMode::Static)
{
  EZ_BEGIN_PROPERTIES
  {
    EZ_RESOURCE_ACCESSOR_PROPERTY("Gradient", GetColorGradient, SetColorGradient)->AddAttributes(new ezAssetBrowserAttribute("CompatibleAsset_Data_Gradient")),
    EZ_MEMBER_PROPERTY("Duration", m_Duration),
    EZ_ENUM_MEMBER_PROPERTY("SetColorMode", ezSetColorMode, m_SetColorMode),
    EZ_ENUM_MEMBER_PROPERTY("AnimationMode", ezPropertyAnimMode, m_AnimationMode),
    EZ_ACCESSOR_PROPERTY("RandomStartOffset", GetRandomStartOffset, SetRandomStartOffset)->AddAttributes(new ezDefaultValueAttribute(true)),
    EZ_ACCESSOR_PROPERTY("ApplyToChildren", GetApplyRecursive, SetApplyRecursive),
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

ezColorAnimationComponent::ezColorAnimationComponent() = default;

void ezColorAnimationComponent::SerializeComponent(ezWorldWriter& inout_stream) const
{
  SUPER::SerializeComponent(inout_stream);
  auto& s = inout_stream.GetStream();

  s << m_hGradient;
  s << m_Duration;

  // version 2
  s << m_SetColorMode;
  s << m_AnimationMode;
  s << GetRandomStartOffset();
  s << GetApplyRecursive();
}

void ezColorAnimationComponent::DeserializeComponent(ezWorldReader& inout_stream)
{
  SUPER::DeserializeComponent(inout_stream);
  const ezUInt32 uiVersion = inout_stream.GetComponentTypeVersion(GetStaticRTTI());
  auto& s = inout_stream.GetStream();

  s >> m_hGradient;
  s >> m_Duration;

  if (uiVersion >= 2)
  {
    s >> m_SetColorMode;
    s >> m_AnimationMode;
    bool b;
    s >> b;
    SetRandomStartOffset(b);
    s >> b;
    SetApplyRecursive(b);
  }
}

void ezColorAnimationComponent::OnSimulationStarted()
{
  SUPER::OnSimulationStarted();

  if (GetRandomStartOffset())
  {
    m_CurAnimTime = ezTime::MakeFromSeconds(GetWorld()->GetRandomNumberGenerator().DoubleMinMax(0.0, m_Duration.GetSeconds()));
  }
}

void ezColorAnimationComponent::SetColorGradient(const ezColorGradientResourceHandle& hResource)
{
  m_hGradient = hResource;
}

bool ezColorAnimationComponent::GetApplyRecursive() const
{
  return GetUserFlag(0);
}

void ezColorAnimationComponent::SetApplyRecursive(bool value)
{
  SetUserFlag(0, value);
}

bool ezColorAnimationComponent::GetRandomStartOffset() const
{
  return GetUserFlag(1);
}

void ezColorAnimationComponent::SetRandomStartOffset(bool value)
{
  SetUserFlag(1, value);
}

void ezColorAnimationComponent::Update()
{
  if (!m_hGradient.IsValid() || m_Duration <= ezTime::MakeZero())
    return;

  ezTime tDiff = GetWorld()->GetClock().GetTimeDiff();

  const bool bReverse = GetUserFlag(0);

  if (bReverse)
    m_CurAnimTime -= tDiff;
  else
    m_CurAnimTime += tDiff;

  switch (m_AnimationMode)
  {
    case ezPropertyAnimMode::Once:
    {
      m_CurAnimTime = ezMath::Min(m_CurAnimTime, m_Duration);
      break;
    }

    case ezPropertyAnimMode::Loop:
    {
      if (m_CurAnimTime >= m_Duration)
        m_CurAnimTime -= m_Duration;

      break;
    }

    case ezPropertyAnimMode::BackAndForth:
    {
      if (m_CurAnimTime > m_Duration)
      {
        SetUserFlag(0, !bReverse);

        const ezTime tOver = m_Duration - m_CurAnimTime;

        m_CurAnimTime = m_Duration - tOver;
      }
      else if (m_CurAnimTime < ezTime::MakeZero())
      {
        SetUserFlag(0, !bReverse);

        m_CurAnimTime = -m_CurAnimTime;
      }

      break;
    }
  }

  ezResourceLock<ezColorGradientResource> pGradient(m_hGradient, ezResourceAcquireMode::AllowLoadingFallback);

  if (pGradient.GetAcquireResult() != ezResourceAcquireResult::Final)
    return;

  ezMsgSetColor msg;
  msg.m_Color = pGradient->Evaluate(m_CurAnimTime.GetSeconds() / m_Duration.GetSeconds());
  msg.m_Mode = m_SetColorMode;

  if (GetApplyRecursive())
    GetOwner()->SendMessageRecursive(msg);
  else
    GetOwner()->SendMessage(msg);
}

EZ_STATICLINK_FILE(GameEngine, GameEngine_Animation_Implementation_ColorAnimationComponent);
