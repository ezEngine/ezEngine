#include <Core/CorePCH.h>

#include <Core/Scripting/ScriptClasses/ScriptCoroutine_MoveTo.h>
#include <Core/World/World.h>

// clang-format off
EZ_BEGIN_STATIC_REFLECTED_TYPE(ezScriptCoroutine_MoveTo, ezScriptCoroutine, 1, ezRTTIDefaultAllocator<ezScriptCoroutine_MoveTo>)
{
  EZ_BEGIN_FUNCTIONS
  {
    EZ_SCRIPT_FUNCTION_PROPERTY(Start, In, "Object", In, "TargetPos", In, "Duration", In, "Easing"),
  }
  EZ_END_FUNCTIONS;
  EZ_BEGIN_ATTRIBUTES
  {
    new ezTitleAttribute("Coroutine::MoveTo {TargetPos}"),
  }
  EZ_END_ATTRIBUTES;
}
EZ_END_STATIC_REFLECTED_TYPE;
// clang-format on

void ezScriptCoroutine_MoveTo::Start(ezGameObjectHandle hObject, const ezVec3& vTargetPos, ezTime duration, ezEnum<ezCurveFunction> easing)
{
  ezGameObject* pObject = nullptr;
  if (ezWorld::GetWorld(hObject)->TryGetObject(hObject, pObject) == false)
  {
    ezLog::Error("MoveTo: The given game object was not found.");
    return;
  }

  m_hObject = hObject;
  m_vSourcePos = pObject->GetLocalPosition();
  m_vTargetPos = vTargetPos;
  m_Easing = easing;

  m_Duration = duration;
  m_TimePassed = ezTime::MakeZero();
}

ezScriptCoroutine::Result ezScriptCoroutine_MoveTo::Update(ezTime deltaTimeSinceLastUpdate)
{
  if (deltaTimeSinceLastUpdate.IsPositive())
  {
    ezGameObject* pObject = nullptr;
    if (ezWorld::GetWorld(m_hObject)->TryGetObject(m_hObject, pObject) == false)
    {
      return Result::Failed();
    }

    m_TimePassed += deltaTimeSinceLastUpdate;

    const double fDuration = m_Duration.GetSeconds();
    double fCurrentX = ezMath::Min(fDuration > 0 ? m_TimePassed.GetSeconds() / fDuration : 1.0, 1.0);
    fCurrentX = ezCurveFunction::GetValue(m_Easing, fCurrentX);

    ezVec3 vCurrentPos = ezMath::Lerp(m_vSourcePos, m_vTargetPos, static_cast<float>(fCurrentX));
    pObject->SetLocalPosition(vCurrentPos);
  }

  if (m_TimePassed < m_Duration)
  {
    return Result::Running();
  }

  return Result::Completed();
}


EZ_STATICLINK_FILE(Core, Core_Scripting_ScriptClasses_Implementation_ScriptCoroutine_MoveTo);
