#pragma once

#include <AiPlugin/UtilityAI/Framework/AiAction.h>
#include <AiPlugin/UtilityAI/Framework/AiBehavior.h>
#include <AiPlugin/UtilityAI/Framework/AiInputPin.h>
#include <Core/World/GameObject.h>

// Behaviors that re-implement the Testing Chambers "Al" bot (Data/Samples/Testing Chambers/AI/Al-Bot/Al-Bot.as)
// on top of the utility AI. They are specific to that bot (blackboard entry names, distances) and only meant for testing the framework.
//
// Blackboard entries used by the Al animation graph:
//   "Shoot": -1 = weapon holstered, 0 = weapon drawn, 1 = shooting
//   "Melee": >= 0 starts a melee attack, the graph sets it to -2 when done
//   "PlayIdleAction": >= 0 plays a random idle animation, the graph resets it to -1
//
// Base scores (category + value), highest wins:
//   Melee 5.9 > Shoot 5.7 > Engage 5.5 > CheckHint 3.8 > Investigate 3.6 > Search 3.3 > Patrol 2.5 > Idle 1.5
// The combat behaviors and Investigate/Search read the ezAiPerceptionTargets from their 'Targets' pin, CheckHint the ezAiPerceptionHints from 'Hints'. The code only checks
// hard conditions (visible or not, already investigated), the considerations in the AI controller asset rate the candidates
// and decide e.g. at which distances Melee, Shoot and Engage take over. Without considerations, Melee always wins.

class ezAiPerceptionTarget;

/// Turns towards a target and sets the "Shoot" blackboard entry while it is in front.
///
/// Never finishes on its own, only when canceled or when the target disappears.
class EZ_AIPLUGIN_DLL ezAiActionAlShootAt : public ezAiAction
{
  EZ_DECLARE_AICMD(ezAiActionAlShootAt);

public:
  ezAiActionAlShootAt();
  ~ezAiActionAlShootAt();

  virtual void Reset() override;
  virtual void GetDebugDesc(ezStringBuilder& inout_sText) override;
  virtual ezAiActionResult Execute(ezGameObject& owner, ezTime tDiff, ezLogInterface* pLog) override;
  virtual void Cancel(ezGameObject& owner) override;

  ezGameObjectHandle m_hTarget;
  ezAngle m_MaxAngle = ezAngle::MakeFromDegree(20);

private:
  bool m_bCanceled = false;
  bool m_bShooting = false;
  ezAngle m_LastAngle;
  ezUInt8 m_uiNavState = 0;
};

//////////////////////////////////////////////////////////////////////////

/// Holsters the weapon, sometimes plays an idle animation, then waits for m_Duration. Always available, lowest score.
class EZ_AIPLUGIN_DLL ezAiBehaviorAlIdle : public ezAiBehavior
{
  EZ_ADD_DYNAMIC_REFLECTION(ezAiBehaviorAlIdle, ezAiBehavior);

public:
  ezAiBehaviorAlIdle();

  ezTime m_Duration = ezTime::Seconds(5); // [ property ]
  /// Probability (0 to 1) to play an idle animation.
  float m_fIdleAnimationChance = 0.4f; // [ property ]

  virtual void FlagNeededPerceptions(ezAiPerceptionManager& ref_PerceptionManager) const override {}
  virtual ezAiBehaviorScore DetermineBehaviorScore(const ezAiContext& ctx, void* pInstanceData) const override;
  virtual void OnActivateBehavior(const ezAiContext& ctx, void* pInstanceData, const ezAiPerception* pPerception) const override;
  virtual ezStringView GetName() const override { return "Idle"; }
};

/// Walks slowly to a random point within m_fRadius. Has a random cooldown after the walk, Idle runs in between.
class EZ_AIPLUGIN_DLL ezAiBehaviorAlPatrol : public ezAiBehavior
{
  EZ_ADD_DYNAMIC_REFLECTION(ezAiBehaviorAlPatrol, ezAiBehavior);

public:
  ezAiBehaviorAlPatrol();

  /// Maximum distance of the patrol point.
  float m_fRadius = 20.0f; // [ property ]
  float m_fSpeed = 1.0f;   // [ property ]
  /// Random cooldown in this range, starts when the behavior ends.
  ezTime m_MinCooldown = ezTime::Seconds(10); // [ property ]
  ezTime m_MaxCooldown = ezTime::Seconds(25); // [ property ]

  virtual void FlagNeededPerceptions(ezAiPerceptionManager& ref_PerceptionManager) const override {}
  virtual ezAiBehaviorScore DetermineBehaviorScore(const ezAiContext& ctx, void* pInstanceData) const override;
  virtual void OnActivateBehavior(const ezAiContext& ctx, void* pInstanceData, const ezAiPerception* pPerception) const override;
  virtual ezTime GetCooldownDuration(const void* pInstanceData) const override;
  virtual bool GetInstanceDataDesc(ezInstanceDataDesc& out_desc) const override;
  virtual ezStringView GetName() const override { return "Patrol"; }

private:
  struct InstanceData
  {
    ezTime m_Cooldown;
  };
};

/// Target not visible anymore, but remembered: walks to the last known position.
///
/// Restarts when it picks another target or the target was seen again in between. Once the position is reached, all information
/// that is older than that counts as done: Investigate only scores again for targets that are seen (and lost) afterwards.
class EZ_AIPLUGIN_DLL ezAiBehaviorAlInvestigate : public ezAiBehavior
{
  EZ_ADD_DYNAMIC_REFLECTION(ezAiBehaviorAlInvestigate, ezAiBehavior);

public:
  ezAiBehaviorAlInvestigate();

  float m_fSpeed = 1.5f; // [ property ]

  /// Generators of ezAiPerceptionTarget.
  ezAiInputPin m_Targets; // [ property ]

  virtual void FlagNeededPerceptions(ezAiPerceptionManager& ref_PerceptionManager) const override;
  virtual ezAiBehaviorScore DetermineBehaviorScore(const ezAiContext& ctx, void* pInstanceData) const override;
  virtual void OnActivateBehavior(const ezAiContext& ctx, void* pInstanceData, const ezAiPerception* pPerception) const override;
  virtual void OnReactivateBehavior(const ezAiContext& ctx, void* pInstanceData, const ezAiPerception* pPerception) const override;
  virtual ContinuationState ContinueBehavior(const ezAiContext& ctx, void* pInstanceData) const override;
  virtual bool GetInstanceDataDesc(ezInstanceDataDesc& out_desc) const override;
  virtual ezStringView GetName() const override { return "Investigate"; }

private:
  struct InstanceData
  {
    ezGameObjectHandle m_hActiveTarget;
    ezTime m_ActiveLastSeen;                       ///< ezAiPerceptionTarget::m_LastSeen of the information that is being investigated.
    ezTime m_DoneTime = ezTime::MakeFromHours(-1); ///< When the last investigation was finished. Older information is ignored.
  };
};

/// Reached the last known position without finding the target: turns towards it, then walks to m_uiNumPoints random points nearby.
///
/// Has the same condition as Investigate but a lower score, so it runs once Investigate is done with the same information.
class EZ_AIPLUGIN_DLL ezAiBehaviorAlSearch : public ezAiBehavior
{
  EZ_ADD_DYNAMIC_REFLECTION(ezAiBehaviorAlSearch, ezAiBehavior);

public:
  ezAiBehaviorAlSearch();

  /// How many random points to walk to.
  ezUInt32 m_uiNumPoints = 3; // [ property ]
  /// Search radius for the first point.
  float m_fRadius = 10.0f; // [ property ]
  /// Added to the radius for every further point.
  float m_fRadiusIncrease = 3.0f; // [ property ]
  float m_fSpeed = 1.5f;          // [ property ]
  /// Maximum time to turn towards the last known position first.
  ezTime m_TurnTimeout = ezTime::Seconds(1.5); // [ property ]

  /// Generators of ezAiPerceptionTarget.
  ezAiInputPin m_Targets; // [ property ]

  virtual void FlagNeededPerceptions(ezAiPerceptionManager& ref_PerceptionManager) const override;
  virtual ezAiBehaviorScore DetermineBehaviorScore(const ezAiContext& ctx, void* pInstanceData) const override;
  virtual void OnActivateBehavior(const ezAiContext& ctx, void* pInstanceData, const ezAiPerception* pPerception) const override;
  virtual ContinuationState ContinueBehavior(const ezAiContext& ctx, void* pInstanceData) const override;
  virtual bool GetInstanceDataDesc(ezInstanceDataDesc& out_desc) const override;
  virtual ezStringView GetName() const override { return "Search"; }

private:
  struct InstanceData
  {
    ezTime m_DoneTime = ezTime::MakeFromHours(-1); ///< When the last search was finished. Older information is ignored.
    ezUInt32 m_uiAttempt = 0;
  };
};

/// A hint was sensed (e.g. the noise of a shot): turns towards its source, then walks there.
///
/// The hint perceptions only exist while the markers do, so the behavior stores the position and keeps its score until it arrives.
/// Restarts when a hint comes from a position more than m_fRepathDistance away from the stored one.
class EZ_AIPLUGIN_DLL ezAiBehaviorAlCheckHint : public ezAiBehavior
{
  EZ_ADD_DYNAMIC_REFLECTION(ezAiBehaviorAlCheckHint, ezAiBehavior);

public:
  ezAiBehaviorAlCheckHint();

  float m_fSpeed = 2.0f;                       // [ property ]
  float m_fRepathDistance = 3.0f;              // [ property ]
  ezTime m_TurnTimeout = ezTime::Seconds(1.0); // [ property ]

  /// Generators of ezAiPerceptionHint.
  ezAiInputPin m_Hints; // [ property ]

  virtual void FlagNeededPerceptions(ezAiPerceptionManager& ref_PerceptionManager) const override;
  virtual ezAiBehaviorScore DetermineBehaviorScore(const ezAiContext& ctx, void* pInstanceData) const override;
  virtual void OnActivateBehavior(const ezAiContext& ctx, void* pInstanceData, const ezAiPerception* pPerception) const override;
  virtual void OnReactivateBehavior(const ezAiContext& ctx, void* pInstanceData, const ezAiPerception* pPerception) const override;
  virtual void OnDeactivateBehavior(const ezAiContext& ctx, void* pInstanceData) const override;
  virtual ContinuationState ContinueBehavior(const ezAiContext& ctx, void* pInstanceData) const override;
  virtual bool GetInstanceDataDesc(ezInstanceDataDesc& out_desc) const override;
  virtual ezStringView GetName() const override { return "CheckHint"; }

private:
  struct InstanceData
  {
    bool m_bActive = false;
    ezVec3 m_vPosition = ezVec3::MakeZero();
  };
};

/// Target visible: runs to a random point near it, within shooting range. Meant to lose against Shoot once in range.
///
/// Picks a new point when the target moved more than m_fRepathDistance.
/// Keeps going for m_LostSightGracePeriod after the target was last seen, so that a briefly missed sensor check doesn't switch to Investigate.
class EZ_AIPLUGIN_DLL ezAiBehaviorAlEngage : public ezAiBehavior
{
  EZ_ADD_DYNAMIC_REFLECTION(ezAiBehaviorAlEngage, ezAiBehavior);

public:
  ezAiBehaviorAlEngage();

  /// The destination is at most 90% of this away from the target.
  float m_fShootDistance = 15.0f; // [ property ]
  float m_fSpeed = 3.0f;          // [ property ]
  /// Picks a new destination, when the target moved further than this.
  float m_fRepathDistance = 2.0f; // [ property ]
  /// How long a target that isn't detected anymore is still engaged.
  ezTime m_LostSightGracePeriod = ezTime::MakeFromSeconds(1.0); // [ property ]

  /// Generators of ezAiPerceptionTarget.
  ezAiInputPin m_Targets; // [ property ]

  virtual void FlagNeededPerceptions(ezAiPerceptionManager& ref_PerceptionManager) const override;
  virtual ezAiBehaviorScore DetermineBehaviorScore(const ezAiContext& ctx, void* pInstanceData) const override;
  virtual void OnActivateBehavior(const ezAiContext& ctx, void* pInstanceData, const ezAiPerception* pPerception) const override;
  virtual void OnReactivateBehavior(const ezAiContext& ctx, void* pInstanceData, const ezAiPerception* pPerception) const override;
  virtual bool GetInstanceDataDesc(ezInstanceDataDesc& out_desc) const override;
  virtual ezStringView GetName() const override { return "Engage"; }

private:
  struct InstanceData
  {
    ezVec3 m_vReferencePosition = ezVec3::MakeZero(); ///< Target position when the current destination was picked.
  };
};

/// Target visible: stops, turns towards it and shoots (ezAiActionAlShootAt). Runs until it loses against another behavior.
///
/// The shooting range comes from a distance consideration. Switches to another target once that one scores best.
/// While active, keeps shooting at its target for m_LostSightGracePeriod after it was last seen, so that losing sight for a moment (e.g. at a corner) doesn't switch to another behavior right away.
class EZ_AIPLUGIN_DLL ezAiBehaviorAlShoot : public ezAiBehavior
{
  EZ_ADD_DYNAMIC_REFLECTION(ezAiBehaviorAlShoot, ezAiBehavior);

public:
  ezAiBehaviorAlShoot();

  /// Generators of ezAiPerceptionTarget.
  ezAiInputPin m_Targets; // [ property ]

  /// How long the active target is still shot at, after it isn't detected anymore.
  ezTime m_LostSightGracePeriod = ezTime::MakeFromSeconds(0.5); // [ property ]

  virtual void FlagNeededPerceptions(ezAiPerceptionManager& ref_PerceptionManager) const override;
  virtual ezAiBehaviorScore DetermineBehaviorScore(const ezAiContext& ctx, void* pInstanceData) const override;
  virtual void OnActivateBehavior(const ezAiContext& ctx, void* pInstanceData, const ezAiPerception* pPerception) const override;
  virtual void OnReactivateBehavior(const ezAiContext& ctx, void* pInstanceData, const ezAiPerception* pPerception) const override;
  virtual void OnDeactivateBehavior(const ezAiContext& ctx, void* pInstanceData) const override;
  virtual bool GetInstanceDataDesc(ezInstanceDataDesc& out_desc) const override;
  virtual ezStringView GetName() const override { return "Shoot"; }

private:
  struct InstanceData
  {
    ezGameObjectHandle m_hActiveTarget;
    bool m_bActive = false;
  };
};

/// Target visible: stops, turns towards it and plays the melee attack (via the 'Melee' blackboard entry).
///
/// The melee range comes from a distance consideration. Can't be interrupted: ContinueBehavior() disallows switching until all its actions (including the attack animation) are done.
class EZ_AIPLUGIN_DLL ezAiBehaviorAlMelee : public ezAiBehavior
{
  EZ_ADD_DYNAMIC_REFLECTION(ezAiBehaviorAlMelee, ezAiBehavior);

public:
  ezAiBehaviorAlMelee();

  /// Generators of ezAiPerceptionTarget.
  ezAiInputPin m_Targets; // [ property ]

  virtual void FlagNeededPerceptions(ezAiPerceptionManager& ref_PerceptionManager) const override;
  virtual ezAiBehaviorScore DetermineBehaviorScore(const ezAiContext& ctx, void* pInstanceData) const override;
  virtual void OnActivateBehavior(const ezAiContext& ctx, void* pInstanceData, const ezAiPerception* pPerception) const override;
  virtual ContinuationState ContinueBehavior(const ezAiContext& ctx, void* pInstanceData) const override;
  virtual ezStringView GetName() const override { return "Melee"; }
};
