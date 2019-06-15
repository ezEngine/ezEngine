#include <GameEnginePCH.h>

#include <Core/WorldSerializer/WorldReader.h>
#include <Core/WorldSerializer/WorldWriter.h>
#include <GameEngine/Effects/Wind/SimpleWindComponent.h>
#include <GameEngine/Interfaces/WindWorldModule.h>

// clang-format off
EZ_BEGIN_COMPONENT_TYPE(ezSimpleWindComponent, 1, ezComponentMode::Static)
{
  EZ_BEGIN_PROPERTIES
  {
    EZ_MEMBER_PROPERTY("MinStrength", m_fWindStrengthMin)->AddAttributes(new ezClampValueAttribute(0.0f, ezVariant())),
    EZ_MEMBER_PROPERTY("MaxStrength", m_fWindStrengthMax)->AddAttributes(new ezClampValueAttribute(0.0f, ezVariant())),
    EZ_MEMBER_PROPERTY("Deviation", m_Deviation)->AddAttributes(new ezClampValueAttribute(ezAngle::Degree(0), ezAngle::Degree(180))),
  }
  EZ_END_PROPERTIES;
  EZ_BEGIN_ATTRIBUTES
  {
    new ezCategoryAttribute("Effects"),
    new ezDirectionVisualizerAttribute(ezBasisAxis::PositiveX, 0.2f, ezColor::DodgerBlue),
  }
  EZ_END_ATTRIBUTES;
}
EZ_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

ezSimpleWindComponent::ezSimpleWindComponent() = default;
ezSimpleWindComponent::~ezSimpleWindComponent() = default;

void ezSimpleWindComponent::Update()
{
  ezWindWorldModuleInterface* pWindInterface = GetWorld()->GetModule<ezWindWorldModuleInterface>();

  if (pWindInterface == nullptr)
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

  pWindInterface->SetFallbackWind(vCurWind);
}

void ezSimpleWindComponent::SerializeComponent(ezWorldWriter& stream) const
{
  SUPER::SerializeComponent(stream);
  auto& s = stream.GetStream();

  s << m_fWindStrengthMin;
  s << m_fWindStrengthMax;
  s << m_Deviation;
}

void ezSimpleWindComponent::DeserializeComponent(ezWorldReader& stream)
{
  SUPER::DeserializeComponent(stream);
  //const ezUInt32 uiVersion = stream.GetComponentTypeVersion(GetStaticRTTI());
  auto& s = stream.GetStream();

  s >> m_fWindStrengthMin;
  s >> m_fWindStrengthMax;
  s >> m_Deviation;
}

void ezSimpleWindComponent::OnActivated()
{
  SUPER::OnActivated();

  m_fNextStrength = m_fWindStrengthMin;
  m_vNextDirection = GetOwner()->GetGlobalDirForwards();
  m_NextChange = GetWorld()->GetClock().GetAccumulatedTime();

  ComputeNextState();
}

void ezSimpleWindComponent::OnDeactivated()
{
  SUPER::OnDeactivated();

  ezWindWorldModuleInterface* pWindInterface = GetWorld()->GetModule<ezWindWorldModuleInterface>();

  if (pWindInterface == nullptr)
    return;

  pWindInterface->SetFallbackWind(ezVec3::ZeroVector());
}

void ezSimpleWindComponent::ComputeNextState()
{
  m_fLastStrength = m_fNextStrength;
  m_vLastDirection = m_vNextDirection;
  m_LastChange = GetWorld()->GetClock().GetAccumulatedTime();

  auto& rng = GetWorld()->GetRandomNumberGenerator();

  if (m_fWindStrengthMax < m_fWindStrengthMin)
    ezMath::Swap(m_fWindStrengthMax, m_fWindStrengthMin);

  float fStrengthDiff = m_fWindStrengthMax - m_fWindStrengthMin;
  float fStrengthChange = fStrengthDiff * 0.2f;

  m_NextChange = m_LastChange + ezTime::Seconds(rng.DoubleMinMax(2.0f, 5.0f));
  m_fNextStrength = ezMath::Clamp<float>(
    m_fLastStrength + (float)rng.DoubleMinMax(-fStrengthChange, +fStrengthChange), m_fWindStrengthMin, m_fWindStrengthMax);

  const ezVec3 vMainDir = GetOwner()->GetGlobalDirForwards();

  if (m_Deviation < ezAngle::Degree(1))
    m_vNextDirection = vMainDir;
  else
    m_vNextDirection = ezVec3::CreateRandomDeviation(rng, m_Deviation, vMainDir);

  ezCoordinateSystem cs;
  GetWorld()->GetCoordinateSystem(GetOwner()->GetGlobalPosition(), cs);
  const float fRemoveUp = m_vNextDirection.Dot(cs.m_vUpDir);

  m_vNextDirection -= cs.m_vUpDir * fRemoveUp;
  m_vNextDirection.NormalizeIfNotZero(ezVec3::ZeroVector());
}

void ezSimpleWindComponent::Initialize()
{
  SUPER::Initialize();

  // make sure to query the wind interface before any simulation starts
  /*ezWindWorldModuleInterface* pWindInterface =*/ GetWorld()->GetOrCreateModule<ezWindWorldModuleInterface>();
}



EZ_STATICLINK_FILE(GameEngine, GameEngine_Components_Implementation_SimpleWindComponent);
