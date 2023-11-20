#include <AiPlugin/AiPluginPCH.h>

#include <AiPlugin/UtilityAI/Components/AiComponent.h>
#include <AiPlugin/UtilityAI/Impl/GameAiBehaviors.h>
#include <AiPlugin/UtilityAI/Impl/GameAiPerceptionGenerators.h>
#include <AiPlugin/UtilityAI/Impl/GameAiSensors.h>
#include <Core/Interfaces/PhysicsWorldModule.h>
#include <Core/WorldSerializer/WorldReader.h>
#include <Core/WorldSerializer/WorldWriter.h>
#include <RendererCore/Debug/DebugRenderer.h>

// clang-format off
//////////////////////////////////////////////////////////////////////////

EZ_BEGIN_COMPONENT_TYPE(ezAiComponent, 1, ezComponentMode::Dynamic)
{
  EZ_BEGIN_PROPERTIES
  {
    EZ_MEMBER_PROPERTY("DebugInfo", m_bDebugInfo),
  }
  EZ_END_PROPERTIES;

  EZ_BEGIN_ATTRIBUTES
  {
    new ezCategoryAttribute("AI/Components"),
  }
  EZ_END_ATTRIBUTES;
}
EZ_END_ABSTRACT_COMPONENT_TYPE
// clang-format on

ezAiComponent::ezAiComponent() = default;
ezAiComponent::~ezAiComponent() = default;

void ezAiComponent::SerializeComponent(ezWorldWriter& inout_stream) const
{
  SUPER::SerializeComponent(inout_stream);
  auto& s = inout_stream.GetStream();

  s << m_bDebugInfo;
}

void ezAiComponent::DeserializeComponent(ezWorldReader& inout_stream)
{
  SUPER::DeserializeComponent(inout_stream);
  // const ezUInt32 uiVersion = stream.GetComponentTypeVersion(GetStaticRTTI());
  auto& s = inout_stream.GetStream();

  s >> m_bDebugInfo;
}

void ezAiComponent::OnSimulationStarted()
{
  SUPER::OnSimulationStarted();

  m_SensorManager.AddSensor("Sensor_See", EZ_DEFAULT_NEW(ezAiSensorSpatial, ezTempHashedString("Sensor_POI")));
  m_PerceptionManager.AddGenerator(EZ_DEFAULT_NEW(ezAiPerceptionGenPOI));
  m_PerceptionManager.AddGenerator(EZ_DEFAULT_NEW(ezAiPerceptionGenWander));
  m_PerceptionManager.AddGenerator(EZ_DEFAULT_NEW(ezAiPerceptionGenCheckpoint));
  m_BehaviorManager.AddBehavior(EZ_DEFAULT_NEW(ezAiBehaviorGoToPOI));
  //m_BehaviorManager.AddBehavior(EZ_DEFAULT_NEW(ezAiBehaviorWander));
  m_BehaviorManager.AddBehavior(EZ_DEFAULT_NEW(ezAiBehaviorGoToCheckpoint));
  // m_BehaviorManager.AddBehavior(EZ_DEFAULT_NEW(ezAiBehaviorShoot));
  m_BehaviorManager.AddBehavior(EZ_DEFAULT_NEW(ezAiBehaviorQuip));
}

void ezAiComponent::OnDeactivated()
{
  m_ActionQueue.InterruptAndClear();

  SUPER::OnDeactivated();
}

void ezAiComponent::Update()
{
  const auto cs = m_BehaviorManager.ContinueActiveBehavior(*GetOwner(), m_ActionQueue);

  if (cs.m_bEndBehavior)
  {
    m_fLastScore = 0.0f;
    m_BehaviorManager.SetActiveBehavior(*GetOwner(), nullptr, nullptr, m_ActionQueue);
  }

  if (cs.m_bAllowBehaviorSwitch && GetWorld()->GetClock().GetAccumulatedTime() > m_LastAiUpdate + ezTime::Seconds(0.5))
  {
    if (!m_BehaviorManager.HasActiveBehavior())
    {
      m_fLastScore = 0.0f;
    }

    m_LastAiUpdate = GetWorld()->GetClock().GetAccumulatedTime();

    m_BehaviorManager.DetermineAvailableBehaviors(m_LastAiUpdate, ezMath::Floor(m_fLastScore));

    m_BehaviorManager.FlagNeededPerceptions(m_PerceptionManager);

    m_PerceptionManager.FlagNeededSensors(m_SensorManager);

    m_SensorManager.UpdateNeededSensors(*GetOwner());

    m_PerceptionManager.UpdateNeededPerceptions(*GetOwner(), m_SensorManager);

    const ezAiBehaviorCandidate candidate = m_BehaviorManager.DetermineBehaviorCandidate(*GetOwner(), m_PerceptionManager);

    if (candidate.m_pBehavior == m_BehaviorManager.GetActiveBehavior())
    {
      m_fLastScore = candidate.m_fScore;
      m_BehaviorManager.KeepActiveBehavior(*GetOwner(), candidate.m_pPerception, m_ActionQueue);
    }
    else if (candidate.m_fScore > ezMath::Ceil(m_fLastScore))
    {
      m_fLastScore = candidate.m_fScore;
      m_BehaviorManager.SetActiveBehavior(*GetOwner(), candidate.m_pBehavior, candidate.m_pPerception, m_ActionQueue);
    }
  }

  m_ActionQueue.Execute(*GetOwner(), GetWorld()->GetClock().GetTimeDiff(), ezLog::GetThreadLocalLogSystem());

  if (m_bDebugInfo)
  {
    m_ActionQueue.PrintDebugInfo(*GetOwner());
  }
}
