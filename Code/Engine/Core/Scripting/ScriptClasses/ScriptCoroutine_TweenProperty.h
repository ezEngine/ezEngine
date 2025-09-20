#pragma once

#include <Core/Scripting/ScriptCoroutine.h>
#include <Core/World/Declarations.h>
#include <Foundation/Math/CurveFunctions.h>

/// Script coroutine that animates a component property value over time.
///
/// Provides smooth interpolation between the current and target property values
/// using configurable easing curves. Supports any property type that can be
/// represented as a variant and interpolated.
class EZ_CORE_DLL ezScriptCoroutine_TweenProperty : public ezTypedScriptCoroutine<ezScriptCoroutine_TweenProperty, ezComponentHandle, ezStringView, ezVariant, ezTime, ezEnum<ezCurveFunction>>
{
public:
  /// Initiates the property animation to the specified target value.
  void Start(ezComponentHandle hComponent, ezStringView sPropertyName, ezVariant targetValue, ezTime duration, ezEnum<ezCurveFunction> easing);
  virtual Result Update(ezTime deltaTimeSinceLastUpdate) override;

private:
  const ezAbstractMemberProperty* m_pProperty = nullptr;
  ezComponentHandle m_hComponent;
  ezVariant m_SourceValue;
  ezVariant m_TargetValue;
  ezEnum<ezCurveFunction> m_Easing;

  ezTime m_Duration;
  ezTime m_TimePassed;
};

EZ_DECLARE_REFLECTABLE_TYPE(EZ_CORE_DLL, ezScriptCoroutine_TweenProperty);
