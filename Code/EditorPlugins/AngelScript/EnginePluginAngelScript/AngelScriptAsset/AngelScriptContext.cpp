#include <EnginePluginAngelScript/EnginePluginAngelScriptPCH.h>

#include <AngelScript/include/angelscript.h>
#include <AngelScriptPlugin/Runtime/AngelScriptEngineSingleton.h>
#include <AngelScriptPlugin/Runtime/AngelScriptInstance.h>
#include <AngelScriptPlugin/Utils/AngelScriptUtils.h>
#include <EnginePluginAngelScript/AngelScriptAsset/AngelScriptContext.h>
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

  asIScriptModule* pModule = CompileModule();

  if (pModule == nullptr)
  {
    return ezStatus(logBuffer.m_sBuffer.GetView());
  }

  ezHybridArray<ezUInt8, 1024 * 8> bytecode;
  ezAngelScriptEngineSingleton::SaveByteCode(pModule, bytecode);
  pModule->Discard();

  ezDeferredFileWriter out;
  out.SetOutput(pMsg->m_sOutputFile);

  {
    // File Header
    ezAssetFileHeader header;
    header.SetFileHashAndVersion(pMsg->m_uiAssetHash, pMsg->m_uiVersion);
    header.Write(out).AssertSuccess();

    ezUInt8 uiVersion = 2;
    out << uiVersion;
  }

  out << m_sClass;
  out.WriteArray(bytecode).AssertSuccess();

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

  asIScriptModule* pModule = CompileModule();
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

  for (ezUInt32 i2 = 0; i2 < pClassType->GetPropertyCount(); ++i2)
  {
    const char* szName;
    int typeId;

    bool isPrivate = false, isProtected = false, isReference = false;
    pClassType->GetProperty(i2, &szName, &typeId, &isPrivate, &isProtected, nullptr, &isReference);
    ezStringBuilder sDecl = pClassType->GetPropertyDeclaration(i2);

    if (isPrivate || isProtected)
      continue;

    ezVariant defVal;
    if (ezAngelScriptUtils::ReadAsProperty(typeId, pInstance->GetAddressOfProperty(i2), pModule->GetEngine(), defVal).Succeeded())
    {
      ezSimpleDocumentConfigMsgToEditor msg;
      msg.m_DocumentGuid = m_DocumentGuid;
      msg.m_sWhatToDo = "SyncExposedParams_Add";
      msg.m_sPayload = szName;
      msg.m_PayloadValue = defVal;
      SendProcessMessage(&msg);
    }
  }

  {
    ezSimpleDocumentConfigMsgToEditor msg;
    msg.m_DocumentGuid = m_DocumentGuid;
    msg.m_sWhatToDo = "SyncExposedParams_Finish";
    SendProcessMessage(&msg);
  }
}

asIScriptModule* ezAngelScriptDocumentContext::CompileModule()
{
  ezStringBuilder sCode;

  if (m_sInputFile == ":inline:")
  {
    sCode = m_sCode;
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
  }

  if (sCode.IsEmpty())
  {
    ezLog::Error("Script code is empty.");
    return nullptr;
  }

  ezStringBuilder sTempName;
  sTempName.SetFormat("asTempModule-{}", s_iCompileCounter.Increment());

  auto pAs = ezAngelScriptEngineSingleton::GetSingleton();
  auto pModule = pAs->CompileModule(sTempName, m_sClass, m_sInputFile, sCode);

  if (pModule == nullptr)
    return nullptr;

  if (pAs->ValidateModule(pModule).Failed())
  {
    pModule->Discard();
    return nullptr;
  }

  return pModule;
}

static void WriteSet(ezStringView file, const ezSet<ezString>& set)
{
  ezFileWriter writer;
  if (writer.Open(file).Failed())
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

static ezString GetNiceFuncDecl(const asIScriptFunction* pFunc)
{
  ezStringBuilder tmp;

  tmp = pFunc->GetDeclaration(false, false, true);

  tmp.ReplaceAll(" :: ", "::");
  tmp.ReplaceAll(" (", "(");
  tmp.ReplaceAll("( ", "(");
  tmp.ReplaceAll(" )", ")");
  tmp.ReplaceAll(") ", ")");
  tmp.ReplaceAll(" ,", ",");
  tmp.ReplaceAll(")const", ") const");

  tmp.Append(";");

  return tmp;
}

void ezAngelScriptDocumentContext::RetrieveScriptInfos(ezStringView sBasePath)
{
  ezSet<ezString> typeNames;
  ezSet<ezString> namespaceNames;
  ezSet<ezString> globalFunctionNames;
  ezSet<ezString> methodNames;
  ezSet<ezString> allDecls;
  ezSet<ezString> properties;
  ezSet<ezString> enums;

  ezStringView sIndent;

  ezStringBuilder sPredef;
  sPredef.Reserve(1024 * 32);

  auto pAS = ezAngelScriptEngineSingleton::GetSingleton();
  asIScriptEngine* pEngine = pAS->GetEngine();

  ezStringBuilder tmp;

  {
    sPredef.Append("// *** TYPEDEFS *** \n\n");

    for (ezUInt32 idx = 0; idx < pEngine->GetTypedefCount(); ++idx)
    {
      const asITypeInfo* pType = pEngine->GetTypedefByIndex(idx);

      switch (pType->GetTypedefTypeId())
      {
        case asTYPEID_BOOL:
          tmp.Set("bool");
          break;
        case asTYPEID_FLOAT:
          tmp.Set("float");
          break;
        case asTYPEID_DOUBLE:
          tmp.Set("double");
          break;
        case asTYPEID_INT8:
          tmp.Set("int8");
          break;
        case asTYPEID_INT16:
          tmp.Set("int16");
          break;
        case asTYPEID_INT32:
          tmp.Set("int32");
          break;
        case asTYPEID_INT64:
          tmp.Set("int64");
          break;
        case asTYPEID_UINT8:
          tmp.Set("uint8");
          break;
        case asTYPEID_UINT16:
          tmp.Set("uint16");
          break;
        case asTYPEID_UINT32:
          tmp.Set("uint32");
          break;
        case asTYPEID_UINT64:
          tmp.Set("uint64");
          break;

          EZ_DEFAULT_CASE_NOT_IMPLEMENTED;
      }

      sPredef.Append("typedef ", tmp, " ", pType->GetName(), ";\n");
    }
  }

  {
    sPredef.Append("\n// *** ENUMS *** \n\n");

    for (ezUInt32 idx = 0; idx < pEngine->GetEnumCount(); ++idx)
    {
      const asITypeInfo* pType = pEngine->GetEnumByIndex(idx);
      typeNames.Insert(pType->GetName());

      sPredef.Append("enum ", pType->GetName(), "\n{\n");

      for (ezUInt32 valIdx = 0; valIdx < pType->GetEnumValueCount(); ++valIdx)
      {
        int value;
        const char* szString = pType->GetEnumValueByIndex(valIdx, &value);

        enums.Insert(szString);
        tmp.Set(pType->GetName(), "::", szString);
        allDecls.Insert(tmp);

        sPredef.AppendFormat("  {} = {},\n", szString, value);
      }

      sPredef.Append("}\n\n");
    }
  }

  {
    sPredef.Append("\n// *** TYPES *** \n\n");

    for (ezUInt32 typeIdx = 0; typeIdx < pEngine->GetObjectTypeCount(); ++typeIdx)
    {
      const asITypeInfo* pType = pEngine->GetObjectTypeByIndex(typeIdx);
      const ezRTTI* pRtti = ezAngelScriptUtils::MapToRTTI(pType->GetTypeId(), pEngine);

      typeNames.Insert(pType->GetName());
      namespaceNames.Insert(pType->GetNamespace());

      if (ezStringUtils::FindSubString(pType->GetName(), "String") != nullptr)
      {
        sPredef.Append("[BuiltinString]\n");
      }

      sPredef.Append("class ", pType->GetName());

      if (pRtti && pRtti->GetParentType() && pRtti->GetParentType() != ezGetStaticRTTI<ezReflectedClass>())
      {
        sPredef.Append(" : ", pRtti->GetParentType()->GetTypeName());
      }

      sPredef.Append("\n{\n");

      for (ezUInt32 methodIdx = 0; methodIdx < pType->GetBehaviourCount(); ++methodIdx)
      {
        asEBehaviours behavior;
        const asIScriptFunction* pFunc = pType->GetBehaviourByIndex(methodIdx, &behavior);

        if (pFunc->IsPrivate())
          continue;

        if (behavior != asEBehaviours::asBEHAVE_CONSTRUCT)
          continue;

        tmp = GetNiceFuncDecl(pFunc);
        sPredef.Append("  ", tmp, "\n");
      }

      for (ezUInt32 methodIdx = 0; methodIdx < pType->GetMethodCount(); ++methodIdx)
      {
        const asIScriptFunction* pFunc = pType->GetMethodByIndex(methodIdx, false);

        if (pFunc->IsPrivate())
          continue;

        const intptr_t flags = reinterpret_cast<const intptr_t>(pFunc->GetUserData(ezAsUserData::FuncFlags));

        if ((flags & 0x01) != 0)
          continue;

        if (pFunc->IsProperty())
        {
          tmp = pFunc->GetName();
          tmp.TrimWordStart("set_");

          if (tmp.TrimWordStart("get_"))
          {
            if (const ezRTTI* pRtti = ezAngelScriptUtils::MapToRTTI(pFunc->GetReturnTypeId(), pFunc->GetEngine()))
            {
              tmp.Prepend(pRtti->GetTypeName(), " ");
            }
            else
            {
              tmp.Prepend("unknown-type ");
            }

            properties.Insert(tmp);

            sPredef.Append("  ", tmp, ";\n");
          }

          allDecls.Insert(pFunc->GetDeclaration(true, true, true));
        }
        else
        {
          methodNames.Insert(pFunc->GetName());
          allDecls.Insert(pFunc->GetDeclaration(true, true, true));

          tmp = GetNiceFuncDecl(pFunc);
          sPredef.Append("  ", tmp, "\n");
        }
      }

      sPredef.Append("}\n\n");
    }
  }

  {
    sPredef.Append("\n// *** EXTRA *** \n\n");

    const char* szClassCode = R"(
class ezAngelScriptClass : ezIAngelScriptClass
{
    ezScriptComponent@ GetOwnerComponent();
    ezGameObject@ GetOwner();
    ezWorld@ GetWorld();
    void SetUpdateInterval(ezTime interval);
}
    )";

    sPredef.Append(szClassCode);
  }

  {
    sPredef.Append("\n// *** GLOBAL FUNCTIONS *** \n\n");

    ezStringBuilder sNamespace;

    for (ezUInt32 funcIdx = 0; funcIdx < pEngine->GetGlobalFunctionCount(); ++funcIdx)
    {
      const asIScriptFunction* pFunc = pEngine->GetGlobalFunctionByIndex(funcIdx);

      if (sNamespace != pFunc->GetNamespace())
      {
        if (!sNamespace.IsEmpty())
        {
          sPredef.Append("}\n\n");
        }

        sNamespace = pFunc->GetNamespace();
        sIndent = "";

        if (!sNamespace.IsEmpty())
        {
          sPredef.Append("namespace ", sNamespace, "\n{\n");
          sIndent = "  ";
        }
      }

      globalFunctionNames.Insert(pFunc->GetName());
      tmp = pFunc->GetDeclaration(true, true, true);
      allDecls.Insert(tmp);
      namespaceNames.Insert(pFunc->GetNamespace());

      tmp = GetNiceFuncDecl(pFunc);

      sPredef.Append(sIndent, tmp, "\n");
    }

    if (!sNamespace.IsEmpty())
    {
      sPredef.Append("}\n\n");
    }
  }

  ezStringBuilder sFullPath;

  sFullPath.SetPath(sBasePath, "Types.txt");
  WriteSet(sFullPath, typeNames);

  sFullPath.SetPath(sBasePath, "Namespaces.txt");
  WriteSet(sFullPath, namespaceNames);

  sFullPath.SetPath(sBasePath, "GlobalFunctions.txt");
  WriteSet(sFullPath, globalFunctionNames);

  sFullPath.SetPath(sBasePath, "Methods.txt");
  WriteSet(sFullPath, methodNames);

  sFullPath.SetPath(sBasePath, "Properties.txt");
  WriteSet(sFullPath, properties);

  sFullPath.SetPath(sBasePath, "Enums.txt");
  WriteSet(sFullPath, enums);

  sFullPath.SetPath(sBasePath, "AllDeclarations.txt");
  WriteSet(sFullPath, allDecls);

  sFullPath.SetPath(sBasePath, "NotRegisteredDecls.txt");
  WriteSet(sFullPath, pAS->GetNotRegistered());

  {
    sFullPath.SetPath(sBasePath, "../../as.predefined");
    ezFileWriter file;
    if (file.Open(sFullPath).Succeeded())
    {
      file.WriteBytes(sPredef.GetData(), sPredef.GetElementCount()).AssertSuccess();
    }
  }
}
