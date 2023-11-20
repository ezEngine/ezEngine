#pragma once

#include <AiPlugin/UtilityAI/Framework/AiConsideration.h>
#include <AiPlugin/UtilityAI/Framework/AiInputPin.h>
#include <Foundation/Containers/ArrayMap.h>
#include <Foundation/Types/RangeView.h>

/// The distance to an ezAiPerceptionTarget candidate (ezAiPerceptionTarget::m_fDistance).
///
/// Returns a raw value of 0 for candidates that aren't targets, which maps to InputMin.
class EZ_AIPLUGIN_DLL ezAiConsiderationTargetDistance : public ezAiConsideration
{
  EZ_ADD_DYNAMIC_REFLECTION(ezAiConsiderationTargetDistance, ezAiConsideration);

public:
  ezAiConsiderationTargetDistance();

  virtual float GetInputValue(const ezAiContext& ctx, void* pInstanceData, const ezAiPerception* pCandidate) const override;
};

/// A number from the agent's blackboard (see ezBlackboardComponent::FindBlackboard()), e.g. health or ammo that the game writes.
///
/// Doesn't depend on the candidate. Uses m_fFallback if there is no such entry or it can't be converted to a number.
class EZ_AIPLUGIN_DLL ezAiConsiderationBlackboard : public ezAiConsideration
{
  EZ_ADD_DYNAMIC_REFLECTION(ezAiConsiderationBlackboard, ezAiConsideration);

public:
  ezAiConsiderationBlackboard();

  ezHashedString m_sEntryName; // [ property ]
  float m_fFallback = 0.0f;    // [ property ]

  virtual bool UsesCandidate() const override { return false; }
  virtual float GetInputValue(const ezAiContext& ctx, void* pInstanceData, const ezAiPerception* pCandidate) const override;
};

/// Computes the raw value in a visual script: the script class has 'AiConsiderationScript' as its base class and implements 'Evaluate'.
///
/// The script calls SetValue() on the ezAiScriptContext, the result then goes through the input range and curve as usual.
/// It gets the candidate (may be null, see GetInputValue()) and the perceptions of the 'Perceptions' pin through the context.
/// Each agent has its own script instance, so script variables keep their values between calls.
/// 'Evaluate' must not wait (no coroutine nodes such as delays), it has to compute the value right away.
class EZ_AIPLUGIN_DLL ezAiConsiderationScript : public ezAiConsideration
{
  EZ_ADD_DYNAMIC_REFLECTION(ezAiConsiderationScript, ezAiConsideration);

public:
  ezAiConsiderationScript();
  ~ezAiConsiderationScript();

  ezString m_sScriptClass; // [ property ]

  /// If false, the script is called only once per decision update and gets no candidate.
  bool m_bUsesCandidate = true; // [ property ]

  /// Optional, the script reads these perceptions through ezAiScriptContext::GetPerception().
  ezAiInputPin m_Perceptions; // [ property ]

  virtual void FlagNeededPerceptions(ezAiPerceptionManager& ref_PerceptionManager) const override;
  virtual bool UsesCandidate() const override { return m_bUsesCandidate; }
  virtual float GetInputValue(const ezAiContext& ctx, void* pInstanceData, const ezAiPerception* pCandidate) const override;
  virtual bool GetInstanceDataDesc(ezInstanceDataDesc& out_desc) const override;

  // Exposed Parameters
  const ezRangeView<const char*, ezUInt32> GetParameters() const;
  void SetParameter(const char* szKey, const ezVariant& value);
  void RemoveParameter(const char* szKey);
  bool GetParameter(const char* szKey, ezVariant& out_value) const;

private:
  // dummy function for the script base class reflection
  void Reflection_Evaluate(ezAiScriptContext* pContext, const ezAiPerception* pCandidate);

  ezArrayMap<ezHashedString, ezVariant> m_Parameters;
};
