#include <AiPlugin/AiPluginPCH.h>

#include <AiPlugin/UtilityAI/Framework/AiActionQueue.h>
#include <AiPlugin/UtilityAI/Framework/AiPerceptionManager.h>
#include <AiPlugin/UtilityAI/Impl/GameAiScriptBehavior.h>
#include <Core/Scripting/ScriptAttributes.h>
#include <Core/World/GameObject.h>
#include <Foundation/Memory/InstanceDataAllocator.h>

namespace
{
  enum ScriptFunction : ezUInt32
  {
    CanRunBehavior,
    OnActivateBehavior,
    OnReactivateBehavior,
    OnDeactivateBehavior,
  };
} // namespace

// clang-format off
EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezAiBehaviorScript, 1, ezRTTIDefaultAllocator<ezAiBehaviorScript>)
{
  EZ_BEGIN_PROPERTIES
  {
    EZ_AI_BEHAVIOR_SCORE_PROPERTIES(ezAiScoreCategory::Idle, 0.5f),
    EZ_MEMBER_PROPERTY("Name", m_sName)->AddAttributes(new ezDefaultValueAttribute("Script")),
    EZ_MEMBER_PROPERTY("ScriptClass", m_sScriptClass)->AddAttributes(new ezAssetBrowserAttribute("CompatibleAsset_ScriptClass"), new ezRequiredAttribute()),
    EZ_MAP_ACCESSOR_PROPERTY("Parameters", GetParameters, GetParameter, SetParameter, RemoveParameter)->AddAttributes(new ezExposedParametersAttribute("ScriptClass")),
    EZ_MEMBER_PROPERTY("Cooldown", m_Cooldown),
    EZ_MEMBER_PROPERTY("KeepCandidate", m_bKeepCandidate)->AddAttributes(new ezDefaultValueAttribute(true)),
    EZ_MEMBER_PROPERTY("Candidates", m_Candidates)->AddAttributes(new ezAiInputPinAttribute("ezAiPerception"), new ezHiddenAttribute()),
  }
  EZ_END_PROPERTIES;
  EZ_BEGIN_FUNCTIONS
  {
    EZ_SCRIPT_FUNCTION_PROPERTY(Reflection_CanRunBehavior, In, "Context", In, "Candidate")->AddAttributes(new ezScriptBaseClassFunctionAttribute(ScriptFunction::CanRunBehavior)),
    EZ_SCRIPT_FUNCTION_PROPERTY(Reflection_OnActivateBehavior, In, "Context", In, "Candidate")->AddAttributes(new ezScriptBaseClassFunctionAttribute(ScriptFunction::OnActivateBehavior)),
    EZ_SCRIPT_FUNCTION_PROPERTY(Reflection_OnReactivateBehavior, In, "Context", In, "Candidate")->AddAttributes(new ezScriptBaseClassFunctionAttribute(ScriptFunction::OnReactivateBehavior)),
    EZ_SCRIPT_FUNCTION_PROPERTY(Reflection_OnDeactivateBehavior, In, "Context")->AddAttributes(new ezScriptBaseClassFunctionAttribute(ScriptFunction::OnDeactivateBehavior)),
  }
  EZ_END_FUNCTIONS;
}
EZ_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

//////////////////////////////////////////////////////////////////////////

ezAiBehaviorScript::ezAiBehaviorScript() = default;
ezAiBehaviorScript::~ezAiBehaviorScript() = default;

void ezAiBehaviorScript::FlagNeededPerceptions(ezAiPerceptionManager& ref_PerceptionManager) const
{
  ref_PerceptionManager.FlagAsNeeded(m_Candidates);
}

ezAiBehaviorScore ezAiBehaviorScript::DetermineBehaviorScore(const ezAiContext& ctx, void* pInstanceData) const
{
  auto& data = *static_cast<InstanceData*>(pInstanceData);

  // instantiates the script on first use, a script that failed to load never runs
  GetFunction(ctx, data, ScriptFunction::CanRunBehavior);
  if (data.m_Script.GetScriptInstance() == nullptr)
    return {};

  if (m_Candidates.GetConnections().IsEmpty())
  {
    if (!CanRun(ctx, data, nullptr))
      return {};

    return MakeScore(ctx);
  }

  ezAiBehaviorScore score = ScoreCandidates<ezAiPerception>(ctx, m_Candidates, [&](const ezAiPerception& candidate)
    { return CanRun(ctx, data, &candidate); });

  if (score.GetScore() <= 0.0f && m_bKeepCandidate && data.m_bActive && data.m_pCandidate != nullptr)
  {
    // the candidate isn't perceived anymore, but the behavior isn't done with it yet
    score = MakeScore(ctx, data.m_pCandidate.Borrow());
  }

  return score;
}

void ezAiBehaviorScript::OnActivateBehavior(const ezAiContext& ctx, void* pInstanceData, const ezAiPerception* pPerception) const
{
  auto& data = *static_cast<InstanceData*>(pInstanceData);

  ctx.m_ActionQueue.CancelCurrentActions(ctx.m_Owner);

  data.m_bActive = true;

  if (m_bKeepCandidate && pPerception != data.m_pCandidate.Borrow())
  {
    data.m_pCandidate = pPerception ? pPerception->Clone() : nullptr;
  }

  if (auto pFunction = GetFunction(ctx, data, ScriptFunction::OnActivateBehavior))
  {
    ezAiScriptContext scriptCtx(ctx);
    scriptCtx.m_pActionQueue = &ctx.m_ActionQueue;
    ezVariant args[] = {&scriptCtx, pPerception};
    data.m_Script.Call(ctx, pFunction, args);
  }
}

void ezAiBehaviorScript::OnReactivateBehavior(const ezAiContext& ctx, void* pInstanceData, const ezAiPerception* pPerception) const
{
  auto& data = *static_cast<InstanceData*>(pInstanceData);

  auto pFunction = GetFunction(ctx, data, ScriptFunction::OnReactivateBehavior);
  if (pFunction == nullptr)
    return;

  ezAiScriptContext scriptCtx(ctx);
  scriptCtx.m_pActionQueue = &ctx.m_ActionQueue;
  ezVariant args[] = {&scriptCtx, pPerception};
  data.m_Script.Call(ctx, pFunction, args);

  if (scriptCtx.m_bRestart)
  {
    OnActivateBehavior(ctx, pInstanceData, pPerception);
  }
}

void ezAiBehaviorScript::OnDeactivateBehavior(const ezAiContext& ctx, void* pInstanceData) const
{
  auto& data = *static_cast<InstanceData*>(pInstanceData);

  data.m_bActive = false;
  data.m_pCandidate.Clear();

  if (auto pFunction = GetFunction(ctx, data, ScriptFunction::OnDeactivateBehavior))
  {
    ezAiScriptContext scriptCtx(ctx);
    ezVariant args[] = {&scriptCtx};
    data.m_Script.Call(ctx, pFunction, args);
  }
}

bool ezAiBehaviorScript::GetInstanceDataDesc(ezInstanceDataDesc& out_desc) const
{
  out_desc.FillFromType<InstanceData>();
  return true;
}

const ezAbstractFunctionProperty* ezAiBehaviorScript::GetFunction(const ezAiContext& ctx, InstanceData& ref_data, ezUInt32 uiFunction) const
{
  return ref_data.m_Script.GetFunction(ctx, uiFunction, const_cast<ezAiBehaviorScript&>(*this), GetStaticRTTI(), m_sScriptClass, m_Parameters);
}

bool ezAiBehaviorScript::CanRun(const ezAiContext& ctx, InstanceData& ref_data, const ezAiPerception* pCandidate) const
{
  auto pFunction = GetFunction(ctx, ref_data, ScriptFunction::CanRunBehavior);
  if (pFunction == nullptr)
    return true;

  ezAiScriptContext scriptCtx(ctx);
  ezVariant args[] = {&scriptCtx, pCandidate};
  ref_data.m_Script.Call(ctx, pFunction, args);

  return scriptCtx.m_bCanRun;
}

const ezRangeView<const char*, ezUInt32> ezAiBehaviorScript::GetParameters() const
{
  return ezRangeView<const char*, ezUInt32>([]() -> ezUInt32
    { return 0; },
    [this]() -> ezUInt32
    { return m_Parameters.GetCount(); },
    [](ezUInt32& ref_uiIt)
    { ++ref_uiIt; },
    [this](const ezUInt32& uiIt) -> const char*
    { return m_Parameters.GetKey(uiIt).GetString().GetData(); });
}

void ezAiBehaviorScript::SetParameter(const char* szKey, const ezVariant& value)
{
  ezHashedString hs;
  hs.Assign(szKey);
  m_Parameters[hs] = value;
}

void ezAiBehaviorScript::RemoveParameter(const char* szKey)
{
  m_Parameters.RemoveAndCopy(ezTempHashedString(szKey));
}

bool ezAiBehaviorScript::GetParameter(const char* szKey, ezVariant& out_value) const
{
  const ezUInt32 uiIndex = m_Parameters.Find(szKey);
  if (uiIndex == ezInvalidIndex)
    return false;

  out_value = m_Parameters.GetValue(uiIndex);
  return true;
}
