#pragma once

#include <AiPlugin/AiPluginDLL.h>
#include <AiPlugin/UtilityAI/Framework/AiActionQueue.h>
#include <AiPlugin/UtilityAI/Framework/AiContext.h>
#include <AiPlugin/UtilityAI/Framework/AiInputPin.h>
#include <AiPlugin/UtilityAI/Framework/AiPerceptionManager.h>
#include <Foundation/Reflection/Reflection.h>
#include <GameEngine/GameEngineDLL.h>

class ezAiBehavior;
class ezAiPerceptionManager;
class ezAiPerception;
struct ezInstanceDataDesc;

/// Rough priority of a behavior. The category is the integer part of a behavior score, see ezAiBehaviorScore.
///
/// A behavior in a higher category always wins over one in a lower category.
struct ezAiScoreCategory
{
  using StorageType = ezUInt8;

  enum Enum : StorageType
  {
    Fallback,    ///< if really nothing else is available
    Idle,        ///< Idle actions (simple animation playback and such)
    ActiveIdle,  ///< The main actions to do when an NPC is idle, e.g. wander around, follow a patrol path
    Investigate, ///< in case something interesting is detected
    Command,     ///< things the player (or level designer) instructs the NPC to do that should have high priority
    Combat,      ///< combat related behavior
    Interrupt,   ///< things that even override combat scenarios (hit reactions, falling down, etc)

    Default = Idle
  };
};

EZ_DECLARE_REFLECTABLE_TYPE(EZ_AIPLUGIN_DLL, ezAiScoreCategory);

/// Result of ezAiBehavior::DetermineBehaviorScore().
///
/// A default constructed score is 0, which means 'don't run this behavior'.
class ezAiBehaviorScore
{
public:
  ezAiBehaviorScore() {}

  /// The final score is the category plus fValue (0 to 1), so e.g. Combat with 0.5 results in 5.5.
  void SetScore(ezAiScoreCategory::Enum category, float fValue)
  {
    EZ_ASSERT_DEBUG(fValue >= 0.0f && fValue <= 1.0f, "Value is out of 0-1 range.");
    m_fValue = static_cast<float>(category) + fValue;
  }

  float GetScore() const { return m_fValue; }

  /// The perception that made the behavior score this high (e.g. the target to attack).
  /// Passed to OnActivateBehavior() / OnReactivateBehavior() if the behavior gets chosen. May be null.
  const ezAiPerception* m_pPerception = nullptr;

  /// Debug output: the result of each consideration (in the order of ezAiBehavior::m_Considerations), for m_pPerception.
  /// -1 for considerations that were skipped, because the score couldn't win anymore.
  ezSmallArray<float, 4> m_ConsiderationValues;

private:
  float m_fValue = 0.0f;
};

/// The behavior with the highest score in one decision update, see ezAiBehaviorManager::DetermineBehaviorCandidate().
struct ezAiBehaviorCandidate
{
  ezUInt32 m_uiBehaviorIndex = ezInvalidIndex; ///< Index in ezAiControllerDesc::GetBehaviors(), ezInvalidIndex if no behavior scored above 0.
  const ezAiBehavior* m_pBehavior = nullptr;
  const ezAiPerception* m_pPerception = nullptr;
  float m_fScore = 0.0f;

  /// Score of the currently active behavior in the same update, or -1 if it wasn't evaluated (e.g. due to cooldown).
  float m_fActiveBehaviorScore = -1.0f;
};

/// One thing an NPC can decide to do, e.g. 'patrol', 'investigate a noise' or 'shoot at the target'.
///
/// Each decision update (see ezAiComponent), all available behaviors compute a score from the current perceptions.
/// The one with the highest score becomes the active behavior, which then fills the ezAiActionQueue with actions.
/// Only one behavior is active at a time.
///
/// Call order:
///  - Decision update: IsAvailable() -> FlagNeededPerceptions() -> DetermineBehaviorScore().
///    If the active behavior wins again: OnReactivateBehavior().
///    If another one wins: OnDeactivateBehavior() on the old one, OnActivateBehavior() on the new one.
///  - Every frame: ContinueBehavior() on the active behavior.
///
/// A behavior object is shared by all agents of the same type and must not be modified after creation.
/// Per-agent state belongs into the instance data, see GetInstanceDataDesc().
class EZ_AIPLUGIN_DLL ezAiBehavior : public ezReflectedClass
{
  EZ_ADD_DYNAMIC_REFLECTION(ezAiBehavior, ezReflectedClass);

public:
  ezAiBehavior() = default;
  ~ezAiBehavior() = default;

  /// Priority band of the behavior. Reflected per behavior type, see EZ_AI_BEHAVIOR_SCORE_PROPERTIES.
  ezEnum<ezAiScoreCategory> m_Category; // [ property ]

  /// Score within the category (0 to 1), used when the behavior wants to run. See MakeScore().
  float m_fScore = 0.5f; // [ property ]

  /// Considerations that scale m_fScore, see MakeScore().
  ezAiInputPin m_Considerations; // [ property ]

  // accessors for EZ_AI_BEHAVIOR_SCORE_PROPERTIES, member properties can't be declared on derived types
  ezEnum<ezAiScoreCategory> GetCategory() const { return m_Category; }
  void SetCategory(ezEnum<ezAiScoreCategory> category) { m_Category = category; }
  float GetScore() const { return m_fScore; }
  void SetScore(float fScore) { m_fScore = fScore; }

  /// Returned by ContinueBehavior().
  struct ContinuationState
  {
    /// If false, the behavior is in a state where it really wants to finish something, before another behavior may take over.
    /// No decision updates are done while this is false.
    bool m_bAllowBehaviorSwitch = true;
    /// In case the behavior wants to quit. The next decision update is then done right away.
    bool m_bEndBehavior = false;
  };

  /// Cheap early-out, before any perceptions are computed.
  ///
  /// fActiveBehaviorScore is the category of the active behavior (integer part of its score), or 0 if there is none.
  /// A behavior that can't score higher than that, may return false to skip the perception and score computation.
  /// Behaviors on cooldown are not asked.
  virtual bool IsAvailable(const void* pInstanceData, float fActiveBehaviorScore) const { return true; }

  /// Calls ezAiPerceptionManager::FlagAsNeeded() for every input pin that DetermineBehaviorScore() reads.
  ///
  /// Only flagged perceptions (and the sensors they need) get updated in this decision update.
  /// The perceptions that the considerations need are flagged separately.
  virtual void FlagNeededPerceptions(ezAiPerceptionManager& ref_PerceptionManager) const = 0;

  /// Returns how much the NPC wants to do this behavior right now. A score of 0 means not at all.
  ///
  /// Also called for the active behavior, which allows it to lose against others when its score drops.
  virtual ezAiBehaviorScore DetermineBehaviorScore(const ezAiContext& ctx, void* pInstanceData) const = 0;

  /// The behavior was chosen. Typically cancels the current actions and queues new ones.
  ///
  /// pPerception is the perception from the winning DetermineBehaviorScore() result and may be null.
  /// It may become invalid with the next perception update, so copy the needed data.
  virtual void OnActivateBehavior(const ezAiContext& ctx, void* pInstanceData, const ezAiPerception* pPerception) const = 0;

  /// The active behavior won the decision update again. Allows updating the queued actions, e.g. when the target moved.
  ///
  /// Called in every decision update in which the behavior wins, not only when something changed.
  /// Implementations typically call OnActivateBehavior() again only if the perception differs enough from the one they work on.
  /// pPerception is the same as for OnActivateBehavior() and may be null.
  virtual void OnReactivateBehavior(const ezAiContext& ctx, void* pInstanceData, const ezAiPerception* pPerception) const {}

  /// Another behavior takes over, or this behavior ended. Called before the next behavior's OnActivateBehavior().
  ///
  /// The queued actions are not canceled automatically, the next behavior usually does that.
  virtual void OnDeactivateBehavior(const ezAiContext& ctx, void* pInstanceData) const {}

  /// Called every frame on the active behavior, before the action queue gets executed.
  ///
  /// By default the behavior ends once its action queue is empty.
  virtual ContinuationState ContinueBehavior(const ezAiContext& ctx, void* pInstanceData) const
  {
    ContinuationState res;
    res.m_bEndBehavior = ctx.m_ActionQueue.IsEmpty();
    return res;
  }

  /// Time after the behavior was deactivated (it ended or another behavior took over), during which it isn't considered again.
  ///
  /// Queried right after OnDeactivateBehavior().
  virtual ezTime GetCooldownDuration(const void* pInstanceData) const { return ezTime::MakeZero(); }

  /// Returns false, if the behavior doesn't need instance data. Then pInstanceData is null in all calls.
  virtual bool GetInstanceDataDesc(ezInstanceDataDesc& out_desc) const { return false; }

  /// Used for debug output.
  virtual ezStringView GetName() const { return "<unnamed>"; }

protected:
  /// The score for one candidate perception: m_Category plus m_fScore multiplied with the results of all considerations.
  ///
  /// pCandidate may be null, e.g. for behaviors that don't use perceptions. Then the considerations only rate the agent's own state.
  /// The result is 0 ('don't run') if any consideration returns 0, or if m_fScore is 0.
  ///
  /// Many considerations would drag the product down, so each result is raised depending on their count
  /// (the 'compensation factor' by Dave Mark): v + (1 - v) * (1 - 1 / count) * v.
  ///
  /// Stops evaluating once the value can't get above fValueToBeat anymore (the part of a score below the category, 0 to 1)
  /// and returns 0 then. The results of the remaining considerations are -1 in that case.
  ezAiBehaviorScore MakeScore(const ezAiContext& ctx, const ezAiPerception* pCandidate = nullptr, float fValueToBeat = -1.0f) const;

  /// Scores every perception from the pin for which filter() returns true and returns the best one. Typical use:
  ///
  /// \code{.cpp}
  ///   return ScoreCandidates<ezAiPerceptionTarget>(ctx, m_Targets, [](const ezAiPerceptionTarget& t) { return t.m_bVisible; });
  /// \endcode
  ///
  /// The filter holds the hard conditions of the behavior, the considerations do the rating (see MakeScore()).
  /// On equal scores, the earlier perception wins. If no candidate scores above 0, the result has a score of 0 and
  /// the consideration results of the first candidate, so that the debug output shows why.
  template <typename PERCEPTION, typename FILTER>
  ezAiBehaviorScore ScoreCandidates(const ezAiContext& ctx, const ezAiInputPin& pin, FILTER filter) const
  {
    ezHybridArray<const ezAiPerception*, 16> perceptions;
    ctx.m_PerceptionManager.GetPerceptions(pin, perceptions);

    ezAiBehaviorScore best;
    float fBestValue = 0.0f;
    bool bFirst = true;

    for (const ezAiPerception* pPerception : perceptions)
    {
      if (!filter(*static_cast<const PERCEPTION*>(pPerception)))
        continue;

      ezAiBehaviorScore score = MakeScore(ctx, pPerception, fBestValue);
      const float fValue = score.GetScore() - static_cast<float>(m_Category.GetValue());

      if ((score.GetScore() > 0.0f && fValue > fBestValue) || bFirst)
      {
        best = std::move(score);
        fBestValue = ezMath::Max(fValue, 0.0f);
      }

      bFirst = false;
    }

    return best;
  }
};

/// Reflects ezAiBehavior::m_Category and m_fScore with the defaults of one concrete behavior type.
///
/// Put this at the start of the property list of every concrete behavior. The defaults only take effect through reflection
/// (in the editor, or with ezReflectionUtils::SetAllMemberPropertiesToDefault()), the constructors don't set them.
/// The 'Considerations' pin is reflected by ezAiBehavior itself.
#define EZ_AI_BEHAVIOR_SCORE_PROPERTIES(DefaultCategory, fDefaultScore) EZ_ENUM_ACCESSOR_PROPERTY("Category", ezAiScoreCategory, GetCategory, SetCategory)->AddAttributes(new ezDefaultValueAttribute(DefaultCategory)), EZ_ACCESSOR_PROPERTY("Score", GetScore, SetScore)->AddAttributes(new ezDefaultValueAttribute(fDefaultScore), new ezClampValueAttribute(0.0f, 1.0f))
