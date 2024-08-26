#include <Core/CorePCH.h>

#include <Core/Scripting/ScriptClasses/ScriptCoroutine_Wait.h>

// clang-format off
EZ_BEGIN_STATIC_REFLECTED_TYPE(ezScriptCoroutine_Wait, ezScriptCoroutine, 1, ezRTTIDefaultAllocator<ezScriptCoroutine_Wait>)
{
  EZ_BEGIN_FUNCTIONS
  {
    EZ_SCRIPT_FUNCTION_PROPERTY(Start, In, "Timeout"),
  }
  EZ_END_FUNCTIONS;
  EZ_BEGIN_ATTRIBUTES
  {
    new ezTitleAttribute("Coroutine::Wait {Timeout}"),
  }
  EZ_END_ATTRIBUTES;
}
EZ_END_STATIC_REFLECTED_TYPE;
// clang-format on

void ezScriptCoroutine_Wait::Start(ezTime timeout)
{
  m_TimeRemaing = timeout;
}

ezScriptCoroutine::Result ezScriptCoroutine_Wait::Update(ezTime deltaTimeSinceLastUpdate)
{
  m_TimeRemaing -= deltaTimeSinceLastUpdate;
  if (m_TimeRemaing.IsPositive())
  {
    // Don't wait for the full remaining time to prevent oversleeping due to scheduling precision.
    return Result::Running(m_TimeRemaing * 0.8);
  }

  return Result::Completed();
}


EZ_STATICLINK_FILE(Core, Core_Scripting_ScriptClasses_Implementation_ScriptCoroutine_Wait);
