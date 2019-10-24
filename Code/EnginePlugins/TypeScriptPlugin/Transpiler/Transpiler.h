#pragma once

#include <TypeScriptPlugin/TypeScriptPluginDLL.h>

#include <Core/Scripting/DuktapeContext.h>
#include <Foundation/Basics.h>
#include <Foundation/Threading/TaskSystem.h>

class EZ_TYPESCRIPTPLUGIN_DLL ezTypeScriptTranspiler
{
public:
  ezTypeScriptTranspiler();
  ~ezTypeScriptTranspiler();

  void SetOutputFolder(const char* szFolder);
  void StartLoadTranspiler();
  void FinishLoadTranspiler();
  ezResult TranspileString(const char* szString, ezStringBuilder& out_Result);
  ezResult TranspileFile(const char* szFile, ezUInt64 uiSkipIfFileHash, ezStringBuilder& out_Result, ezUInt64& out_uiFileHash);
  ezResult TranspileFileAndStoreJS(const char* szFile, ezStringBuilder& out_Result);

private:
  ezString m_sOutputFolder;
  ezTaskGroupID m_LoadTaskGroup;
  ezDuktapeContext m_Transpiler;
};
