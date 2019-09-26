#pragma once

#include <TypeScriptPlugin/TypeScriptPluginDLL.h>

#include <Core/Scripting/DuktapeWrapper.h>
#include <Foundation/Threading/Implementation/TaskSystemDeclarations.h>
#include <Foundation/Types/Status.h>

class ezWorld;

class ezTypeScriptWrapper
{
public:
  ezTypeScriptWrapper();

  void Initialize(ezWorld* pWorld);

  ezStatus SetupScript();

  void SetModuleSearchPath(const char* szPath);

  ezDuktapeWrapper m_Script;

public:
  static void StartLoadTranspiler();
  static void FinishLoadTranspiler();
  static ezResult TranspileFile(const char* szFile, ezStringBuilder& result);

private:
  static int DukSearchModule(duk_context* pContext);

  ezStatus Init_RequireModules();
  ezStatus Init_Log();

  static ezTaskGroupID s_LoadTranspilerTask;
  static ezDuktapeWrapper s_Transpiler;
};
