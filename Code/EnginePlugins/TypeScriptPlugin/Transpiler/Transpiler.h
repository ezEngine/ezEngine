#pragma once

#include <Core/Scripting/DuktapeWrapper.h>
#include <Foundation/Basics.h>
#include <Foundation/Threading/TaskSystem.h>

class ezTypeScriptTranspiler
{
public:
  ezTypeScriptTranspiler();
  ~ezTypeScriptTranspiler();

  void StartLoadTranspiler();
  void FinishLoadTranspiler();
  ezResult TranspileString(const char* szString, ezStringBuilder& out_Result);
  ezResult TranspileFile(const char* szFile, ezStringBuilder& out_Result);
  ezResult TranspileFileAndStoreJS(const char* szFile, ezStringBuilder& out_Result);

private:
  ezTaskGroupID m_LoadTaskGroup;
  ezDuktapeWrapper m_Transpiler;
};
