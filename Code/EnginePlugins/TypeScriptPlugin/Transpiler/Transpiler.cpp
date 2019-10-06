#include <TypeScriptPluginPCH.h>

#include <Foundation/IO/FileSystem/FileReader.h>
#include <Foundation/IO/FileSystem/FileWriter.h>
#include <Foundation/Profiling/Profiling.h>
#include <Foundation/Threading/DelegateTask.h>
#include <Foundation/Threading/TaskSystem.h>
#include <Foundation/Utilities/ConversionUtils.h>
#include <TypeScriptPlugin/Transpiler/Transpiler.h>

ezTypeScriptTranspiler::ezTypeScriptTranspiler()
  : m_Transpiler("TypeScript Transpiler")
{
}

ezTypeScriptTranspiler::~ezTypeScriptTranspiler() = default;

void ezTypeScriptTranspiler::SetOutputFolder(const char* szFolder)
{
  m_sOutputFolder = szFolder;
}

void ezTypeScriptTranspiler::StartLoadTranspiler()
{
  if (m_LoadTaskGroup.IsValid())
    return;

  ezDelegateTask<void>* pTask = EZ_DEFAULT_NEW(ezDelegateTask<void>, "Load TypeScript Transpiler", [this]() //
    {
      EZ_PROFILE_SCOPE("Load TypeScript Transpiler");

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

  EZ_PROFILE_SCOPE("Transpile TypeScript");

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

ezResult ezTypeScriptTranspiler::TranspileFile(const char* szFile, ezUInt64 uiSkipIfFileHash, ezStringBuilder& out_Result, ezUInt64& out_uiFileHash)
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

  out_uiFileHash = ezHashingUtils::xxHash64(source.GetData(), source.GetElementCount());

  if (uiSkipIfFileHash == out_uiFileHash)
    return EZ_SUCCESS;

  return TranspileString(source, out_Result);
}

ezResult ezTypeScriptTranspiler::TranspileFileAndStoreJS(const char* szFile, ezStringBuilder& out_Result)
{
  EZ_LOG_BLOCK("TranspileFileAndStoreJS", szFile);

  EZ_ASSERT_DEV(!m_sOutputFolder.IsEmpty(), "Output folder has not been set");

  ezUInt64 uiExpectedFileHash = 0;
  ezUInt64 uiActualFileHash = 0;

  ezStringBuilder sOutFile = szFile;
  sOutFile.ChangeFileExtension("js");
  sOutFile.Prepend(m_sOutputFolder, "/");
  sOutFile.MakeCleanPath();

  {
    ezFileReader fileIn;
    if (fileIn.Open(sOutFile).Succeeded())
    {
      out_Result.ReadAll(fileIn);

      if (out_Result.StartsWith_NoCase("/*SOURCE-HASH:"))
      {
        ezStringView sHashView = out_Result.GetView();
        sHashView.Shrink(14, 0);

        // try to extract the hash
        if (ezConversionUtils::ConvertHexStringToUInt64(sHashView, uiExpectedFileHash).Failed())
        {
          uiExpectedFileHash = 0;
        }
      }
    }
  }

  EZ_SUCCEED_OR_RETURN(TranspileFile(szFile, uiExpectedFileHash, out_Result, uiActualFileHash));

  if (uiExpectedFileHash != uiActualFileHash)
  {
    ezFileWriter fileOut;
    if (fileOut.Open(sOutFile).Failed())
    {
      ezLog::Error("Could not write transpiled JS to file '{}'", sOutFile);
      return EZ_FAILURE;
    }

    ezStringBuilder sHashHeader;
    sHashHeader.Format("/*SOURCE-HASH:{}*/\n", ezArgU(uiActualFileHash, 16, true, 16, true));
    out_Result.Prepend(sHashHeader);

    fileOut.WriteBytes(out_Result.GetData(), out_Result.GetElementCount());
    ezLog::Success("Transpiled '{}'", szFile);
  }

  return EZ_SUCCESS;
}
