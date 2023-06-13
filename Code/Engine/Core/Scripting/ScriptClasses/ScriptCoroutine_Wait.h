#pragma once

#include <Core/Scripting/ScriptCoroutine.h>

class EZ_CORE_DLL ezScriptCoroutine_Wait : public ezTypedScriptCoroutine<ezScriptCoroutine_Wait, ezTime>
{
public:
  void Start(ezTime timeout);
  virtual Result Update(ezTime deltaTimeSinceLastUpdate) override;

private:
  ezTime m_TimeRemaing;
};

EZ_DECLARE_REFLECTABLE_TYPE(EZ_CORE_DLL, ezScriptCoroutine_Wait);
