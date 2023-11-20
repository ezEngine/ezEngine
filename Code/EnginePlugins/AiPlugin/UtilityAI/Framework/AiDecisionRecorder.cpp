#include <AiPlugin/AiPluginPCH.h>

#include <AiPlugin/UtilityAI/Framework/AiBehaviorManager.h>
#include <AiPlugin/UtilityAI/Framework/AiConsideration.h>
#include <AiPlugin/UtilityAI/Framework/AiControllerDesc.h>
#include <AiPlugin/UtilityAI/Framework/AiDecisionRecorder.h>
#include <AiPlugin/UtilityAI/Framework/AiPerception.h>
#include <AiPlugin/UtilityAI/Framework/AiPerceptionManager.h>
#include <Core/World/GameObject.h>
#include <Foundation/IO/JSONWriter.h>

ezAiDecisionRecorder::ezAiDecisionRecorder() = default;
ezAiDecisionRecorder::~ezAiDecisionRecorder() = default;

ezAiDecisionRecorder::Event& ezAiDecisionRecorder::AddEvent(EventType type, ezTime now, const ezGameObject& owner)
{
  if (m_Events.GetCount() >= c_uiMaxEvents)
  {
    m_Events.PopFront();
  }

  Event& e = m_Events.ExpandAndGetRef();
  e.m_Type = type;
  e.m_Time = now;
  e.m_vPosition = owner.GetGlobalPosition();
  e.m_vForward = owner.GetGlobalDirForwards().GetAsVec2();
  e.m_vForward.NormalizeIfNotZero(ezVec2::MakeZero()).IgnoreResult();
  e.m_uiWinner = ezInvalidIndex;
  e.m_Behaviors.Clear();
  e.m_Perceptions.Clear();
  return e;
}

void ezAiDecisionRecorder::RecordDecision(ezTime now, const ezGameObject& owner, const ezAiBehaviorManager& behaviors, const ezAiPerceptionManager& perceptions, ezUInt32 uiActiveBefore, float fActiveScore, const ezAiBehaviorCandidate& candidate, Result result, ezStringView sAction)
{
  Event& e = AddEvent(EventType::Decision, now, owner);
  e.m_Result = result;
  e.m_uiActiveBehavior = uiActiveBefore;
  e.m_uiWinner = candidate.m_uiBehaviorIndex;
  e.m_fWinnerScore = candidate.m_fScore;
  e.m_fActiveScore = fActiveScore;
  e.m_sAction = sAction;

  e.m_Behaviors.SetCount(behaviors.GetBehaviorCount());
  for (ezUInt32 i = 0; i < behaviors.GetBehaviorCount(); ++i)
  {
    const auto& state = behaviors.GetBehaviorState(i);
    BehaviorEntry& b = e.m_Behaviors[i];

    if (behaviors.WasAvailable(i))
    {
      b.m_State = BehaviorState::Scored;
      b.m_fScore = state.m_fLastScore;
      b.m_ConsiderationValues = state.m_LastConsiderationValues;
      b.m_bHasCandidatePosition = state.m_bLastCandidateHasPosition;
      b.m_vCandidatePosition = state.m_vLastCandidatePosition;
    }
    else
    {
      b.m_State = (state.m_CooldownUntil > now) ? BehaviorState::Cooldown : BehaviorState::NotAvailable;
      b.m_fScore = 0.0f;
      b.m_ConsiderationValues.Clear();
      b.m_bHasCandidatePosition = false;
    }
  }

  ezHybridArray<const ezAiPerception*, 16> allPerceptions;
  perceptions.GetAllPerceptions(allPerceptions);

  ezStringBuilder sDesc;
  e.m_Perceptions.SetCount(allPerceptions.GetCount());
  for (ezUInt32 i = 0; i < allPerceptions.GetCount(); ++i)
  {
    PerceptionEntry& p = e.m_Perceptions[i];
    p.m_pType = allPerceptions[i]->GetDynamicRTTI();
    p.m_bHasPosition = allPerceptions[i]->GetDebugPosition(p.m_vPosition);

    sDesc.Clear();
    allPerceptions[i]->GetDebugDesc(sDesc);
    p.m_sDesc = sDesc;
  }
}

void ezAiDecisionRecorder::RecordBehaviorEnd(ezTime now, const ezGameObject& owner, ezUInt32 uiBehavior, ezStringView sAction)
{
  Event& e = AddEvent(EventType::BehaviorEnd, now, owner);
  e.m_uiActiveBehavior = uiBehavior;
  e.m_sAction = sAction;
}

void ezAiDecisionRecorder::RecordActionFailure(ezTime now, const ezGameObject& owner, ezUInt32 uiBehavior, ezStringView sAction)
{
  Event& e = AddEvent(EventType::ActionFailed, now, owner);
  e.m_uiActiveBehavior = uiBehavior;
  e.m_sAction = sAction;
}

void ezAiDecisionRecorder::Clear()
{
  m_Events.Clear();
}

void ezAiDecisionRecorder::WriteJson(ezJSONWriter& inout_writer, const ezAiControllerDesc& desc) const
{
  const auto descBehaviors = desc.GetBehaviors();

  auto GetName = [&](ezUInt32 uiBehavior) -> ezStringView
  { return uiBehavior < descBehaviors.GetCount() ? descBehaviors[uiBehavior].m_pBehavior->GetName() : ezStringView("<none>"); };

  auto WritePosition = [&](ezStringView sName, const ezVec3& v)
  {
    inout_writer.BeginArray(sName);
    inout_writer.WriteFloat(v.x);
    inout_writer.WriteFloat(v.y);
    inout_writer.WriteFloat(v.z);
    inout_writer.EndArray();
  };

  // ezAiPerceptionTarget -> Target
  auto GetPerceptionName = [](const ezRTTI* pType) -> ezStringView
  {
    ezStringView sName = pType->GetTypeName();
    sName.TrimWordStart("ezAiPerception");
    return sName;
  };

  // the names of the considerations of each behavior, in the order of the values in the events
  inout_writer.BeginObject("considerations");
  for (ezUInt32 i = 0; i < descBehaviors.GetCount(); ++i)
  {
    inout_writer.BeginArray(GetName(i));
    for (ezUInt32 uiConsideration : descBehaviors[i].m_pBehavior->m_Considerations.GetConnections())
    {
      inout_writer.WriteString(desc.GetConsiderations()[uiConsideration].m_pConsideration->GetDisplayName());
    }
    inout_writer.EndArray();
  }
  inout_writer.EndObject();

  inout_writer.BeginArray("events");

  for (const Event& e : m_Events)
  {
    inout_writer.BeginObject();
    inout_writer.AddVariableFloat("t", e.m_Time.AsFloatInSeconds());
    WritePosition("pos", e.m_vPosition);

    inout_writer.BeginArray("dir");
    inout_writer.WriteFloat(e.m_vForward.x);
    inout_writer.WriteFloat(e.m_vForward.y);
    inout_writer.EndArray();

    if (e.m_Type == EventType::BehaviorEnd || e.m_Type == EventType::ActionFailed)
    {
      inout_writer.AddVariableString("type", e.m_Type == EventType::BehaviorEnd ? "end" : "actionFailed");
      inout_writer.AddVariableString("behavior", GetName(e.m_uiActiveBehavior));
      inout_writer.AddVariableString("action", e.m_sAction);
    }
    else
    {
      static constexpr const char* s_szResults[] = {"noCandidate", "kept", "switched", "notEnough"};

      inout_writer.AddVariableString("type", "decision");
      inout_writer.AddVariableString("active", GetName(e.m_uiActiveBehavior));
      inout_writer.AddVariableFloat("activeScore", e.m_fActiveScore);
      inout_writer.AddVariableString("winner", GetName(e.m_uiWinner));
      inout_writer.AddVariableFloat("winnerScore", e.m_fWinnerScore);
      inout_writer.AddVariableString("result", s_szResults[(ezUInt32)e.m_Result]);
      inout_writer.AddVariableString("action", e.m_sAction);

      inout_writer.BeginObject("behaviors");
      for (ezUInt32 i = 0; i < e.m_Behaviors.GetCount(); ++i)
      {
        const BehaviorEntry& b = e.m_Behaviors[i];

        if (b.m_State == BehaviorState::Cooldown)
        {
          inout_writer.AddVariableString(GetName(i), "cooldown");
          continue;
        }

        if (b.m_State == BehaviorState::NotAvailable)
          continue;

        // no candidate passed the filter, not interesting unless it is involved in the decision
        if (b.m_fScore <= 0.0f && b.m_ConsiderationValues.IsEmpty() && i != e.m_uiActiveBehavior && i != e.m_uiWinner)
          continue;

        inout_writer.BeginObject(GetName(i));
        inout_writer.AddVariableFloat("score", b.m_fScore);

        if (!b.m_ConsiderationValues.IsEmpty())
        {
          // -1: skipped, because the score couldn't win anymore
          inout_writer.BeginArray("considerations");
          for (float f : b.m_ConsiderationValues)
          {
            inout_writer.WriteFloat(f);
          }
          inout_writer.EndArray();
        }

        if (b.m_bHasCandidatePosition)
        {
          WritePosition("candidate", b.m_vCandidatePosition);
        }

        inout_writer.EndObject();
      }
      inout_writer.EndObject();

      if (!e.m_Perceptions.IsEmpty())
      {
        inout_writer.BeginArray("perceptions");
        for (const PerceptionEntry& p : e.m_Perceptions)
        {
          inout_writer.BeginObject();
          inout_writer.AddVariableString("type", GetPerceptionName(p.m_pType));

          if (p.m_bHasPosition)
          {
            WritePosition("pos", p.m_vPosition);
          }

          if (!p.m_sDesc.IsEmpty())
          {
            inout_writer.AddVariableString("info", p.m_sDesc);
          }

          inout_writer.EndObject();
        }
        inout_writer.EndArray();
      }
    }

    inout_writer.EndObject();
  }

  inout_writer.EndArray();
}
