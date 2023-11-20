#include <AiPlugin/AiPluginPCH.h>

#include <AiPlugin/UtilityAI/Framework/AiPerceptionManager.h>
#include <AiPlugin/UtilityAI/Framework/AiScript.h>
#include <AiPlugin/UtilityAI/Impl/GameAiConsiderations.h>
#include <AiPlugin/UtilityAI/Impl/GameAiPerceptions.h>
#include <Core/Scripting/ScriptAttributes.h>
#include <Core/World/GameObject.h>
#include <Foundation/Memory/InstanceDataAllocator.h>
#include <RendererCore/Components/BlackboardComponent.h>

// clang-format off
EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezAiConsiderationTargetDistance, 1, ezRTTIDefaultAllocator<ezAiConsiderationTargetDistance>)
{
  EZ_BEGIN_PROPERTIES
  {
    EZ_AI_CONSIDERATION_INPUT_PROPERTIES(0.0f, 20.0f, ezAiResponseCurve::Linear, true),
  }
  EZ_END_PROPERTIES;
}
EZ_END_DYNAMIC_REFLECTED_TYPE;

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezAiConsiderationBlackboard, 1, ezRTTIDefaultAllocator<ezAiConsiderationBlackboard>)
{
  EZ_BEGIN_PROPERTIES
  {
    EZ_MEMBER_PROPERTY("EntryName", m_sEntryName),
    EZ_MEMBER_PROPERTY("Fallback", m_fFallback),
    EZ_AI_CONSIDERATION_INPUT_PROPERTIES(0.0f, 1.0f, ezAiResponseCurve::Linear, false),
  }
  EZ_END_PROPERTIES;
}
EZ_END_DYNAMIC_REFLECTED_TYPE;

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezAiConsiderationScript, 1, ezRTTIDefaultAllocator<ezAiConsiderationScript>)
{
  EZ_BEGIN_PROPERTIES
  {
    EZ_MEMBER_PROPERTY("ScriptClass", m_sScriptClass)->AddAttributes(new ezAssetBrowserAttribute("CompatibleAsset_ScriptClass"), new ezRequiredAttribute()),
    EZ_MAP_ACCESSOR_PROPERTY("Parameters", GetParameters, GetParameter, SetParameter, RemoveParameter)->AddAttributes(new ezExposedParametersAttribute("ScriptClass")),
    EZ_MEMBER_PROPERTY("UsesCandidate", m_bUsesCandidate)->AddAttributes(new ezDefaultValueAttribute(true)),
    EZ_MEMBER_PROPERTY("Perceptions", m_Perceptions)->AddAttributes(new ezAiInputPinAttribute("ezAiPerception"), new ezHiddenAttribute()),
    EZ_AI_CONSIDERATION_INPUT_PROPERTIES(0.0f, 1.0f, ezAiResponseCurve::Linear, false),
  }
  EZ_END_PROPERTIES;
  EZ_BEGIN_FUNCTIONS
  {
    EZ_SCRIPT_FUNCTION_PROPERTY(Reflection_Evaluate, In, "Context", In, "Candidate")->AddAttributes(new ezScriptBaseClassFunctionAttribute(0)),
  }
  EZ_END_FUNCTIONS;
}
EZ_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

ezAiConsiderationTargetDistance::ezAiConsiderationTargetDistance() = default;

float ezAiConsiderationTargetDistance::GetInputValue(const ezAiContext& ctx, void* pInstanceData, const ezAiPerception* pCandidate) const
{
  const ezAiPerceptionTarget* pTarget = ezDynamicCast<const ezAiPerceptionTarget*>(pCandidate);
  return pTarget ? pTarget->m_fDistance : 0.0f;
}

//////////////////////////////////////////////////////////////////////////

ezAiConsiderationBlackboard::ezAiConsiderationBlackboard() = default;

float ezAiConsiderationBlackboard::GetInputValue(const ezAiContext& ctx, void* pInstanceData, const ezAiPerception* pCandidate) const
{
  auto pBlackboard = ezBlackboardComponent::FindBlackboard(ctx.m_Owner);
  if (pBlackboard == nullptr)
    return m_fFallback;

  const ezVariant value = pBlackboard->GetEntryValue(m_sEntryName);
  if (!value.CanConvertTo<float>())
    return m_fFallback;

  return value.ConvertTo<float>();
}

//////////////////////////////////////////////////////////////////////////

ezAiConsiderationScript::ezAiConsiderationScript() = default;
ezAiConsiderationScript::~ezAiConsiderationScript() = default;

void ezAiConsiderationScript::FlagNeededPerceptions(ezAiPerceptionManager& ref_PerceptionManager) const
{
  ref_PerceptionManager.FlagAsNeeded(m_Perceptions);
}

float ezAiConsiderationScript::GetInputValue(const ezAiContext& ctx, void* pInstanceData, const ezAiPerception* pCandidate) const
{
  auto& script = *static_cast<ezAiScriptInstance*>(pInstanceData);

  auto pFunction = script.GetFunction(ctx, 0, const_cast<ezAiConsiderationScript&>(*this), GetStaticRTTI(), m_sScriptClass, m_Parameters);
  if (pFunction == nullptr)
    return 0.0f;

  ezAiScriptContext scriptCtx(ctx);
  ctx.m_PerceptionManager.GetPerceptions(m_Perceptions, scriptCtx.m_Perceptions);

  ezVariant args[] = {&scriptCtx, pCandidate};
  script.Call(ctx, pFunction, args);

  return scriptCtx.m_fValue;
}

bool ezAiConsiderationScript::GetInstanceDataDesc(ezInstanceDataDesc& out_desc) const
{
  out_desc.FillFromType<ezAiScriptInstance>();
  return true;
}

void ezAiConsiderationScript::Reflection_Evaluate(ezAiScriptContext* pContext, const ezAiPerception* pCandidate)
{
}

const ezRangeView<const char*, ezUInt32> ezAiConsiderationScript::GetParameters() const
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

void ezAiConsiderationScript::SetParameter(const char* szKey, const ezVariant& value)
{
  ezHashedString hs;
  hs.Assign(szKey);
  m_Parameters[hs] = value;
}

void ezAiConsiderationScript::RemoveParameter(const char* szKey)
{
  m_Parameters.RemoveAndCopy(ezTempHashedString(szKey));
}

bool ezAiConsiderationScript::GetParameter(const char* szKey, ezVariant& out_value) const
{
  const ezUInt32 uiIndex = m_Parameters.Find(szKey);
  if (uiIndex == ezInvalidIndex)
    return false;

  out_value = m_Parameters.GetValue(uiIndex);
  return true;
}
