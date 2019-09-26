#include <TypeScriptPluginPCH.h>

#include <Foundation/Threading/DelegateTask.h>
#include <Foundation/Threading/TaskSystem.h>
#include <TypeScriptPlugin/Transpiler/Transpiler.h>
#include <Foundation/IO/FileSystem/FileReader.h>
#include <Foundation/IO/FileSystem/FileWriter.h>

ezTypeScriptTranspiler::ezTypeScriptTranspiler()
  : m_Transpiler("TypeScript Transpiler")
{
}

ezTypeScriptTranspiler::~ezTypeScriptTranspiler() = default;

void ezTypeScriptTranspiler::StartLoadTranspiler()
{
  if (m_LoadTaskGroup.IsValid())
    return;

  ezDelegateTask<void>* pTask = EZ_DEFAULT_NEW(ezDelegateTask<void>, "Load TypeScript Transpiler", [this]() //
    {
      EZ_VERIFY(m_Transpiler.ExecuteFile("typescriptServices.js").Succeeded(), "");
    });

  pTask->SetOnTaskFinished([](ezTask* pTask) { EZ_DEFAULT_DELETE(pTask); });
  m_LoadTaskGroup = ezTaskSystem::StartSingleTask(pTask, ezTaskPriority::LongRunning);
}

void ezTypeScriptTranspiler::FinishLoadTranspiler()
{
  StartLoadTranspiler();

  ezTaskSystem::WaitForGroup(m_LoadTaskGroup);
}

ezResult ezTypeScriptTranspiler::TranspileString(const char* szString, ezStringBuilder& out_Result)
{
  FinishLoadTranspiler();

  ezDuktapeStackValidator validator(m_Transpiler);

  EZ_SUCCEED_OR_RETURN(m_Transpiler.OpenObject("ts"));
  EZ_SUCCEED_OR_RETURN(m_Transpiler.BeginFunctionCall("transpile"));
  m_Transpiler.PushParameter(szString);
  EZ_SUCCEED_OR_RETURN(m_Transpiler.ExecuteFunctionCall());

  out_Result = m_Transpiler.GetStringReturnValue();

  m_Transpiler.EndFunctionCall();
  m_Transpiler.CloseObject();

  return EZ_SUCCESS;
}

ezResult ezTypeScriptTranspiler::TranspileFile(const char* szFile, ezStringBuilder& out_Result)
{
  FinishLoadTranspiler();

  ezFileReader file;
  EZ_SUCCEED_OR_RETURN(file.Open(szFile));

  ezStringBuilder source;
  source.ReadAll(file);

  return TranspileString(source, out_Result);
}

ezResult ezTypeScriptTranspiler::TranspileFileAndStoreJS(const char* szFile, ezStringBuilder& out_Result)
{
  EZ_SUCCEED_OR_RETURN(TranspileFile(szFile, out_Result));

  ezStringBuilder sOutFile = szFile;
  sOutFile.ChangeFileExtension("js");
  sOutFile.Prepend(":project/");

  ezFileWriter fileOut;
  fileOut.Open(sOutFile);
  fileOut.WriteBytes(out_Result.GetData(), out_Result.GetElementCount());

  return EZ_SUCCESS;
}
