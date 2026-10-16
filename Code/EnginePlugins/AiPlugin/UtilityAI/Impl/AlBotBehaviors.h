#pragma once

#include <AiPlugin/UtilityAI/Framework/AiAction.h>
#include <AiPlugin/UtilityAI/Framework/AiBehavior.h>
#include <Core/World/GameObject.h>

// Behaviors that re-implement the Testing Chambers "Al" bot (Data/Samples/Testing Chambers/AI/Al-Bot/Al-Bot.as)
// on top of the utility AI. They are specific to that bot (blackboard entry names, distances) and only meant for testing the framework.
//
// Blackboard entries used by the Al animation graph:
//   "Shoot": -1 = weapon holstered, 0 = weapon drawn, 1 = shooting
//   "Melee": >= 0 starts a melee attack, the graph sets it to -2 when done
//   "PlayIdleAction": >= 0 plays a random idle animation, the graph resets it to -1

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

/// Stand around, sometimes play an idle animation.
class EZ_AIPLUGIN_DLL ezAiBehaviorAlIdle : public ezAiBehavior
{
public:
  virtual void FlagNeededPerceptions(ezAiPerceptionManager& ref_PerceptionManager) override {}
  virtual ezAiBehaviorScore DetermineBehaviorScore(ezGameObject& owner, const ezAiPerceptionManager& perceptionManager) override;
  virtual void ActivateBehavior(ezGameObject& owner, const ezAiPerception* pPerception, ezAiActionQueue& inout_ActionQueue) override;
  virtual void ReactivateBehavior(ezGameObject& owner, const ezAiPerception* pPerception, ezAiActionQueue& inout_ActionQueue) override {}
  virtual ezStringView GetName() const override { return "Idle"; }
};

/// Walk to a random point nearby.
class EZ_AIPLUGIN_DLL ezAiBehaviorAlPatrol : public ezAiBehavior
{
public:
  virtual void FlagNeededPerceptions(ezAiPerceptionManager& ref_PerceptionManager) override {}
  virtual ezAiBehaviorScore DetermineBehaviorScore(ezGameObject& owner, const ezAiPerceptionManager& perceptionManager) override;
  virtual void ActivateBehavior(ezGameObject& owner, const ezAiPerception* pPerception, ezAiActionQueue& inout_ActionQueue) override;
  virtual void ReactivateBehavior(ezGameObject& owner, const ezAiPerception* pPerception, ezAiActionQueue& inout_ActionQueue) override {}
  virtual ezTime GetCooldownDuration() override { return m_Cooldown; }
  virtual ezStringView GetName() const override { return "Patrol"; }

private:
  ezTime m_Cooldown;
};

/// Target not visible anymore: go to the last known position.
class EZ_AIPLUGIN_DLL ezAiBehaviorAlInvestigate : public ezAiBehavior
{
public:
  virtual void FlagNeededPerceptions(ezAiPerceptionManager& ref_PerceptionManager) override;
  virtual ezAiBehaviorScore DetermineBehaviorScore(ezGameObject& owner, const ezAiPerceptionManager& perceptionManager) override;
  virtual void ActivateBehavior(ezGameObject& owner, const ezAiPerception* pPerception, ezAiActionQueue& inout_ActionQueue) override;
  virtual void ReactivateBehavior(ezGameObject& owner, const ezAiPerception* pPerception, ezAiActionQueue& inout_ActionQueue) override;
  virtual ContinuationState ContinueBehavior(ezGameObject& owner, ezAiActionQueue& inout_ActionQueue) override;
  virtual ezStringView GetName() const override { return "Investigate"; }

private:
  ezTime m_ActiveLastSeen;
  ezTime m_DoneLastSeen = ezTime::MakeFromHours(-1);
};

/// Reached the last known position without finding the target: look around a few random points.
class EZ_AIPLUGIN_DLL ezAiBehaviorAlSearch : public ezAiBehavior
{
public:
  virtual void FlagNeededPerceptions(ezAiPerceptionManager& ref_PerceptionManager) override;
  virtual ezAiBehaviorScore DetermineBehaviorScore(ezGameObject& owner, const ezAiPerceptionManager& perceptionManager) override;
  virtual void ActivateBehavior(ezGameObject& owner, const ezAiPerception* pPerception, ezAiActionQueue& inout_ActionQueue) override;
  virtual void ReactivateBehavior(ezGameObject& owner, const ezAiPerception* pPerception, ezAiActionQueue& inout_ActionQueue) override {}
  virtual ContinuationState ContinueBehavior(ezGameObject& owner, ezAiActionQueue& inout_ActionQueue) override;
  virtual ezStringView GetName() const override { return "Search"; }

private:
  ezTime m_ActiveLastSeen;
  ezTime m_DoneLastSeen = ezTime::MakeFromHours(-1);
  ezUInt32 m_uiAttempt = 0;
};

/// Target visible but too far away: run to a random point near it.
class EZ_AIPLUGIN_DLL ezAiBehaviorAlEngage : public ezAiBehavior
{
public:
  virtual void FlagNeededPerceptions(ezAiPerceptionManager& ref_PerceptionManager) override;
  virtual ezAiBehaviorScore DetermineBehaviorScore(ezGameObject& owner, const ezAiPerceptionManager& perceptionManager) override;
  virtual void ActivateBehavior(ezGameObject& owner, const ezAiPerception* pPerception, ezAiActionQueue& inout_ActionQueue) override;
  virtual void ReactivateBehavior(ezGameObject& owner, const ezAiPerception* pPerception, ezAiActionQueue& inout_ActionQueue) override;
  virtual ezStringView GetName() const override { return "Engage"; }

private:
  ezVec3 m_vReferencePosition = ezVec3::MakeZero();
};

/// Target visible and in range: turn towards it and shoot.
class EZ_AIPLUGIN_DLL ezAiBehaviorAlShoot : public ezAiBehavior
{
public:
  virtual void FlagNeededPerceptions(ezAiPerceptionManager& ref_PerceptionManager) override;
  virtual ezAiBehaviorScore DetermineBehaviorScore(ezGameObject& owner, const ezAiPerceptionManager& perceptionManager) override;
  virtual void ActivateBehavior(ezGameObject& owner, const ezAiPerception* pPerception, ezAiActionQueue& inout_ActionQueue) override;
  virtual void ReactivateBehavior(ezGameObject& owner, const ezAiPerception* pPerception, ezAiActionQueue& inout_ActionQueue) override {}
  virtual void DeactivateBehavior(ezGameObject& owner, ezAiActionQueue& inout_ActionQueue) override { m_bActive = false; }
  virtual ezStringView GetName() const override { return "Shoot"; }

private:
  bool m_bActive = false;
};

/// Target very close: melee attack. Can't be interrupted while the attack animation plays.
class EZ_AIPLUGIN_DLL ezAiBehaviorAlMelee : public ezAiBehavior
{
public:
  virtual void FlagNeededPerceptions(ezAiPerceptionManager& ref_PerceptionManager) override;
  virtual ezAiBehaviorScore DetermineBehaviorScore(ezGameObject& owner, const ezAiPerceptionManager& perceptionManager) override;
  virtual void ActivateBehavior(ezGameObject& owner, const ezAiPerception* pPerception, ezAiActionQueue& inout_ActionQueue) override;
  virtual void ReactivateBehavior(ezGameObject& owner, const ezAiPerception* pPerception, ezAiActionQueue& inout_ActionQueue) override {}
  virtual ContinuationState ContinueBehavior(ezGameObject& owner, ezAiActionQueue& inout_ActionQueue) override;
  virtual ezStringView GetName() const override { return "Melee"; }
};
