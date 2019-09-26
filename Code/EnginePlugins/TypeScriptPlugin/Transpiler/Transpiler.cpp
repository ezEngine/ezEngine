#include <TypeScriptPluginPCH.h>

#include <Foundation/IO/FileSystem/FileReader.h>
#include <Foundation/IO/FileSystem/FileWriter.h>
#include <Foundation/Threading/DelegateTask.h>
#include <Foundation/Threading/TaskSystem.h>
#include <TypeScriptPlugin/Transpiler/Transpiler.h>

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
      if (m_Transpiler.ExecuteFile("typescriptServices.js").Failed())
      {
        ezLog::Error("typescriptServices.js could not be loaded");
      }
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
  EZ_LOG_BLOCK("TranspileString");

  FinishLoadTranspiler();

  ezDuktapeStackValidator validator(m_Transpiler);

  if (m_Transpiler.OpenObject("ts").Failed())
  {
    ezLog::Error("'ts' object does not exist");
    return EZ_FAILURE;
  }

  if (m_Transpiler.BeginFunctionCall("transpile").Failed())
  {
    ezLog::Error("'ts.transpile' function does not exist");
    return EZ_FAILURE;
  }

  m_Transpiler.PushParameter(szString);
  if (m_Transpiler.ExecuteFunctionCall().Failed())
  {
    ezLog::Error("String could not be transpiled");
    return EZ_FAILURE;
  }

  out_Result = m_Transpiler.GetStringReturnValue();

  m_Transpiler.EndFunctionCall();
  m_Transpiler.CloseObject();

  return EZ_SUCCESS;
}

ezResult ezTypeScriptTranspiler::TranspileFile(const char* szFile, ezStringBuilder& out_Result)
{
  EZ_LOG_BLOCK("TranspileFile", szFile);

  FinishLoadTranspiler();

  ezFileReader file;
  if (file.Open(szFile).Failed())
  {
    ezLog::Error("File does not exist: '{}'", szFile);
    return EZ_FAILURE;
  }

  ezStringBuilder source;
  source.ReadAll(file);

  return TranspileString(source, out_Result);
}

ezResult ezTypeScriptTranspiler::TranspileFileAndStoreJS(const char* szFile, ezStringBuilder& out_Result)
{
  EZ_LOG_BLOCK("TranspileFileAndStoreJS", szFile);

  EZ_SUCCEED_OR_RETURN(TranspileFile(szFile, out_Result));

  ezStringBuilder sOutFile = szFile;
  sOutFile.ChangeFileExtension("js");
  sOutFile.Prepend(":project/");

  ezFileWriter fileOut;
  if (fileOut.Open(sOutFile).Failed())
  {
    ezLog::Error("Could not write transpiled JS to file '{}'", sOutFile);
    return EZ_FAILURE;
  }

  fileOut.WriteBytes(out_Result.GetData(), out_Result.GetElementCount());

  return EZ_SUCCESS;
}
