#include <AiPlugin/AiPluginPCH.h>

#include <AiPlugin/UtilityAI/Framework/AiConsiderationManager.h>
#include <AiPlugin/UtilityAI/Framework/AiControllerDesc.h>
#include <AiPlugin/UtilityAI/Framework/AiInputPin.h>

// clang-format off
EZ_BEGIN_STATIC_REFLECTED_ENUM(ezAiResponseCurve, 1)
  EZ_ENUM_CONSTANTS(ezAiResponseCurve::Linear, ezAiResponseCurve::Quadratic, ezAiResponseCurve::InvQuadratic, ezAiResponseCurve::SmoothStep)
  EZ_ENUM_CONSTANTS(ezAiResponseCurve::Logistic, ezAiResponseCurve::Step)
EZ_END_STATIC_REFLECTED_ENUM;

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezAiConsideration, 1, ezRTTINoAllocator)
{
  EZ_BEGIN_PROPERTIES
  {
    EZ_MEMBER_PROPERTY("Name", m_sName),
  }
  EZ_END_PROPERTIES;
}
EZ_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

float ezAiConsideration::Evaluate(const ezAiContext& ctx, void* pInstanceData, const ezAiPerception* pCandidate) const
{
  return ApplyResponse(GetInputValue(ctx, pInstanceData, pCandidate));
}

float ezAiConsideration::ApplyResponse(float fValue) const
{
  float x;
  if (m_fInputMin == m_fInputMax)
    x = (fValue >= m_fInputMax) ? 1.0f : 0.0f;
  else
    x = ezMath::Saturate((fValue - m_fInputMin) / (m_fInputMax - m_fInputMin));

  float y = x;
  switch (m_Curve)
  {
    case ezAiResponseCurve::Linear:
      break;
    case ezAiResponseCurve::Quadratic:
      y = x * x;
      break;
    case ezAiResponseCurve::InvQuadratic:
      y = 1.0f - (1.0f - x) * (1.0f - x);
      break;
    case ezAiResponseCurve::SmoothStep:
      y = x * x * (3.0f - 2.0f * x);
      break;
    case ezAiResponseCurve::Logistic:
    {
      // normalized, so that 0 maps to 0 and 1 to 1
      constexpr float fSteepness = 12.0f;
      const float fLow = 1.0f / (1.0f + ezMath::Exp(fSteepness * 0.5f));
      const float fHigh = 1.0f / (1.0f + ezMath::Exp(-fSteepness * 0.5f));
      y = (1.0f / (1.0f + ezMath::Exp(-fSteepness * (x - 0.5f))) - fLow) / (fHigh - fLow);
      break;
    }
    case ezAiResponseCurve::Step:
      y = (x >= 0.5f) ? 1.0f : 0.0f;
      break;
  }

  y = ezMath::Saturate(y);
  return m_bInvert ? 1.0f - y : y;
}

ezStringView ezAiConsideration::GetDisplayName() const
{
  if (!m_sName.IsEmpty())
    return m_sName;

  ezStringView sName = GetDynamicRTTI()->GetTypeName();
  constexpr ezStringView sPrefix = "ezAiConsideration"_ezsv;
  if (sName.StartsWith(sPrefix) && sName.GetElementCount() > sPrefix.GetElementCount())
  {
    sName.Shrink(sPrefix.GetElementCount(), 0);
  }

  return sName;
}

//////////////////////////////////////////////////////////////////////////

ezAiConsiderationManager::ezAiConsiderationManager() = default;
ezAiConsiderationManager::~ezAiConsiderationManager() = default;

void ezAiConsiderationManager::Initialize(const ezAiControllerDesc& desc, ezByteBlobPtr instanceData)
{
  m_pDesc = &desc;
  m_InstanceData = instanceData;
  m_States.Clear();
  m_States.SetCount(desc.GetConsiderations().GetCount());
}

void ezAiConsiderationManager::Deinitialize()
{
  m_pDesc = nullptr;
  m_InstanceData.Clear();
  m_States.Clear();
}

void ezAiConsiderationManager::FlagNeededPerceptions(const ezAiInputPin& pin, ezAiPerceptionManager& ref_PerceptionManager) const
{
  for (ezUInt32 uiIndex : pin.GetConnections())
  {
    GetConsideration(uiIndex)->FlagNeededPerceptions(ref_PerceptionManager);
  }
}

float ezAiConsiderationManager::Evaluate(const ezAiContext& ctx, ezUInt32 uiConsiderationIndex, const ezAiPerception* pCandidate)
{
  const ezAiConsideration* pConsideration = GetConsideration(uiConsiderationIndex);
  State& state = m_States[uiConsiderationIndex];

  if (!pConsideration->UsesCandidate())
  {
    pCandidate = nullptr;
  }

  if (state.m_CacheTime != ctx.m_Now)
  {
    state.m_CacheTime = ctx.m_Now;
    state.m_Cache.Clear();
  }

  for (const CacheEntry& entry : state.m_Cache)
  {
    if (entry.m_pCandidate == pCandidate)
      return entry.m_fValue;
  }

  CacheEntry& entry = state.m_Cache.ExpandAndGetRef();
  entry.m_pCandidate = pCandidate;
  entry.m_fValue = pConsideration->Evaluate(ctx, GetInstanceData(uiConsiderationIndex), pCandidate);
  return entry.m_fValue;
}

const ezAiConsideration* ezAiConsiderationManager::GetConsideration(ezUInt32 uiConsiderationIndex) const
{
  return m_pDesc->GetConsiderations()[uiConsiderationIndex].m_pConsideration.Borrow();
}

void* ezAiConsiderationManager::GetInstanceData(ezUInt32 uiConsiderationIndex) const
{
  return ezInstanceDataAllocator::GetInstanceData(m_InstanceData, m_pDesc->GetConsiderations()[uiConsiderationIndex].m_uiInstanceDataOffset);
}
