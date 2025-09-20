#pragma once

#include <Core/Scripting/ScriptCoroutine.h>

/// Script coroutine that pauses execution for a specified duration.
///
/// Simple timing coroutine that delays script execution for a given time period.
/// Useful for creating delays in script sequences or implementing timed behaviors.
class EZ_CORE_DLL ezScriptCoroutine_Wait : public ezTypedScriptCoroutine<ezScriptCoroutine_Wait, ezTime>
{
public:
  /// Initiates the wait period for the specified duration.
  void Start(ezTime timeout);
  virtual Result Update(ezTime deltaTimeSinceLastUpdate) override;

private:
  ezTime m_TimeRemaing;
};

EZ_DECLARE_REFLECTABLE_TYPE(EZ_CORE_DLL, ezScriptCoroutine_Wait);
