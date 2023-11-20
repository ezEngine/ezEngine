#pragma once

#include <AiPlugin/UtilityAI/Framework/AiBehavior.h>
#include <Foundation/Containers/HybridArray.h>
#include <Foundation/Types/UniquePtr.h>

class ezGameObject;
class ezAiPerceptionManager;

class EZ_AIPLUGIN_DLL ezAiBehaviorManager
{
  EZ_DISALLOW_COPY_AND_ASSIGN(ezAiBehaviorManager);

public:
  ezAiBehaviorManager();
  ~ezAiBehaviorManager();

  void DetermineAvailableBehaviors(ezTime currentTime, float fActiveBehaviorScore);

  void AddBehavior(ezUniquePtr<ezAiBehavior>&& pBehavior);

  void FlagNeededPerceptions(ezAiPerceptionManager& ref_PerceptionManager);

  ezAiBehaviorCandidate DetermineBehaviorCandidate(ezGameObject& owner, const ezAiPerceptionManager& perceptionManager);

  void SetActiveBehavior(ezGameObject& owner, ezAiBehavior* pBehavior, const ezAiPerception* pPerception, ezAiActionQueue& inout_ActionQueue);
  void KeepActiveBehavior(ezGameObject& owner, const ezAiPerception* pPerception, ezAiActionQueue& inout_ActionQueue);

  ezAiBehavior::ContinuationState ContinueActiveBehavior(ezGameObject& owner, ezAiActionQueue& inout_ActionQueue);

  bool HasActiveBehavior() const { return m_pActiveBehavior != nullptr; }

  ezAiBehavior* GetActiveBehavior() const { return m_pActiveBehavior; }

private:
  struct BehaviorInfo
  {
    // bool m_bActive = true;
    ezTime m_CooldownUntil;
    ezUInt32 m_uiNeededInUpdate = 0;

    ezUniquePtr<ezAiBehavior> m_pBehavior;
  };

  ezAiBehavior* m_pActiveBehavior = nullptr;
  ezHybridArray<BehaviorInfo, 12> m_Behaviors;

  ezUInt32 m_uiUpdateCount = 1;
  ezTime m_CurrentTime;
};
