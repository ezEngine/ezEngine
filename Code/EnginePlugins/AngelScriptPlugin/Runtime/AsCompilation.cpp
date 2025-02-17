#include <AngelScriptPlugin/AngelScriptPluginPCH.h>

#include <AngelScript/include/angelscript.h>
#include <AngelScriptPlugin/Runtime/AsEngineSingleton.h>
#include <Foundation/CodeUtils/Preprocessor.h>
#include <Foundation/IO/FileSystem/FileReader.h>

class ezAsPreprocessor
{
public:
  ezStringView m_sRefFilePath;
  ezStringView m_sMainCode;
  ezSet<ezString>* m_pDependencies = nullptr;

  ezAsPreprocessor()
  {
    m_Processor.SetFileOpenFunction(ezMakeDelegate(&ezAsPreprocessor::PreProc_OpenFile, this));
    m_Processor.m_ProcessingEvents.AddEventHandler(ezMakeDelegate(&ezAsPreprocessor::PreProc_Event, this));
    m_Processor.SetImplicitPragmaOnce(true);
    m_Processor.SetPassThroughLine(true);
  }

  ezResult Process(ezStringBuilder& ref_sResult)
  {
    const bool bNeedsLineStmts = !m_sMainCode.StartsWith("//#ln");
    const bool bKeepComments = !bNeedsLineStmts;

    auto res = m_Processor.Process(m_sRefFilePath, ref_sResult, bKeepComments, false, bNeedsLineStmts);
    ref_sResult.ReplaceAll("#line", "//#ln");
    return res;
  };

private:
  ezResult PreProc_OpenFile(ezStringView sAbsFile, ezDynamicArray<ezUInt8>& out_Content, ezTimestamp& out_FileModification)
  {
    if (sAbsFile == m_sRefFilePath)
    {
      out_Content.SetCount(m_sMainCode.GetElementCount());
      ezMemoryUtils::RawByteCopy(out_Content.GetData(), m_sMainCode.GetStartPointer(), m_sMainCode.GetElementCount());
      return EZ_SUCCESS;
    }

    ezFileReader file;
    if (file.Open(sAbsFile).Failed())
      return EZ_FAILURE;

    if (m_pDependencies)
    {
      m_pDependencies->Insert(sAbsFile);
    }

    out_Content.SetCountUninitialized((ezUInt32)file.GetFileSize());
    file.ReadBytes(out_Content.GetData(), out_Content.GetCount());
    return EZ_SUCCESS;
  }

  void PreProc_Event(const ezPreprocessor::ProcessingEvent& event)
  {
    switch (event.m_Type)
    {
      case ezPreprocessor::ProcessingEvent::Error:
        ezLog::Error("{0}: Line {1} [{2}]: {}", event.m_pToken->m_File.GetString(), event.m_pToken->m_uiLine, event.m_pToken->m_uiColumn, event.m_sInfo);
        break;
      case ezPreprocessor::ProcessingEvent::Warning:
        ezLog::Warning("{0}: Line {1} [{2}]: {}", event.m_pToken->m_File.GetString(), event.m_pToken->m_uiLine, event.m_pToken->m_uiColumn, event.m_sInfo);
        break;
      default:
        break;
    }
  }

  ezPreprocessor m_Processor;
};

void ezAngelScriptEngineSingleton::CompilerMessageCallback(const asSMessageInfo* msg)
{
  ezDynamicArray<ezStringView> lines;
  m_sCodeInCompilation.Split(true, lines, "\n");

  ezInt32 iLine = msg->row;
  ezStringView sSection = msg->section;

  if (iLine - 1 < (ezInt32)lines.GetCount())
  {
    --iLine;

    int iStepsBack = 0;

    while (iLine >= 0)
    {
      if (lines[iLine].StartsWith("//#ln"))
      {
        ezStringView line = lines[iLine];
        line.TrimWordStart("//#ln ");

        const char* szParsePos;
        ezConversionUtils::StringToInt(line, iLine, &szParsePos).AssertSuccess();

        line.SetStartPosition(szParsePos + 1);
        line.Trim("\"");

        sSection = line;
        iLine += iStepsBack - 1;
        break;
      }

      --iLine;
      ++iStepsBack;
    }
  }

  switch (msg->type)
  {
    case asMSGTYPE_ERROR:
      ezLog::Error("{} ({}, {}) : {}", sSection, iLine, msg->col, msg->message);
      break;
    case asMSGTYPE_WARNING:
      ezLog::Warning("{} ({}, {}) : {}", sSection, iLine, msg->col, msg->message);
      break;
    case asMSGTYPE_INFORMATION:
      ezLog::Info("{} ({}, {}) : {}", sSection, iLine, msg->col, msg->message);
      break;
  }
}

asIScriptModule* ezAngelScriptEngineSingleton::SetModuleCode(ezStringView sModuleName, ezStringView sCode, bool bAddExternalSection)
{
  EZ_LOCK(m_CompilerMutex);

  m_sCodeInCompilation = sCode;

  ezStringBuilder tmp;
  asIScriptModule* pModule = m_pEngine->GetModule(sModuleName.GetData(tmp), asGM_ALWAYS_CREATE);

  if (bAddExternalSection)
  {
    const char* szExternal = R"(
external shared class ezAngelScriptClass;
)";

    pModule->AddScriptSection("External", szExternal);
  }

  pModule->AddScriptSection("Main", sCode.GetStartPointer(), sCode.GetElementCount());

  const int res = pModule->Build();
  switch (res)
  {
    case asBUILD_IN_PROGRESS:
      ezLog::Error("AS: Another compilation is in progress.");
      break;

    case asINVALID_CONFIGURATION:
      ezLog::Error("AS: Invalid Configuration.");
      break;

    case asINIT_GLOBAL_VARS_FAILED:
      ezLog::Error("AS: Global Variable initialization failed.");
      break;

    case asNOT_SUPPORTED:
      ezLog::Error("AS: Compiler support is disabled in the engine.");
      break;

    case asMODULE_IS_IN_USE:
      ezLog::Error("AS: Module is in use.");
      break;

    case asERROR:
      break;
  }

  if (res < 0)
  {
    // TODO AngelScript: Forward compiler errors
    pModule->Discard();
    return nullptr;
  }

  return pModule;
}

asIScriptModule* ezAngelScriptEngineSingleton::CompileModule(ezStringView sModuleName, ezStringView sMainClass, ezStringView sRefFilePath, ezStringView sCode, ezStringBuilder* out_pProcessedCode, ezSet<ezString>* out_pDependencies)
{
  ezAsPreprocessor asPP;
  asPP.m_sRefFilePath = sRefFilePath;
  asPP.m_sMainCode = sCode;
  asPP.m_pDependencies = out_pDependencies;

  ezStringBuilder fullCode;
  if (asPP.Process(fullCode).Failed())
  {
    ezLog::Error("Failed to pre-process AngelScript");
    return nullptr;
  }

  if (out_pProcessedCode)
  {
    *out_pProcessedCode = fullCode;
  }

  asIScriptModule* pModule = SetModuleCode(sModuleName, fullCode, true);

  if (pModule == nullptr)
    return nullptr;

  ezStringBuilder tmp;
  const asITypeInfo* pClassType = pModule->GetTypeInfoByName(sMainClass.GetData(tmp));

  if (pClassType == nullptr)
  {
    ezLog::Error("AngelScript code doesn't contain class '{}'", sMainClass);
    return nullptr;
  }

  if (ValidateModule(pModule).Failed())
  {
    return nullptr;
  }

  return pModule;
}


ezResult ezAngelScriptEngineSingleton::ValidateModule(asIScriptModule* pModule) const
{
  ezResult res = EZ_SUCCESS;

  for (ezUInt32 i = 0; i < pModule->GetGlobalVarCount(); ++i)
  {
    const char* szName;
    int typeId;

    if (pModule->GetGlobalVar(i, &szName, nullptr, &typeId) == asSUCCESS)
    {
      if (const asITypeInfo* pInfo = pModule->GetEngine()->GetTypeInfoById(typeId))
      {
        if (IsTypeForbidden(pInfo))
        {
          ezLog::Error("Global variable '{}' uses forbidden type '{}'", szName, pInfo->GetName());
          res = EZ_FAILURE;
        }
      }
    }
  }

  for (ezUInt32 i = 0; i < pModule->GetObjectTypeCount(); ++i)
  {
    const asITypeInfo* pType = pModule->GetObjectTypeByIndex(i);

    for (ezUInt32 i2 = 0; i2 < pType->GetPropertyCount(); ++i2)
    {
      const char* szName;
      int typeId;

      pType->GetProperty(i2, &szName, &typeId);

      if (const asITypeInfo* pInfo = pModule->GetEngine()->GetTypeInfoById(typeId))
      {
        if (IsTypeForbidden(pInfo))
        {
          ezLog::Error("Property '{}::{}' uses forbidden type '{}'", pType->GetName(), szName, pInfo->GetName());
          res = EZ_FAILURE;
        }
      }
    }
  }

  return res;
}
