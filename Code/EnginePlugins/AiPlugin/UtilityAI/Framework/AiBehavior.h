#pragma once

#include <GameEngine/GameEngineDLL.h>

class ezAiBehavior;
class ezAiPerceptionManager;
class ezAiPerception;
class ezGameObject;
class ezAiActionQueue;

enum class ezAiScoreCategory
{
  Fallback,    ///< if really nothing else is available
  Idle,        ///< Idle actions (simple animation playback and such)
  ActiveIdle,  ///< The main actions to do when an NPC is idle, e.g. wander around, follow a patrol path
  Investigate, ///< in case something interesting is detected
  Command,     ///< things the player (or level designer) instructs the NPC to do that should have high priority
  Combat,      ///< combat related behavior
  Interrupt,   ///< things that even override combat scenarios (hit reactions, falling down, etc)
};

class ezAiBehaviorScore
{
public:
  ezAiBehaviorScore() {}

  void SetScore(ezAiScoreCategory category, float fValue)
  {
    EZ_ASSERT_DEBUG(fValue >= 0.0f && fValue <= 1.0f, "Value is out of 0-1 range.");
    m_fValue = static_cast<float>(category) + fValue;
  }

  float GetScore() const { return m_fValue; }

  const ezAiPerception* m_pPerception = nullptr;

private:
  float m_fValue = 0.0f;
};

struct ezAiBehaviorCandidate
{
  ezAiBehavior* m_pBehavior = nullptr;
  const ezAiPerception* m_pPerception = nullptr;
  float m_fScore = 0.0f;
};

class EZ_AIPLUGIN_DLL ezAiBehavior
{
public:
  ezAiBehavior() = default;
  virtual ~ezAiBehavior() = default;

  struct ContinuationState
  {
    /// \brief If false, the behavior is in a state where it really wants to finish something, before another behavior may take over.
    bool m_bAllowBehaviorSwitch = true;
    /// \brief In case the behavior wants to quit.
    bool m_bEndBehavior = false;
  };

  virtual bool IsAvailable(float fActiveBehaviorScore) const { return true; }
  virtual void FlagNeededPerceptions(ezAiPerceptionManager& ref_PerceptionManager) = 0;

  virtual ezAiBehaviorScore DetermineBehaviorScore(ezGameObject& owner, const ezAiPerceptionManager& perceptionManager) = 0;

  virtual void ActivateBehavior(ezGameObject& owner, const ezAiPerception* pPerception, ezAiActionQueue& inout_ActionQueue) = 0;
  virtual void ReactivateBehavior(ezGameObject& owner, const ezAiPerception* pPerception, ezAiActionQueue& inout_ActionQueue) = 0;
  virtual void DeactivateBehavior(ezGameObject& owner, ezAiActionQueue& inout_ActionQueue) {}
  virtual ContinuationState ContinueBehavior(ezGameObject& owner, ezAiActionQueue& inout_ActionQueue)
  {
    ContinuationState res;
    res.m_bEndBehavior = inout_ActionQueue.IsEmpty();
    return res;
  }

  virtual ezTime GetCooldownDuration() { return ezTime::MakeZero(); }
};
