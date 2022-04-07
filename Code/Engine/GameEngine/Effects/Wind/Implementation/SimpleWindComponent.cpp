#include <GameEngine/GameEnginePCH.h>

#include <Core/WorldSerializer/WorldReader.h>
#include <Core/WorldSerializer/WorldWriter.h>
#include <GameEngine/Effects/Wind/SimpleWindComponent.h>
#include <GameEngine/Effects/Wind/SimpleWindWorldModule.h>

// clang-format off
EZ_BEGIN_COMPONENT_TYPE(ezSimpleWindComponent, 2, ezComponentMode::Static)
{
  EZ_BEGIN_PROPERTIES
  {
    EZ_ENUM_MEMBER_PROPERTY("MinWindStrength", ezWindStrength, m_MinWindStrength),
    EZ_ENUM_MEMBER_PROPERTY("MaxWindStrength", ezWindStrength, m_MaxWindStrength),
    EZ_MEMBER_PROPERTY("MaxDeviation", m_Deviation)->AddAttributes(new ezClampValueAttribute(ezAngle::Degree(0), ezAngle::Degree(180))),
  }
  EZ_END_PROPERTIES;
  EZ_BEGIN_ATTRIBUTES
  {
    new ezCategoryAttribute("Effects/Wind"),
    new ezDirectionVisualizerAttribute(ezBasisAxis::PositiveX, 0.5f, ezColor::DodgerBlue),
  }
  EZ_END_ATTRIBUTES;
}
EZ_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

ezSimpleWindComponent::ezSimpleWindComponent() = default;
ezSimpleWindComponent::~ezSimpleWindComponent() = default;

void ezSimpleWindComponent::Update()
{
  ezSimpleWindWorldModule* pWindModule = GetWorld()->GetModule<ezSimpleWindWorldModule>();

  if (pWindModule == nullptr)
    return;

  const ezTime tCur = GetWorld()->GetClock().GetAccumulatedTime();
  const float fLerp = static_cast<float>((tCur - m_LastChange).GetSeconds() / (m_NextChange - m_LastChange).GetSeconds());

  ezVec3 vCurWind;

  if (fLerp >= 1.0f)
  {
    ComputeNextState();

    vCurWind = m_vLastDirection * m_fLastStrength;
  }
  else
  {
    const float fCurStrength = ezMath::Lerp(m_fLastStrength, m_fNextStrength, fLerp);
    const ezVec3 vCurDir = ezMath::Lerp(m_vLastDirection, m_vNextDirection, fLerp);

    vCurWind = vCurDir * fCurStrength;
  }

  pWindModule->SetFallbackWind(vCurWind);
}

void ezSimpleWindComponent::SerializeComponent(ezWorldWriter& stream) const
{
  SUPER::SerializeComponent(stream);
  auto& s = stream.GetStream();

  s << m_MinWindStrength;
  s << m_MaxWindStrength;
  s << m_Deviation;
}

void ezSimpleWindComponent::DeserializeComponent(ezWorldReader& stream)
{
  SUPER::DeserializeComponent(stream);
  const ezUInt32 uiVersion = stream.GetComponentTypeVersion(GetStaticRTTI());
  auto& s = stream.GetStream();

  if (uiVersion == 1)
  {
    float m_fWindStrengthMin, m_fWindStrengthMax;
    s >> m_fWindStrengthMin;
    s >> m_fWindStrengthMax;
  }
  else
  {
    s >> m_MinWindStrength;
    s >> m_MaxWindStrength;
  }

  s >> m_Deviation;
}

void ezSimpleWindComponent::OnActivated()
{
  SUPER::OnActivated();

  m_fNextStrength = ezWindStrength::GetInMetersPerSecond(m_MinWindStrength);
  m_vNextDirection = GetOwner()->GetGlobalDirForwards();
  m_NextChange = GetWorld()->GetClock().GetAccumulatedTime();
  m_LastChange = m_NextChange - ezTime::Seconds(1);

  ComputeNextState();
}

void ezSimpleWindComponent::OnDeactivated()
{
  SUPER::OnDeactivated();

  ezSimpleWindWorldModule* pWindModule = GetWorld()->GetModule<ezSimpleWindWorldModule>();

  if (pWindModule == nullptr)
    return;

  pWindModule->SetFallbackWind(ezVec3::ZeroVector());
}

void ezSimpleWindComponent::ComputeNextState()
{
  m_fLastStrength = m_fNextStrength;
  m_vLastDirection = m_vNextDirection;
  m_LastChange = GetWorld()->GetClock().GetAccumulatedTime();

  auto& rng = GetWorld()->GetRandomNumberGenerator();

  const ezEnum<ezWindStrength> minWind = ezMath::Min(m_MinWindStrength, m_MaxWindStrength);
  const ezEnum<ezWindStrength> maxWind = ezMath::Max(m_MinWindStrength, m_MaxWindStrength);

  const float fMinStrength = ezWindStrength::GetInMetersPerSecond(minWind);
  const float fMaxStrength = ezWindStrength::GetInMetersPerSecond(maxWind);

  float fStrengthDiff = fMaxStrength - fMinStrength;
  float fStrengthChange = fStrengthDiff * 0.2f;

  m_NextChange = m_LastChange + ezTime::Seconds(rng.DoubleMinMax(2.0f, 5.0f));
  m_fNextStrength = ezMath::Clamp<float>(m_fLastStrength + (float)rng.DoubleMinMax(-fStrengthChange, +fStrengthChange), fMinStrength, fMaxStrength);

  const ezVec3 vMainDir = GetOwner()->GetGlobalDirForwards();

  if (m_Deviation < ezAngle::Degree(1))
    m_vNextDirection = vMainDir;
  else
    m_vNextDirection = ezVec3::CreateRandomDeviation(rng, m_Deviation, vMainDir);

  ezCoordinateSystem cs;
  GetWorld()->GetCoordinateSystem(GetOwner()->GetGlobalPosition(), cs);
  const float fRemoveUp = m_vNextDirection.Dot(cs.m_vUpDir);

  m_vNextDirection -= cs.m_vUpDir * fRemoveUp;
  m_vNextDirection.NormalizeIfNotZero(ezVec3::ZeroVector()).IgnoreResult();
}

void ezSimpleWindComponent::Initialize()
{
  SUPER::Initialize();

  // make sure to query the wind interface before any simulation starts
  /*ezWindWorldModuleInterface* pWindInterface =*/GetWorld()->GetOrCreateModule<ezSimpleWindWorldModule>();
}



EZ_STATICLINK_FILE(GameEngine, GameEngine_Effects_Wind_Implementation_SimpleWindComponent);
