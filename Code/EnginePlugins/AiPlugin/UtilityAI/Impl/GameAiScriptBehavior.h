#pragma once

#include <AiPlugin/UtilityAI/Framework/AiBehavior.h>
#include <AiPlugin/UtilityAI/Framework/AiInputPin.h>
#include <AiPlugin/UtilityAI/Framework/AiScript.h>

/// A behavior that is implemented as a visual script: the script class has 'AiBehaviorScript' as its base class.
///
/// The score is computed as for C++ behaviors (category, score and considerations), once per perception of the 'Candidates' pin,
/// or once without a candidate if nothing is connected. The script implements any of these functions, which match the ezAiBehavior functions:
///  - 'CanRunBehavior(Context, Candidate)': hard conditions, calls SetCanRun(false) on the context to skip the candidate. Called before the considerations.
///  - 'OnActivateBehavior(Context, Candidate)': the behavior was chosen. The queued actions of the previous behavior are canceled already.
///    Queues the behavior's actions (AiActions::Queue... nodes). The behavior stays active until they are done, without actions it ends right away.
///  - 'OnReactivateBehavior(Context, Candidate)': called in every decision update in which the active behavior wins again.
///    May queue more actions, or call Restart() on the context to cancel the actions and run 'OnActivateBehavior' again with the new candidate.
///  - 'OnDeactivateBehavior(Context)': the behavior ended or another behavior takes over.
///
/// The script functions must not wait (no coroutine nodes such as delays), they are stopped at the first wait, with a warning.
/// The context and the candidate are only valid during the call.
class EZ_AIPLUGIN_DLL ezAiBehaviorScript : public ezAiBehavior
{
  EZ_ADD_DYNAMIC_REFLECTION(ezAiBehaviorScript, ezAiBehavior);

public:
  ezAiBehaviorScript();
  ~ezAiBehaviorScript();

  /// Used in the debug output.
  ezString m_sName;        // [ property ]

  ezString m_sScriptClass; // [ property ]

  /// The perceptions to choose from, any type.
  ezAiInputPin m_Candidates; // [ property ]

  /// While the behavior is active, keep its candidate when it isn't perceived anymore (e.g. a hint that disappeared).
  ///
  /// The considerations then rate a copy of the candidate from the activation, and 'OnReactivateBehavior' gets that copy.
  /// If false, the behavior loses against any other behavior once no candidate is left.
  bool m_bKeepCandidate = true; // [ property ]

  /// Time after the behavior ended, before it may run again.
  ezTime m_Cooldown; // [ property ]

  virtual void FlagNeededPerceptions(ezAiPerceptionManager& ref_PerceptionManager) const override;
  virtual ezAiBehaviorScore DetermineBehaviorScore(const ezAiContext& ctx, void* pInstanceData) const override;
  virtual void OnActivateBehavior(const ezAiContext& ctx, void* pInstanceData, const ezAiPerception* pPerception) const override;
  virtual void OnReactivateBehavior(const ezAiContext& ctx, void* pInstanceData, const ezAiPerception* pPerception) const override;
  virtual void OnDeactivateBehavior(const ezAiContext& ctx, void* pInstanceData) const override;
  virtual ezTime GetCooldownDuration(const void* pInstanceData) const override { return m_Cooldown; }
  virtual bool GetInstanceDataDesc(ezInstanceDataDesc& out_desc) const override;
  virtual ezStringView GetName() const override { return m_sName; }

  // Exposed Parameters
  const ezRangeView<const char*, ezUInt32> GetParameters() const;
  void SetParameter(const char* szKey, const ezVariant& value);
  void RemoveParameter(const char* szKey);
  bool GetParameter(const char* szKey, ezVariant& out_value) const;

private:
  struct InstanceData
  {
    ezAiScriptInstance m_Script;
    ezUniquePtr<ezAiPerception> m_pCandidate; ///< Copy of the candidate of the last activation, for m_bKeepCandidate.
    bool m_bActive = false;
  };

  const ezAbstractFunctionProperty* GetFunction(const ezAiContext& ctx, InstanceData& ref_data, ezUInt32 uiFunction) const;
  bool CanRun(const ezAiContext& ctx, InstanceData& ref_data, const ezAiPerception* pCandidate) const;

  // dummy functions for the script base class reflection
  void Reflection_CanRunBehavior(ezAiScriptContext* pContext, const ezAiPerception* pCandidate) {}
  void Reflection_OnActivateBehavior(ezAiScriptContext* pContext, const ezAiPerception* pCandidate) {}
  void Reflection_OnReactivateBehavior(ezAiScriptContext* pContext, const ezAiPerception* pCandidate) {}
  void Reflection_OnDeactivateBehavior(ezAiScriptContext* pContext) {}

  ezArrayMap<ezHashedString, ezVariant> m_Parameters;
};
