#pragma once

#include <AiPlugin/AiPluginDLL.h>
#include <AiPlugin/UtilityAI/Framework/AiBehavior.h>
#include <Foundation/Containers/Blob.h>
#include <Foundation/Containers/HybridArray.h>

class ezAiControllerDesc;
class ezAiPerceptionManager;
class ezAiConsiderationManager;

/// Picks the best behavior of one agent and tracks which one is active and which are on cooldown.
///
/// The behaviors themselves are owned by the ezAiControllerDesc, this only tracks the per-agent state.
/// The decision logic itself (when to decide, when to switch) is in ezAiComponent::Update().
/// The functions are meant to be called in the order they are declared.
class EZ_AIPLUGIN_DLL ezAiBehaviorManager
{
  EZ_DISALLOW_COPY_AND_ASSIGN(ezAiBehaviorManager);

public:
  ezAiBehaviorManager();
  ~ezAiBehaviorManager();

  /// instanceData must stay valid until Deinitialize() is called.
  void Initialize(const ezAiControllerDesc& desc, ezByteBlobPtr instanceData);

  /// Does not deactivate the active behavior, the instance data is expected to be destroyed afterwards.
  void Deinitialize();

  /// Starts a new decision update: marks all behaviors as 'needed' that are not on cooldown and return true from ezAiBehavior::IsAvailable().
  ///
  /// Only those are considered in FlagNeededPerceptions() and DetermineBehaviorCandidate().
  /// fActiveBehaviorScore is passed through to IsAvailable().
  void DetermineAvailableBehaviors(ezTime currentTime, float fActiveBehaviorScore);

  /// Lets every available behavior and its considerations flag the perceptions they need.
  void FlagNeededPerceptions(ezAiPerceptionManager& ref_PerceptionManager, const ezAiConsiderationManager& considerationManager);

  /// Scores all available behaviors and returns the one with the highest score (above 0).
  ///
  /// Doesn't change the active behavior. If no behavior scores above 0, the candidate's behavior is null.
  ezAiBehaviorCandidate DetermineBehaviorCandidate(const ezAiContext& ctx);

  /// Deactivates the current behavior and activates the given one (ezInvalidIndex for none).
  ///
  /// Starts the cooldown of the deactivated behavior, whether it ended itself or another one took over.
  /// Activating the active behavior again (with a new perception) doesn't start its cooldown.
  ///
  /// Does nothing if the behavior is already active and pPerception is null.
  void SetActiveBehavior(const ezAiContext& ctx, ezUInt32 uiBehaviorIndex, const ezAiPerception* pPerception);

  /// Calls ezAiBehavior::OnReactivateBehavior() on the active behavior.
  void KeepActiveBehavior(const ezAiContext& ctx, const ezAiPerception* pPerception);

  /// Calls ezAiBehavior::ContinueBehavior() on the active behavior. Returns default values, if there is none.
  ezAiBehavior::ContinuationState ContinueActiveBehavior(const ezAiContext& ctx);

  bool HasActiveBehavior() const { return m_uiActiveBehavior != ezInvalidIndex; }

  /// Returns nullptr if there is no active behavior.
  const ezAiBehavior* GetActiveBehavior() const;

  /// Appends one line per behavior with its score and consideration results from the last DetermineBehaviorCandidate() call.
  void GetDebugText(ezStringBuilder& inout_sText) const;

  /// Per-agent state of one behavior, for debug output.
  struct BehaviorState
  {
    ezTime m_CooldownUntil;
    ezUInt32 m_uiNeededInUpdate = 0; ///< Equals m_uiUpdateCount if the behavior is available in the current decision update.
    float m_fLastScore = 0.0f;
    ezSmallArray<float, 4> m_LastConsiderationValues;
    const ezRTTI* m_pLastCandidateType = nullptr; ///< Type of the best candidate perception in the last scoring, null if there was none.
    ezVec3 m_vLastCandidatePosition = ezVec3::MakeZero();
    bool m_bLastCandidateHasPosition = false;     ///< See ezAiPerception::GetDebugPosition().
  };

  ezUInt32 GetBehaviorCount() const { return m_States.GetCount(); }
  const BehaviorState& GetBehaviorState(ezUInt32 uiIndex) const { return m_States[uiIndex]; }
  const ezAiBehavior* GetBehavior(ezUInt32 uiIndex) const;
  ezUInt32 GetActiveBehaviorIndex() const { return m_uiActiveBehavior; }

  /// Whether the behavior was available (not on cooldown, IsAvailable() returned true) in the last decision update.
  bool WasAvailable(ezUInt32 uiIndex) const { return m_States[uiIndex].m_uiNeededInUpdate == m_uiUpdateCount; }

  /// Time of the last decision update.
  ezTime GetDecisionTime() const { return m_CurrentTime; }

private:
  void* GetInstanceData(ezUInt32 uiIndex) const;

  const ezAiControllerDesc* m_pDesc = nullptr;
  ezByteBlobPtr m_InstanceData;
  ezUInt32 m_uiActiveBehavior = ezInvalidIndex;
  ezHybridArray<BehaviorState, 12> m_States; ///< One per behavior in the controller desc.

  ezUInt32 m_uiUpdateCount = 1;
  ezTime m_CurrentTime;
};
