#include <VisualScriptPlugin/VisualScriptPluginPCH.h>

#include <Foundation/Configuration/Startup.h>
#include <Foundation/IO/ChunkStream.h>
#include <Foundation/IO/StringDeduplicationContext.h>
#include <Foundation/Utilities/AssetFileHeader.h>
#include <VisualScriptPlugin/Resources/VisualScriptClassResource.h>
#include <VisualScriptPlugin/Runtime/VisualScriptCoroutine.h>
#include <VisualScriptPlugin/Runtime/VisualScriptFunctionProperty.h>
#include <VisualScriptPlugin/Runtime/VisualScriptInstance.h>

// clang-format off
EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezVisualScriptClassResource, 1, ezRTTIDefaultAllocator<ezVisualScriptClassResource>)
EZ_END_DYNAMIC_REFLECTED_TYPE;
EZ_RESOURCE_IMPLEMENT_COMMON_CODE(ezVisualScriptClassResource);

EZ_BEGIN_SUBSYSTEM_DECLARATION(VisualScript, VisualScriptResource)

  BEGIN_SUBSYSTEM_DEPENDENCIES
    "ResourceManager" 
  END_SUBSYSTEM_DEPENDENCIES

  ON_CORESYSTEMS_STARTUP 
  {
    ezResourceManager::RegisterResourceForAssetType("VisualScriptClass", ezGetStaticRTTI<ezVisualScriptClassResource>());
    ezResourceManager::RegisterResourceOverrideType(ezGetStaticRTTI<ezVisualScriptClassResource>(), [](const ezStringBuilder& sResourceID) -> bool  {
        return sResourceID.HasExtension(".ezBinVisualScriptClass");
      });
  }

  ON_CORESYSTEMS_SHUTDOWN
  {
    ezResourceManager::UnregisterResourceOverrideType(ezGetStaticRTTI<ezVisualScriptClassResource>());
  }

EZ_END_SUBSYSTEM_DECLARATION;
// clang-format on

ezVisualScriptClassResource::ezVisualScriptClassResource() = default;
ezVisualScriptClassResource::~ezVisualScriptClassResource() = default;

ezResourceLoadDesc ezVisualScriptClassResource::UnloadData(Unload WhatToUnload)
{
  DeleteScriptType();
  DeleteAllScriptCoroutineTypes();

  ezResourceLoadDesc ld;
  ld.m_State = ezResourceState::Unloaded;
  ld.m_uiQualityLevelsDiscardable = 0;
  ld.m_uiQualityLevelsLoadable = 0;

  return ld;
}

ezResourceLoadDesc ezVisualScriptClassResource::UpdateContent(ezStreamReader* pStream)
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

  ezString sScriptClassName;
  const ezRTTI* pBaseClassType = nullptr;
  ezScriptRTTI::FunctionList functions;
  ezScriptRTTI::MessageHandlerList messageHandlers;
  {
    ezStringDeduplicationReadContext stringDedup(*pStream);

    ezChunkStreamReader chunk(*pStream);
    chunk.SetEndChunkFileMode(ezChunkStreamReader::EndChunkFileMode::JustClose);

    chunk.BeginStream();

    // skip all chunks that we don't know
    while (chunk.GetCurrentChunk().m_bValid)
    {
      if (chunk.GetCurrentChunk().m_sChunkName == "Header")
      {
        ezString sBaseClassName;
        chunk >> sBaseClassName;
        chunk >> sScriptClassName;
        pBaseClassType = ezRTTI::FindTypeByName(sBaseClassName);
        if (pBaseClassType == nullptr)
        {
          ezLog::Error("Invalid base class '{}' for Visual Script Class '{}'", sBaseClassName, sScriptClassName);
          return ld;
        }
      }
      else if (chunk.GetCurrentChunk().m_sChunkName == "ConstantData")
      {
        ezSharedPtr<ezVisualScriptDataDescription> pConstantDataDesc = EZ_SCRIPT_NEW(ezVisualScriptDataDescription);
        if (pConstantDataDesc->Deserialize(chunk).Failed())
        {
          return ld;
        }

        ezSharedPtr<ezVisualScriptDataStorage> pConstantDataStorage = EZ_SCRIPT_NEW(ezVisualScriptDataStorage, pConstantDataDesc);
        if (pConstantDataStorage->Deserialize(chunk, ezScriptAllocator::GetAllocator()).Succeeded())
        {
          m_pConstantDataStorage = pConstantDataStorage;
        }
      }
      else if (chunk.GetCurrentChunk().m_sChunkName == "InstanceData")
      {
        ezSharedPtr<ezVisualScriptDataDescription> pInstanceDataDesc = EZ_SCRIPT_NEW(ezVisualScriptDataDescription);
        if (pInstanceDataDesc->Deserialize(chunk).Succeeded())
        {
          m_pInstanceDataDesc = pInstanceDataDesc;
        }

        ezSharedPtr<ezVisualScriptInstanceDataMapping> pInstanceDataMapping = EZ_SCRIPT_NEW(ezVisualScriptInstanceDataMapping);
        if (chunk.ReadHashTable(pInstanceDataMapping->m_Content).Succeeded())
        {
          m_pInstanceDataMapping = pInstanceDataMapping;

          // calculate byte offsets from indices
          for (auto& it : m_pInstanceDataMapping->m_Content)
          {
            auto& dataOffset = it.Value().m_DataOffset;
            dataOffset = m_pInstanceDataDesc->GetOffset(dataOffset.GetType(), dataOffset.m_uiByteOffset, dataOffset.GetSource());
          }
        }
      }
      else if (chunk.GetCurrentChunk().m_sChunkName == "FunctionGraphs")
      {
        ezUInt32 uiNumFunctions;
        chunk >> uiNumFunctions;

        if (m_pInstanceDataDesc == nullptr || m_pConstantDataStorage == nullptr)
        {
          ezLog::Error("Old visual script, needs re-export");
          return ld;
        }

        for (ezUInt32 i = 0; i < uiNumFunctions; ++i)
        {
          ezString sFunctionName;
          ezEnum<ezVisualScriptNodeDescription::Type> functionType;
          ezEnum<ezScriptCoroutineCreationMode> coroutineCreationMode;
          chunk >> sFunctionName;
          chunk >> functionType;
          chunk >> coroutineCreationMode;

          ezUniquePtr<ezVisualScriptGraphDescription> pDesc = EZ_SCRIPT_NEW(ezVisualScriptGraphDescription);
          if (pDesc->Deserialize(chunk, *m_pInstanceDataDesc, m_pConstantDataStorage->GetDesc()).Failed())
          {
            ezLog::Error("Invalid visual script desc");
            return ld;
          }

          if (functionType == ezVisualScriptNodeDescription::Type::EntryCall)
          {
            ezUniquePtr<ezVisualScriptFunctionProperty> pFunctionProperty = EZ_SCRIPT_NEW(ezVisualScriptFunctionProperty, sFunctionName, std::move(pDesc));
            functions.PushBack(std::move(pFunctionProperty));
          }
          else if (functionType == ezVisualScriptNodeDescription::Type::EntryCall_Coroutine)
          {
            ezUniquePtr<ezVisualScriptCoroutineAllocator> pCoroutineAllocator = EZ_SCRIPT_NEW(ezVisualScriptCoroutineAllocator, std::move(pDesc));
            auto pCoroutineType = CreateScriptCoroutineType(sScriptClassName, sFunctionName, std::move(pCoroutineAllocator));
            ezUniquePtr<ezScriptCoroutineFunctionProperty> pFunctionProperty = EZ_SCRIPT_NEW(ezScriptCoroutineFunctionProperty, sFunctionName, pCoroutineType, coroutineCreationMode);
            functions.PushBack(std::move(pFunctionProperty));
          }
          else if (functionType == ezVisualScriptNodeDescription::Type::MessageHandler)
          {
            auto desc = pDesc->GetMessageDesc();
            ezUniquePtr<ezVisualScriptMessageHandler> pMessageHandler = EZ_SCRIPT_NEW(ezVisualScriptMessageHandler, desc, std::move(pDesc));
            messageHandlers.PushBack(std::move(pMessageHandler));
          }
          else if (functionType == ezVisualScriptNodeDescription::Type::MessageHandler_Coroutine)
          {
            auto desc = pDesc->GetMessageDesc();
            ezUniquePtr<ezVisualScriptCoroutineAllocator> pCoroutineAllocator = EZ_SCRIPT_NEW(ezVisualScriptCoroutineAllocator, std::move(pDesc));
            auto pCoroutineType = CreateScriptCoroutineType(sScriptClassName, sFunctionName, std::move(pCoroutineAllocator));
            ezUniquePtr<ezScriptCoroutineMessageHandler> pMessageHandler = EZ_SCRIPT_NEW(ezScriptCoroutineMessageHandler, sFunctionName, desc, pCoroutineType, coroutineCreationMode);
            messageHandlers.PushBack(std::move(pMessageHandler));
          }
          else
          {
            ezLog::Error("Invalid event handler type '{}' for event handler '{}'", ezVisualScriptNodeDescription::Type::GetName(functionType), sFunctionName);
            return ld;
          }
        }
      }

      chunk.NextChunk();
    }
  }

  CreateScriptType(sScriptClassName, pBaseClassType, std::move(functions), std::move(messageHandlers));

  ld.m_State = ezResourceState::Loaded;
  return ld;
}

void ezVisualScriptClassResource::UpdateMemoryUsage(MemoryUsage& out_NewMemoryUsage)
{
  out_NewMemoryUsage.m_uiMemoryCPU = (ezUInt32)sizeof(ezVisualScriptClassResource);
  out_NewMemoryUsage.m_uiMemoryGPU = 0;
}

ezUniquePtr<ezScriptInstance> ezVisualScriptClassResource::Instantiate(ezReflectedClass& inout_owner, ezWorld* pWorld) const
{
  return EZ_SCRIPT_NEW(ezVisualScriptInstance, inout_owner, pWorld, m_pConstantDataStorage, m_pInstanceDataDesc, m_pInstanceDataMapping);
}


EZ_STATICLINK_FILE(VisualScriptPlugin, VisualScriptPlugin_Resources_VisualScriptClassResource);
