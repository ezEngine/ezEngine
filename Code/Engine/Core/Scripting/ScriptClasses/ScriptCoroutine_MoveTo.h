#pragma once

#include <Core/Scripting/ScriptCoroutine.h>
#include <Core/World/Declarations.h>
#include <Foundation/Math/CurveFunctions.h>

class EZ_CORE_DLL ezScriptCoroutine_MoveTo : public ezTypedScriptCoroutine<ezScriptCoroutine_MoveTo, ezGameObjectHandle, ezVec3, ezTime, ezEnum<ezCurveFunction>>
{
public:
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
