#include <TypeScriptPlugin/TypeScriptPluginPCH.h>

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

  ezSharedPtr<ezTask> pTask = EZ_DEFAULT_NEW(ezDelegateTask<void>, "Load TypeScript Transpiler", ezTaskNesting::Never,
    [this]() //
    {
      EZ_PROFILE_SCOPE("Load TypeScript Transpiler");

      if (m_Transpiler.ExecuteFile("typescriptServices.js").Failed())
      {
        ezLog::Error("typescriptServices.js could not be loaded");
      }

      ezLog::Success("Loaded TypeScript transpiler.");
    });

  pTask->ConfigureTask("Load TypeScript Transpiler", ezTaskNesting::Never);
  m_LoadTaskGroup = ezTaskSystem::StartSingleTask(pTask, ezTaskPriority::LongRunning);
}

void ezTypeScriptTranspiler::FinishLoadTranspiler()
{
  StartLoadTranspiler();

  ezTaskSystem::WaitForGroup(m_LoadTaskGroup);
}

ezResult ezTypeScriptTranspiler::TranspileString(const char* szString, ezStringBuilder& out_sResult)
{
  EZ_LOG_BLOCK("TranspileString");

  FinishLoadTranspiler();

  EZ_PROFILE_SCOPE("Transpile TypeScript");

  ezDuktapeHelper duk(m_Transpiler);

  m_Transpiler.PushGlobalObject();                 // [ global ]
  if (m_Transpiler.PushLocalObject("ts").Failed()) // [ global ts ]
  {
    ezLog::Error("'ts' object does not exist");
    duk.PopStack(2);                               // [ ]
    EZ_DUK_RETURN_AND_VERIFY_STACK(duk, EZ_FAILURE, 0);
  }

  if (m_Transpiler.PrepareObjectFunctionCall("transpile").Failed()) // [ global ts transpile ]
  {
    ezLog::Error("'ts.transpile' function does not exist");
    duk.PopStack(3);                                                // [ ]
    EZ_DUK_RETURN_AND_VERIFY_STACK(duk, EZ_FAILURE, 0);
  }

  m_Transpiler.PushString(szString);                // [ global ts transpile source ]
  if (m_Transpiler.CallPreparedFunction().Failed()) // [ global ts result ]
  {
    ezLog::Error("String could not be transpiled");
    duk.PopStack(3);                                // [ ]
    EZ_DUK_RETURN_AND_VERIFY_STACK(duk, EZ_FAILURE, 0);
  }

  out_sResult = m_Transpiler.GetStringValue(-1); // [ global ts result ]
  m_Transpiler.PopStack(3);                      // [ ]

  EZ_DUK_RETURN_AND_VERIFY_STACK(duk, EZ_SUCCESS, 0);
}

ezResult ezTypeScriptTranspiler::TranspileFile(const char* szFile, ezUInt64 uiSkipIfFileHash, ezStringBuilder& out_sResult, ezUInt64& out_uiFileHash)
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

  if (m_ModifyTsBeforeTranspilationCB.IsValid())
  {
    m_ModifyTsBeforeTranspilationCB(source);
  }

  out_uiFileHash = ezHashingUtils::xxHash64(source.GetData(), source.GetElementCount());

  if (uiSkipIfFileHash == out_uiFileHash)
    return EZ_SUCCESS;

  return TranspileString(source, out_sResult);
}

ezResult ezTypeScriptTranspiler::TranspileFileAndStoreJS(const char* szFile, ezStringBuilder& out_sResult)
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
      out_sResult.ReadAll(fileIn);

      if (out_sResult.StartsWith_NoCase("/*SOURCE-HASH:"))
      {
        ezStringView sHashView = out_sResult.GetView();
        sHashView.Shrink(14, 0);

        // try to extract the hash
        if (ezConversionUtils::ConvertHexStringToUInt64(sHashView, uiExpectedFileHash).Failed())
        {
          uiExpectedFileHash = 0;
        }
      }
    }
  }

  EZ_SUCCEED_OR_RETURN(TranspileFile(szFile, uiExpectedFileHash, out_sResult, uiActualFileHash));

  if (uiExpectedFileHash != uiActualFileHash)
  {
    ezFileWriter fileOut;
    if (fileOut.Open(sOutFile).Failed())
    {
      ezLog::Error("Could not write transpiled JS to file '{}'", sOutFile);
      return EZ_FAILURE;
    }

    ezStringBuilder sHashHeader;
    sHashHeader.SetFormat("/*SOURCE-HASH:{}*/\n", ezArgU(uiActualFileHash, 16, true, 16, true));
    out_sResult.Prepend(sHashHeader);

    EZ_SUCCEED_OR_RETURN(fileOut.WriteBytes(out_sResult.GetData(), out_sResult.GetElementCount()));
    ezLog::Success("Transpiled '{}'", szFile);
  }

  return EZ_SUCCESS;
}

void ezTypeScriptTranspiler::SetModifyTsBeforeTranspilationCallback(ezDelegate<void(ezStringBuilder&)> callback)
{
  m_ModifyTsBeforeTranspilationCB = callback;
}
