#pragma once

#include <AiPlugin/AiPluginDLL.h>
#include <Foundation/Containers/Deque.h>
#include <Foundation/Containers/SmallArray.h>
#include <Foundation/Math/Vec2.h>
#include <Foundation/Math/Vec3.h>
#include <Foundation/Strings/String.h>
#include <Foundation/Time/Time.h>

class ezAiBehaviorManager;
class ezAiControllerDesc;
class ezAiPerceptionManager;
class ezGameObject;
class ezJSONWriter;
struct ezAiBehaviorCandidate;

/// Keeps the last decisions of one agent, so that they can be written to a file and looked at afterwards (see ezAiComponentManager).
///
/// Records one event per decision update, with the score and consideration results of every behavior and all perceptions,
/// one when a behavior ends itself and one when an action fails.
/// Only the newest events are kept (see c_uiMaxEvents), so recording can stay on all the time.
class EZ_AIPLUGIN_DLL ezAiDecisionRecorder
{
public:
  ezAiDecisionRecorder();
  ~ezAiDecisionRecorder();

  static constexpr ezUInt32 c_uiMaxEvents = 512;

  /// What the decision update did with the winning behavior.
  enum class Result : ezUInt8
  {
    NoCandidate, ///< No behavior scored above 0, the active one stays.
    Kept,        ///< The active behavior won again.
    Switched,    ///< Another behavior won and replaced the active one.
    NotEnough,   ///< Another behavior won, but not by enough to replace the active one.
  };

  void RecordDecision(ezTime now, const ezGameObject& owner, const ezAiBehaviorManager& behaviors, const ezAiPerceptionManager& perceptions, ezUInt32 uiActiveBefore, float fActiveScore, const ezAiBehaviorCandidate& candidate, Result result, ezStringView sAction);

  /// The behavior returned m_bEndBehavior from ContinueBehavior().
  void RecordBehaviorEnd(ezTime now, const ezGameObject& owner, ezUInt32 uiBehavior, ezStringView sAction);

  /// An action returned Failed, which cancels the whole queue.
  void RecordActionFailure(ezTime now, const ezGameObject& owner, ezUInt32 uiBehavior, ezStringView sAction);

  void Clear();

  /// Writes the events as a JSON object (the caller has to open it). Behavior and consideration names are taken from desc.
  ///
  /// Behaviors that scored 0 without any consideration results (no candidate passed their filter) are left out, unless they were active or won.
  void WriteJson(ezJSONWriter& inout_writer, const ezAiControllerDesc& desc) const;

private:
  enum class EventType : ezUInt8
  {
    Decision,
    BehaviorEnd,
    ActionFailed,
  };

  enum class BehaviorState : ezUInt8
  {
    Scored,
    Cooldown,
    NotAvailable,
  };

  struct BehaviorEntry
  {
    BehaviorState m_State = BehaviorState::NotAvailable;
    bool m_bHasCandidatePosition = false;
    float m_fScore = 0.0f;
    ezVec3 m_vCandidatePosition = ezVec3::MakeZero();
    ezSmallArray<float, 4> m_ConsiderationValues;
  };

  struct PerceptionEntry
  {
    const ezRTTI* m_pType = nullptr;
    bool m_bHasPosition = false;
    ezVec3 m_vPosition = ezVec3::MakeZero();
    ezString m_sDesc;
  };

  struct Event
  {
    EventType m_Type = EventType::Decision;
    Result m_Result = Result::NoCandidate;
    ezTime m_Time;
    ezVec3 m_vPosition = ezVec3::MakeZero();
    ezVec2 m_vForward = ezVec2::MakeZero();
    ezUInt32 m_uiActiveBehavior = ezInvalidIndex; ///< Before the decision, or the behavior that ended or whose action failed.
    ezUInt32 m_uiWinner = ezInvalidIndex;
    float m_fWinnerScore = 0.0f;
    float m_fActiveScore = 0.0f;                  ///< The score the winner had to beat (plus the switch margin).
    ezString m_sAction;                           ///< Debug description of the action queue.
    ezSmallArray<BehaviorEntry, 12> m_Behaviors;
    ezSmallArray<PerceptionEntry, 4> m_Perceptions;
  };

  Event& AddEvent(EventType type, ezTime now, const ezGameObject& owner);

  ezDeque<Event> m_Events;
};
