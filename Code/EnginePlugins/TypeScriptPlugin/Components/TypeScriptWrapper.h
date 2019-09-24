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

  void SetModuleSearchPath(const char* szPath);

  ezDuktapeWrapper m_Script;

public:
  static void StartLoadTranspiler();
  static void FinishLoadTranspiler();
  static ezResult TranspileFile(const char* szFile, ezStringBuilder& result);

private:
  static int DukSearchModule(duk_context* pContext);

  static ezTaskGroupID s_LoadTranspilerTask;
  static ezDuktapeWrapper s_Transpiler;

  ezStringBuilder m_sSearchPath;
};
