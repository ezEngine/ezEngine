#pragma once

#include <TypeScriptPlugin/TypeScriptPluginDLL.h>

#include <Core/Scripting/DuktapeWrapper.h>
#include <Foundation/Threading/Implementation/TaskSystemDeclarations.h>

class ezWorld;

class ezTypeScriptWrapper
{
public:
  ezTypeScriptWrapper();

  void Initialize(ezWorld* pWorld);

  void SetupScript();

  ezDuktapeWrapper m_Script;

public:
  static void StartLoadTranspiler();
  static void FinishLoadTranspiler();
  static ezResult TranspileFile(const char* szFile, ezStringBuilder& result);

private:
  static ezTaskGroupID s_LoadTranspilerTask;
  static ezDuktapeWrapper s_Transpiler;
};
