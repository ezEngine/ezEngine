#include <Core/CorePCH.h>

#include <Core/Scripting/ScriptClasses/ScriptCoroutine_TweenProperty.h>
#include <Core/World/World.h>
#include <Foundation/Reflection/ReflectionUtils.h>

// clang-format off
EZ_BEGIN_STATIC_REFLECTED_TYPE(ezScriptCoroutine_TweenProperty, ezScriptCoroutine, 1, ezRTTIDefaultAllocator<ezScriptCoroutine_TweenProperty>)
{
  EZ_BEGIN_FUNCTIONS
  {
    EZ_SCRIPT_FUNCTION_PROPERTY(Start, In, "Component", In, "PropertyName", In, "TargetValue", In, "Duration", In, "Easing"),
  }
  EZ_END_FUNCTIONS;
  EZ_BEGIN_ATTRIBUTES
  {
    new ezTitleAttribute("Coroutine::TweenProperty {PropertyName}"),
  }
  EZ_END_ATTRIBUTES;
}
EZ_END_STATIC_REFLECTED_TYPE;
// clang-format on

void ezScriptCoroutine_TweenProperty::Start(ezComponentHandle hComponent, ezStringView sPropertyName, ezVariant targetValue, ezTime duration, ezEnum<ezCurveFunction> easing)
{
  ezComponent* pComponent = nullptr;
  if (ezWorld::GetWorld(hComponent)->TryGetComponent(hComponent, pComponent) == false)
  {
    ezLog::Error("TweenProperty: The given component was not found.");
    return;
  }

  auto pType = pComponent->GetDynamicRTTI();
  auto pProp = pType->FindPropertyByName(sPropertyName);
  if (pProp == nullptr || pProp->GetCategory() != ezPropertyCategory::Member)
  {
    ezLog::Error("TweenProperty: The given component of type '{}' does not have a member property named '{}'.", pType->GetTypeName(), sPropertyName);
    return;
  }

  ezVariantType::Enum variantType = pProp->GetSpecificType()->GetVariantType();
  if (variantType == ezVariantType::Invalid)
  {
    ezLog::Error("TweenProperty: Can't tween property '{}' of type '{}'.", sPropertyName, pProp->GetSpecificType()->GetTypeName());
    return;
  }

  ezResult conversionStatus = EZ_SUCCESS;
  m_TargetValue = targetValue.ConvertTo(variantType, &conversionStatus);
  if (conversionStatus.Failed())
  {
    ezLog::Error("TweenProperty: Can't convert given target value to '{}'.", pProp->GetSpecificType()->GetTypeName());
    return;
  }

  m_pProperty = static_cast<ezAbstractMemberProperty*>(pProp);
  m_hComponent = hComponent;
  m_SourceValue = ezReflectionUtils::GetMemberPropertyValue(m_pProperty, pComponent);
  m_Easing = easing;

  m_Duration = duration;
  m_TimePassed = ezTime::Zero();
}

ezScriptCoroutine::Result ezScriptCoroutine_TweenProperty::Update(ezTime deltaTimeSinceLastUpdate)
{
  if (m_pProperty == nullptr)
  {
    return Result::Failed();
  }

  if (deltaTimeSinceLastUpdate.IsPositive())
  {
    ezComponent* pComponent = nullptr;
    if (ezWorld::GetWorld(m_hComponent)->TryGetComponent(m_hComponent, pComponent) == false)
    {
      return Result::Failed();
    }

    m_TimePassed += deltaTimeSinceLastUpdate;

    const double fDuration = m_Duration.GetSeconds();
    double fCurrentX = ezMath::Min(fDuration > 0 ? m_TimePassed.GetSeconds() / fDuration : 1.0, 1.0);
    fCurrentX = ezCurveFunction::GetValue(m_Easing, fCurrentX);
    ezVariant currentValue = ezMath::Lerp(m_SourceValue, m_TargetValue, fCurrentX);
    
    ezReflectionUtils::SetMemberPropertyValue(m_pProperty, pComponent, currentValue);
  }

  if (m_TimePassed < m_Duration)
  {
    return Result::Running();
  }

  return Result::Completed();
}
