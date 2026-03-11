#pragma once

#include <Core/Scripting/ScriptCoroutine.h>
#include <Core/World/Declarations.h>
#include <Foundation/Math/CurveFunctions.h>

/// Script coroutine that smoothly moves a game object to a target position over time.
///
/// Provides interpolated movement with configurable easing curves for animation effects.
/// The object's position is updated each frame until the target is reached or the duration expires.
class EZ_CORE_DLL ezScriptCoroutine_MoveTo : public ezTypedScriptCoroutine<ezScriptCoroutine_MoveTo, ezGameObjectHandle, ezVec3, ezTime, ezEnum<ezCurveFunction>>
{
public:
  /// Initiates the move operation to the specified target position.
  void Start(ezGameObjectHandle hObject, const ezVec3& vTargetPos, ezTime duration, ezEnum<ezCurveFunction> easing);
  virtual Result Update(ezTime deltaTimeSinceLastUpdate) override;

private:
  ezGameObjectHandle m_hObject;
  ezVec3 m_vSourcePos;
  ezVec3 m_vTargetPos;
  ezEnum<ezCurveFunction> m_Easing;

  ezTime m_Duration;
  ezTime m_TimePassed;
};

EZ_DECLARE_REFLECTABLE_TYPE(EZ_CORE_DLL, ezScriptCoroutine_MoveTo);
