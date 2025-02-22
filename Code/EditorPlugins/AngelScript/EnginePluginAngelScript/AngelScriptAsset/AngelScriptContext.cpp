#include <EnginePluginAngelScript/EnginePluginAngelScriptPCH.h>

#include <AngelScript/include/angelscript.h>
#include <AngelScriptPlugin/Runtime/AsEngineSingleton.h>
#include <AngelScriptPlugin/Runtime/AsInstance.h>
#include <AngelScriptPlugin/Utils/AngelScriptUtils.h>
#include <EnginePluginAngelScript/AngelScriptAsset/AngelScriptContext.h>
#include <Foundation/IO/CompressedStreamZstd.h>
#include <Foundation/IO/FileSystem/DeferredFileWriter.h>
#include <Foundation/IO/FileSystem/FileReader.h>
#include <Foundation/Utilities/AssetFileHeader.h>

// clang-format off
EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezAngelScriptDocumentContext, 1, ezRTTIDefaultAllocator<ezAngelScriptDocumentContext>)
{
  EZ_BEGIN_PROPERTIES
  {
    EZ_CONSTANT_PROPERTY("DocumentType", (const char*) "AngelScript"),
  }
  EZ_END_PROPERTIES;
}
EZ_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

static ezAtomicInteger32 s_iCompileCounter;

ezAngelScriptDocumentContext::ezAngelScriptDocumentContext()
  : ezEngineProcessDocumentContext(ezEngineProcessDocumentContextFlags::CreateWorld)
{
}

ezAngelScriptDocumentContext::~ezAngelScriptDocumentContext() = default;

ezStatus ezAngelScriptDocumentContext::ExportDocument(const ezExportDocumentMsgToEngine* pMsg)
{
  ezLogSystemToBuffer logBuffer;
  logBuffer.SetLogLevel(ezLogMsgType::ErrorMsg);

  ezLogSystemScope logScope(&logBuffer);

  ezStringBuilder sCode;
  asIScriptModule* pModule = CompileModule(sCode, nullptr);

  if (pModule == nullptr)
  {
    return ezStatus(logBuffer.m_sBuffer.GetView());
  }

  ezHybridArray<ezUInt8, 1024 * 8> bytecode;
  ezAngelScriptUtils::SaveByteCode(pModule, bytecode);
  pModule->Discard();

  ezDeferredFileWriter out;
  out.SetOutput(pMsg->m_sOutputFile);

  {
    // File Header
    ezAssetFileHeader header;
    header.SetFileHashAndVersion(pMsg->m_uiAssetHash, pMsg->m_uiVersion);
    header.Write(out).AssertSuccess();

    ezUInt8 uiVersion = 4;
    out << uiVersion;
  }

  ezUInt8 uiCompressionMode = 0;

#ifdef BUILDSYSTEM_ENABLE_ZSTD_SUPPORT
  uiCompressionMode = 1;
  ezCompressedStreamWriterZstd stream(&out, 0, ezCompressedStreamWriterZstd::Compression::Average);
#else
  ezStreamWriter& stream = out;
#endif

  // write uncompressed
  out << uiCompressionMode;

  // write the rest compressed
  stream << m_sClass;
  stream.WriteArray(bytecode).AssertSuccess();
  stream << sCode;

  return ezStatus(EZ_SUCCESS);
}

void ezAngelScriptDocumentContext::HandleMessage(const ezEditorEngineDocumentMsg* pMsg)
{
  if (const ezDocumentConfigMsgToEngine* pMsg2 = ezDynamicCast<const ezDocumentConfigMsgToEngine*>(pMsg))
  {
    if (pMsg2->m_sWhatToDo == "InputFile")
    {
      m_sInputFile = pMsg2->m_sValue;
    }
    else if (pMsg2->m_sWhatToDo == "Code")
    {
      m_sCode = pMsg2->m_sValue;
    }
    else if (pMsg2->m_sWhatToDo == "Class")
    {
      m_sClass = pMsg2->m_sValue;
    }
    else if (pMsg2->m_sWhatToDo == "SyncExposedParams")
    {
      SyncExposedParameters();
    }
    else if (pMsg2->m_sWhatToDo == "RetrieveScriptInfos")
    {
      RetrieveScriptInfos(pMsg2->m_sValue);
    }
  }

  ezEngineProcessDocumentContext::HandleMessage(pMsg);
}

ezEngineProcessViewContext* ezAngelScriptDocumentContext::CreateViewContext()
{
  EZ_ASSERT_NOT_IMPLEMENTED;
  return nullptr;
}

void ezAngelScriptDocumentContext::DestroyViewContext(ezEngineProcessViewContext* pContext)
{
  EZ_ASSERT_NOT_IMPLEMENTED;
}

class ezLogSystemNull : public ezLogInterface
{
public:
  void HandleLogMessage(const ezLoggingEventData& le) override
  {
  }
};

void ezAngelScriptDocumentContext::SyncExposedParameters()
{
  ezLogSystemNull logNull;
  ezLogSystemScope scope(&logNull); // disable logging

  ezStringBuilder sCode;
  ezSet<ezString> dependencies;
  asIScriptModule* pModule = CompileModule(sCode, &dependencies);
  if (pModule == nullptr)
    return;

  EZ_SCOPE_EXIT(pModule->Discard());

  const asITypeInfo* pClassType = pModule->GetTypeInfoByName(m_sClass);

  asIScriptContext* pContext = pModule->GetEngine()->CreateContext();
  EZ_SCOPE_EXIT(pContext->Release());

  AS_CHECK(pContext->Prepare(pClassType->GetFactoryByIndex(0)));
  AS_CHECK(pContext->Execute());
  asIScriptObject* pInstance = (asIScriptObject*)pContext->GetReturnObject();
  pInstance->AddRef();
  EZ_SCOPE_EXIT(pInstance->Release());

  ezStringBuilder sTypeName;

  {
    ezSimpleDocumentConfigMsgToEditor msg;
    msg.m_DocumentGuid = m_DocumentGuid;
    msg.m_sWhatToDo = "SyncExposedParams_Clear";
    SendProcessMessage(&msg);
  }

  for (ezUInt32 idx = 0; idx < pClassType->GetPropertyCount(); ++idx)
  {
    const char* szName;
    int typeId;

    bool isPrivate = false, isProtected = false, isReference = false;
    pClassType->GetProperty(idx, &szName, &typeId, &isPrivate, &isProtected, nullptr, &isReference);

    if (isPrivate || isProtected)
      continue;

    ezVariant defVal;
    if (ezAngelScriptUtils::ReadFromAsTypeAtLocation(pModule->GetEngine(), typeId, pInstance->GetAddressOfProperty(idx), defVal).Succeeded())
    {
      ezSimpleDocumentConfigMsgToEditor msg;
      msg.m_DocumentGuid = m_DocumentGuid;
      msg.m_sWhatToDo = "SyncExposedParams_Add";
      msg.m_sPayload = szName;
      msg.m_PayloadValue = defVal;
      SendProcessMessage(&msg);
    }
  }

  for (const auto& dep : dependencies)
  {
    ezSimpleDocumentConfigMsgToEditor msg;
    msg.m_DocumentGuid = m_DocumentGuid;
    msg.m_sWhatToDo = "SyncDependencies_Add";
    msg.m_sPayload = dep;
    SendProcessMessage(&msg);
  }

  {
    ezSimpleDocumentConfigMsgToEditor msg;
    msg.m_DocumentGuid = m_DocumentGuid;
    msg.m_sWhatToDo = "SyncExposedParams_Finish";
    SendProcessMessage(&msg);
  }
}

asIScriptModule* ezAngelScriptDocumentContext::CompileModule(ezStringBuilder& out_sCode, ezSet<ezString>* out_pDependencies)
{
  ezStringBuilder sCode, sInputFile;

  if (m_sInputFile.StartsWith(":inline:"))
  {
    sCode = m_sCode;
    sInputFile = m_sInputFile;
    sInputFile.TrimWordStart(":inline:");
  }
  else
  {
    if (m_sInputFile.IsEmpty())
    {
      ezLog::Error("No AngelScript file specified.");
      return nullptr;
    }

    ezFileReader file;
    if (file.Open(m_sInputFile).Failed())
    {
      ezLog::Error("Failed to open script file '{}'.", m_sInputFile);
      return nullptr;
    }

    sCode.ReadAll(file);

    sInputFile = m_sInputFile;
  }

  if (sCode.IsEmpty())
  {
    ezLog::Error("Script code is empty.");
    return nullptr;
  }

  ezStringBuilder sTempName;
  sTempName.SetFormat("asTempModule-{}", s_iCompileCounter.Increment());

  auto pAs = ezAngelScriptEngineSingleton::GetSingleton();
  auto pModule = pAs->CompileModule(sTempName, m_sClass, sInputFile, sCode, &out_sCode, out_pDependencies);

  if (pModule == nullptr)
    return nullptr;

  if (pAs->ValidateModule(pModule).Failed())
  {
    pModule->Discard();
    return nullptr;
  }

  return pModule;
}

static void WriteSet(ezStringView sFile, const ezSet<ezString>& set)
{
  ezFileWriter writer;
  if (writer.Open(sFile).Failed())
    return;

  const char* szLineBreak = "\n";

  for (const ezString& sItem : set)
  {
    if (sItem.IsEmpty())
      continue;

    writer.WriteBytes(sItem.GetData(), sItem.GetElementCount()).AssertSuccess();
    writer.WriteBytes(szLineBreak, 1).AssertSuccess();
  }
}

void ezAngelScriptDocumentContext::RetrieveScriptInfos(ezStringView sBasePath)
{
  auto pEngine = ezAngelScriptEngineSingleton::GetSingleton()->GetEngine();

  {
    ezAsInfos infos;
    ezAngelScriptUtils::RetrieveAsInfos(pEngine, infos);

    ezStringBuilder sFullPath;

    sFullPath.SetPath(sBasePath, "Types.txt");
    WriteSet(sFullPath, infos.m_Types);

    sFullPath.SetPath(sBasePath, "Namespaces.txt");
    WriteSet(sFullPath, infos.m_Namespaces);

    sFullPath.SetPath(sBasePath, "GlobalFunctions.txt");
    WriteSet(sFullPath, infos.m_GlobalFunctions);

    sFullPath.SetPath(sBasePath, "Methods.txt");
    WriteSet(sFullPath, infos.m_Methods);

    sFullPath.SetPath(sBasePath, "Properties.txt");
    WriteSet(sFullPath, infos.m_Properties);

    sFullPath.SetPath(sBasePath, "Enums.txt");
    WriteSet(sFullPath, infos.m_EnumValues);

    sFullPath.SetPath(sBasePath, "AllDeclarations.txt");
    WriteSet(sFullPath, infos.m_AllDeclarations);

    sFullPath.SetPath(sBasePath, "NotRegisteredDecls.txt");
    WriteSet(sFullPath, ezAngelScriptEngineSingleton::GetSingleton()->GetNotRegistered());
  }

  {
    ezStringBuilder sPredef;
    ezAngelScriptUtils::GenerateAsPredefinedFile(pEngine, sPredef);

    ezStringBuilder sFullPath;
    sFullPath.SetPath(sBasePath, "../../as.predefined");
    ezFileWriter file;
    if (file.Open(sFullPath).Succeeded())
    {
      file.WriteBytes(sPredef.GetData(), sPredef.GetElementCount()).AssertSuccess();
    }
  }
}
