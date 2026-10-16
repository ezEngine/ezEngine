#include <AiPlugin/AiPluginPCH.h>

#include <AiPlugin/UtilityAI/Components/AiComponent.h>
#include <AiPlugin/UtilityAI/Impl/AlBotBehaviors.h>
#include <AiPlugin/UtilityAI/Impl/GameAiBehaviors.h>
#include <AiPlugin/UtilityAI/Impl/GameAiPerceptionGenerators.h>
#include <AiPlugin/UtilityAI/Impl/GameAiSensors.h>
#include <Core/Interfaces/PhysicsWorldModule.h>
#include <Core/WorldSerializer/WorldReader.h>
#include <Core/WorldSerializer/WorldWriter.h>
#include <GameEngine/Messages/DamageMessage.h>
#include <RendererCore/Debug/DebugRenderer.h>

// clang-format off
EZ_BEGIN_STATIC_REFLECTED_ENUM(ezAiBehaviorSet, 1)
  EZ_ENUM_CONSTANTS(ezAiBehaviorSet::Prototype, ezAiBehaviorSet::AlBot)
EZ_END_STATIC_REFLECTED_ENUM;

EZ_BEGIN_COMPONENT_TYPE(ezAiComponent, 2, ezComponentMode::Dynamic)
{
  EZ_BEGIN_PROPERTIES
  {
    EZ_ENUM_MEMBER_PROPERTY("BehaviorSet", ezAiBehaviorSet, m_BehaviorSet),
    EZ_MEMBER_PROPERTY("DecisionInterval", m_DecisionInterval)->AddAttributes(new ezDefaultValueAttribute(ezTime::Seconds(0.5)), new ezClampValueAttribute(ezTime::MakeZero(), ezVariant())),
    EZ_MEMBER_PROPERTY("DebugInfo", m_bDebugInfo),
  }
  EZ_END_PROPERTIES;

  EZ_BEGIN_MESSAGEHANDLERS
  {
    EZ_MESSAGE_HANDLER(ezMsgDamage, OnMsgDamage),
  }
  EZ_END_MESSAGEHANDLERS;

  EZ_BEGIN_ATTRIBUTES
  {
    new ezCategoryAttribute("AI/Components"),
  }
  EZ_END_ATTRIBUTES;
}
EZ_END_COMPONENT_TYPE
// clang-format on

ezAiComponent::ezAiComponent() = default;
ezAiComponent::~ezAiComponent() = default;

void ezAiComponent::SerializeComponent(ezWorldWriter& inout_stream) const
{
  SUPER::SerializeComponent(inout_stream);
  auto& s = inout_stream.GetStream();

  s << m_bDebugInfo;

  // version 2
  s << m_BehaviorSet;
  s << m_DecisionInterval;
}

void ezAiComponent::DeserializeComponent(ezWorldReader& inout_stream)
{
  SUPER::DeserializeComponent(inout_stream);
  const ezUInt32 uiVersion = inout_stream.GetComponentTypeVersion(GetStaticRTTI());
  auto& s = inout_stream.GetStream();

  s >> m_bDebugInfo;

  if (uiVersion >= 2)
  {
    s >> m_BehaviorSet;
    s >> m_DecisionInterval;
  }
}

void ezAiComponent::OnSimulationStarted()
{
  SUPER::OnSimulationStarted();

  switch (m_BehaviorSet)
  {
    case ezAiBehaviorSet::Prototype:
      SetupPrototype();
      break;

    case ezAiBehaviorSet::AlBot:
      SetupAlBot();
      break;
  }

  // stagger the decisions of multiple agents
  const double fOffset = GetWorld()->GetRandomNumberGenerator().DoubleZeroToOneExclusive();
  m_LastAiUpdate = GetWorld()->GetClock().GetAccumulatedTime() - m_DecisionInterval * fOffset;
}

void ezAiComponent::SetupPrototype()
{
  m_SensorManager.AddSensor("Sensor_See", EZ_DEFAULT_NEW(ezAiSensorSpatial, ezTempHashedString("Sensor_POI")));
  m_PerceptionManager.AddGenerator(EZ_DEFAULT_NEW(ezAiPerceptionGenPOI));
  m_PerceptionManager.AddGenerator(EZ_DEFAULT_NEW(ezAiPerceptionGenWander));
  m_PerceptionManager.AddGenerator(EZ_DEFAULT_NEW(ezAiPerceptionGenCheckpoint));
  m_BehaviorManager.AddBehavior(EZ_DEFAULT_NEW(ezAiBehaviorGoToPOI));
  // m_BehaviorManager.AddBehavior(EZ_DEFAULT_NEW(ezAiBehaviorWander));
  m_BehaviorManager.AddBehavior(EZ_DEFAULT_NEW(ezAiBehaviorGoToCheckpoint));
  // m_BehaviorManager.AddBehavior(EZ_DEFAULT_NEW(ezAiBehaviorShoot));
  m_BehaviorManager.AddBehavior(EZ_DEFAULT_NEW(ezAiBehaviorQuip));
}

void ezAiComponent::SetupAlBot()
{
  m_SensorManager.AddSensor("FeelSensor", EZ_DEFAULT_NEW(ezAiSensorSpatial, ezTempHashedString("FeelSensor")));
  m_SensorManager.AddSensor("VisionSensor", EZ_DEFAULT_NEW(ezAiSensorSpatial, ezTempHashedString("VisionSensor")));

  {
    auto pGen = EZ_DEFAULT_NEW(ezAiPerceptionGenTarget);
    pGen->m_SensorNames.PushBack("FeelSensor");
    pGen->m_SensorNames.PushBack("VisionSensor");
    pGen->m_sDamageSourceGlobalKey = "Player";
    m_PerceptionManager.AddGenerator(pGen);
  }

  m_BehaviorManager.AddBehavior(EZ_DEFAULT_NEW(ezAiBehaviorAlIdle));
  m_BehaviorManager.AddBehavior(EZ_DEFAULT_NEW(ezAiBehaviorAlPatrol));
  m_BehaviorManager.AddBehavior(EZ_DEFAULT_NEW(ezAiBehaviorAlInvestigate));
  m_BehaviorManager.AddBehavior(EZ_DEFAULT_NEW(ezAiBehaviorAlSearch));
  m_BehaviorManager.AddBehavior(EZ_DEFAULT_NEW(ezAiBehaviorAlEngage));
  m_BehaviorManager.AddBehavior(EZ_DEFAULT_NEW(ezAiBehaviorAlShoot));
  m_BehaviorManager.AddBehavior(EZ_DEFAULT_NEW(ezAiBehaviorAlMelee));
}

void ezAiComponent::OnDeactivated()
{
  m_ActionQueue.InterruptAndClear();

  SUPER::OnDeactivated();
}

void ezAiComponent::OnMsgDamage(ezMsgDamage& ref_msg)
{
  m_PerceptionManager.HandleMessage(*GetOwner(), ref_msg);
}

void ezAiComponent::Update()
{
  // how much better a behavior in the same category has to be, to replace the active one
  constexpr float fSameCategorySwitchMargin = 0.1f;

  const ezTime tNow = GetWorld()->GetClock().GetAccumulatedTime();

  const auto cs = m_BehaviorManager.ContinueActiveBehavior(*GetOwner(), m_ActionQueue);

  if (cs.m_bEndBehavior)
  {
    m_fLastScore = 0.0f;
    m_BehaviorManager.SetActiveBehavior(*GetOwner(), nullptr, nullptr, m_ActionQueue);

    // decide on the next behavior right away
    m_LastAiUpdate = tNow - m_DecisionInterval;
  }

  if (cs.m_bAllowBehaviorSwitch && tNow >= m_LastAiUpdate + m_DecisionInterval)
  {
    if (!m_BehaviorManager.HasActiveBehavior())
    {
      m_fLastScore = 0.0f;
    }

    m_LastAiUpdate = tNow;

    m_BehaviorManager.DetermineAvailableBehaviors(m_LastAiUpdate, ezMath::Floor(m_fLastScore));

    m_BehaviorManager.FlagNeededPerceptions(m_PerceptionManager);

    m_PerceptionManager.FlagNeededSensors(m_SensorManager);

    m_SensorManager.UpdateNeededSensors(*GetOwner());

    m_PerceptionManager.UpdateNeededPerceptions(*GetOwner(), m_SensorManager);

    const ezAiBehaviorCandidate candidate = m_BehaviorManager.DetermineBehaviorCandidate(*GetOwner(), m_PerceptionManager);

    if (candidate.m_fActiveBehaviorScore >= 0.0f)
    {
      // the active behavior was re-evaluated, its score may have dropped
      m_fLastScore = candidate.m_fActiveBehaviorScore;
    }

    if (candidate.m_pBehavior == nullptr)
    {
      // nothing wants to run, keep the active behavior (if any)
    }
    else if (candidate.m_pBehavior == m_BehaviorManager.GetActiveBehavior())
    {
      m_fLastScore = candidate.m_fScore;
      m_BehaviorManager.KeepActiveBehavior(*GetOwner(), candidate.m_pPerception, m_ActionQueue);
    }
    else if (candidate.m_fScore > ezMath::Ceil(m_fLastScore) || candidate.m_fScore > m_fLastScore + fSameCategorySwitchMargin)
    {
      m_fLastScore = candidate.m_fScore;
      if (m_bDebugInfo)
      {
        ezAiBehavior* pPrev = m_BehaviorManager.GetActiveBehavior();
        ezLog::Info("AI {}: {} -> {} ({})", ezArgP(this), pPrev ? pPrev->GetName() : "<none>", candidate.m_pBehavior->GetName(), ezArgF(candidate.m_fScore, 2));
      }

      m_BehaviorManager.SetActiveBehavior(*GetOwner(), candidate.m_pBehavior, candidate.m_pPerception, m_ActionQueue);
    }
  }

  m_ActionQueue.Execute(*GetOwner(), GetWorld()->GetClock().GetTimeDiff(), ezLog::GetThreadLocalLogSystem());

  if (m_bDebugInfo)
  {
    DrawDebugInfo();
  }
}

void ezAiComponent::DrawDebugInfo()
{
  ezStringBuilder sText, sTmp;

  if (ezAiBehavior* pActive = m_BehaviorManager.GetActiveBehavior())
  {
    sText.SetFormat("{} ({})\n", pActive->GetName(), ezArgF(m_fLastScore, 2));
  }
  else
  {
    sText = "<no behavior>\n";
  }

  m_ActionQueue.GetDebugDesc(sTmp);
  sText.AppendFormat("{}\n\n", sTmp);

  m_BehaviorManager.GetDebugText(sText);

  ezDebugRenderer::Draw3DText(GetWorld(), sText, GetOwner()->GetGlobalPosition() + ezVec3(0, 0, 1.0f), ezColor::White, 16);
}
