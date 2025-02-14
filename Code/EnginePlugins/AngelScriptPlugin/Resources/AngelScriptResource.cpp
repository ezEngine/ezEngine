#include <AngelScriptPlugin/AngelScriptPluginPCH.h>

#include <AngelScript/include/angelscript.h>
#include <AngelScriptPlugin/Resources/AngelScriptResource.h>
#include <AngelScriptPlugin/Runtime/AsEngineSingleton.h>
#include <AngelScriptPlugin/Runtime/AsFunctionDispatch.h>
#include <AngelScriptPlugin/Runtime/AsInstance.h>
#include <AngelScriptPlugin/Utils/AngelScriptUtils.h>
#include <Core/Scripting/ScriptAttributes.h>
#include <Core/World/Component.h>
#include <Foundation/Communication/Message.h>
#include <Foundation/Configuration/Startup.h>
#include <Foundation/IO/ChunkStream.h>
#include <Foundation/IO/CompressedStreamZstd.h>
#include <Foundation/IO/StringDeduplicationContext.h>
#include <Foundation/Utilities/AssetFileHeader.h>

// clang-format off
EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezAngelScriptResource, 1, ezRTTIDefaultAllocator<ezAngelScriptResource>)
EZ_END_DYNAMIC_REFLECTED_TYPE;
EZ_RESOURCE_IMPLEMENT_COMMON_CODE(ezAngelScriptResource);

EZ_BEGIN_SUBSYSTEM_DECLARATION(AngelScript, AngelScriptResource)

  BEGIN_SUBSYSTEM_DEPENDENCIES
    "ResourceManager" 
  END_SUBSYSTEM_DEPENDENCIES

  ON_CORESYSTEMS_STARTUP 
  {
    ezResourceManager::RegisterResourceForAssetType("AngelScriptClass", ezGetStaticRTTI<ezAngelScriptResource>());
    ezResourceManager::RegisterResourceOverrideType(ezGetStaticRTTI<ezAngelScriptResource>(), [](const ezStringBuilder& sResourceID) -> bool  {
        return sResourceID.HasExtension(".ezBinAngelScript");
      });
  }

  ON_CORESYSTEMS_SHUTDOWN
  {
    ezResourceManager::UnregisterResourceOverrideType(ezGetStaticRTTI<ezAngelScriptResource>());
  }

EZ_END_SUBSYSTEM_DECLARATION;
// clang-format on

ezAngelScriptResource::ezAngelScriptResource() = default;
ezAngelScriptResource::~ezAngelScriptResource() = default;

ezResourceLoadDesc ezAngelScriptResource::UnloadData(Unload WhatToUnload)
{
  DeleteScriptType();
  DeleteAllScriptCoroutineTypes();

  if (m_pModule)
  {
    // can't do this here, because other worlds may still use the current state
    // need to track somehow where modules are still in use
    // m_pModule->Discard();
    m_pModule = nullptr;
  }

  ezResourceLoadDesc ld;
  ld.m_State = ezResourceState::Unloaded;
  ld.m_uiQualityLevelsDiscardable = 0;
  ld.m_uiQualityLevelsLoadable = 0;

  return ld;
}

ezResourceLoadDesc ezAngelScriptResource::UpdateContent(ezStreamReader* pStream)
{
  ezResourceLoadDesc ld;
  ld.m_uiQualityLevelsDiscardable = 0;
  ld.m_uiQualityLevelsLoadable = 0;
  ld.m_State = ezResourceState::LoadedResourceMissing;

  if (pStream == nullptr)
  {
    return ld;
  }

  // the standard file reader writes the absolute file path into the stream
  ezString sAbsFilePath;
  (*pStream) >> sAbsFilePath;

  // skip the asset file header at the start of the file
  ezAssetFileHeader AssetHash;
  AssetHash.Read(*pStream).IgnoreResult();

  ezUInt8 uiVersion = 1;
  (*pStream) >> uiVersion;

  ezUInt8 uiCompressionMode = 0;

  if (uiVersion >= 4)
  {
    (*pStream) >> uiCompressionMode;
  }

  ezStreamReader* pDeCompressor = pStream;

#ifdef BUILDSYSTEM_ENABLE_ZSTD_SUPPORT
  ezCompressedStreamReaderZstd decompressorZstd;
#endif

  switch (uiCompressionMode)
  {
    case 0:
      break;

    case 1:
#ifdef BUILDSYSTEM_ENABLE_ZSTD_SUPPORT
      decompressorZstd.SetInputStream(pStream);
      pDeCompressor = &decompressorZstd;
      break;
#else
      ezLog::Error("AngelScript is compressed with zstandard, but support for this compressor is not compiled in.");
      ld.m_State = ezResourceState::LoadedResourceMissing;
      return res;
#endif

    default:
      ezLog::Error("AngelScript is compressed with an unknown algorithm.");
      ld.m_State = ezResourceState::LoadedResourceMissing;
      return ld;
  }

  ezStreamReader& stream = *pDeCompressor;
  stream >> m_sClassName;

  ezStringBuilder sModuleID;
  sModuleID.SetFormat("{}-{}", GetResourceID(), GetCurrentResourceChangeCounter());

  auto pAs = ezAngelScriptEngineSingleton::GetSingleton();

  if (uiVersion == 1)
  {
    stream >> m_sScriptContent;
    m_pModule = pAs->CompileModule(sModuleID, m_sClassName, "main", m_sScriptContent, nullptr, nullptr);
  }
  else
  {
    ezHybridArray<ezUInt8, 1024 * 8> bytecode;
    stream.ReadArray(bytecode).AssertSuccess();

    if (uiVersion >= 3)
    {
      stream >> m_sScriptContent;
      m_pModule = pAs->CompileModule(sModuleID, m_sClassName, "main", m_sScriptContent, nullptr, nullptr);
    }
    else
    {
      m_pModule = ezAngelScriptUtils::LoadFromByteCode(pAs->GetEngine(), sModuleID, bytecode);
    }
  }

  if (m_pModule == nullptr)
  {
    return ld;
  }

  const asITypeInfo* pClassType = m_pModule->GetTypeInfoByDecl(m_sClassName);

  ezScriptRTTI::FunctionList functions;
  ezScriptRTTI::MessageHandlerList messageHandlers;

  const ezRTTI* pBaseType = ezGetStaticRTTI<ezComponent>();

  ezStringBuilder sFunctionName;

  ezHybridArray<ezString, 16> funcNames;

  for (auto pCompFunc : pBaseType->GetFunctions())
  {
    const ezScriptBaseClassFunctionAttribute* pAttr = pCompFunc->GetAttributeByType<ezScriptBaseClassFunctionAttribute>();
    if (pAttr == nullptr)
      continue;

    sFunctionName = pCompFunc->GetPropertyName();
    sFunctionName.TrimWordStart("Reflection_");

    funcNames.PushBack(sFunctionName);

    if (auto pFunc = pClassType->GetMethodByName(sFunctionName))
    {
      ezUniquePtr<ezAngelScriptFunctionProperty> pFunctionProperty = EZ_SCRIPT_NEW(ezAngelScriptFunctionProperty, sFunctionName, pFunc);
      functions.PushBack(std::move(pFunctionProperty));
    }
  }

  FindMessageHandlers(pClassType, messageHandlers);

  if (functions.IsEmpty() && messageHandlers.IsEmpty())
  {
    ezLog::Error("AngelScript code doesn't contain any callable function or message handlers.");
    ezLog::Info("Candidates are:");

    for (const auto& s : funcNames)
    {
      ezLog::Info("  {}", s);
    }

    return ld;
  }

  CreateScriptType(GetResourceID(), pBaseType, std::move(functions), std::move(messageHandlers));

  ld.m_State = ezResourceState::Loaded;
  return ld;
}

void ezAngelScriptResource::UpdateMemoryUsage(MemoryUsage& out_NewMemoryUsage)
{
  out_NewMemoryUsage.m_uiMemoryCPU = (ezUInt32)sizeof(ezAngelScriptResource);
  out_NewMemoryUsage.m_uiMemoryGPU = 0;
}

ezUniquePtr<ezScriptInstance> ezAngelScriptResource::Instantiate(ezReflectedClass& inout_owner, ezWorld* pWorld) const
{
  return EZ_SCRIPT_NEW(ezAngelScriptInstance, inout_owner, pWorld, m_pModule, m_sClassName);
}

void ezAngelScriptResource::FindMessageHandlers(const asITypeInfo* pClassType, ezScriptRTTI::MessageHandlerList& inout_Handlers)
{
  auto pAs = ezAngelScriptEngineSingleton::GetSingleton();

  ezStringBuilder sArgType;

  ezUniquePtr<ezAngelScriptCustomAsMessageHandler> pAsMsgHandler;

  for (ezUInt32 i = 0; i < pClassType->GetMethodCount(); ++i)
  {
    asIScriptFunction* pFunc = pClassType->GetMethodByIndex(i, false);

    if (!ezStringUtils::StartsWith(pFunc->GetName(), "OnMsg"))
      continue;

    // only allow void functions
    if (pFunc->GetReturnTypeId() != asTYPEID_VOID)
    {
      ezLog::Error("Malformed message handler '{}': Return type must be 'void'.", pFunc->GetName());
      continue;
    }

    // that take exactly one argument
    if (pFunc->GetParamCount() != 1)
    {
      ezLog::Error("Malformed message handler '{}': Must take exactly one argument.", pFunc->GetName());
      continue;
    }

    int iArgTypeId;
    pFunc->GetParam(0, &iArgTypeId);

    if (iArgTypeId & asTYPEID_APPOBJECT)
    {
      const ezRTTI* pArgType = ezAngelScriptUtils::MapToRTTI(iArgTypeId, pAs->GetEngine());

      if (pArgType == nullptr)
      {
        ezLog::Error("Malformed message handler '{}': Argument has unknown type.", pFunc->GetName());
        continue;
      }

      // has to be a type derived from ezMessage
      if (!pArgType->IsDerivedFrom<ezMessage>())
      {
        ezLog::Error("Malformed message handler '{}': Argument type has to derive from ezMessage or ezAngelScriptMessage.", pFunc->GetName());
        continue;
      }

      ezScriptMessageDesc desc;
      desc.m_pType = pArgType;

      ezUniquePtr<ezAngelScriptMessageHandler> pFunctionProperty = EZ_SCRIPT_NEW(ezAngelScriptMessageHandler, desc, pFunc);
      inout_Handlers.PushBack(std::move(pFunctionProperty));
    }

    if (iArgTypeId & asTYPEID_SCRIPTOBJECT)
    {
      if (auto pArgType = pAs->GetEngine()->GetTypeInfoById(iArgTypeId))
      {
        if (pArgType->GetInterfaceCount() != 1 || !ezStringUtils::IsEqual(pArgType->GetInterface(0)->GetName(), "ezAngelScriptMessage"))
        {
          ezLog::Error("Malformed message handler '{}': Argument type has to derive from ezMessage or ezAngelScriptMessage.", pFunc->GetName());
          continue;
        }

        if (!pAsMsgHandler)
        {
          ezScriptMessageDesc desc;
          desc.m_pType = ezGetStaticRTTI<ezMsgDeliverAngelScriptMsg>();

          pAsMsgHandler = EZ_SCRIPT_NEW(ezAngelScriptCustomAsMessageHandler, desc);
        }

        pAsMsgHandler->AddReceiver(pFunc, pArgType->GetName());
      }
    }
  }

  if (pAsMsgHandler)
  {
    inout_Handlers.PushBack(std::move(pAsMsgHandler));
  }
}
