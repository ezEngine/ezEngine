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

  ezAsPreprocessor()
  {
    m_Processor.SetFileOpenFunction(ezMakeDelegate(&ezAsPreprocessor::PreProc_OpenFile, this));
    m_Processor.m_ProcessingEvents.AddEventHandler(ezMakeDelegate(&ezAsPreprocessor::PreProc_Event, this));
    m_Processor.SetImplicitPragmaOnce(true);
  }

  ezResult Process(ezStringBuilder& ref_sResult)
  {
    return m_Processor.Process(m_sRefFilePath, ref_sResult);
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

asIScriptModule* ezAngelScriptEngineSingleton::SetModuleCode(ezStringView sModuleName, ezStringView sCode, bool bAddExternalSection)
{
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

  if (int r = pModule->Build(); r < 0)
  {
    // TODO AngelScript: Forward compiler errors

    pModule->Discard();
    return nullptr;
  }

  return pModule;
}

asIScriptModule* ezAngelScriptEngineSingleton::CompileModule(ezStringView sModuleName, ezStringView sMainClass, ezStringView sRefFilePath, ezStringView sCode, ezStringBuilder* out_pProcessedCode)
{
  ezAsPreprocessor asPP;
  asPP.m_sRefFilePath = sRefFilePath;
  asPP.m_sMainCode = sCode;

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
