#include <VisualScriptPlugin/VisualScriptPluginPCH.h>

#include <Core/Assets/AssetFileHeader.h>
#include <Foundation/Configuration/Startup.h>
#include <Foundation/IO/ChunkStream.h>
#include <Foundation/IO/StringDeduplicationContext.h>
#include <VisualScriptPlugin/Resources/VisualScriptClassResource.h>
#include <VisualScriptPlugin/Runtime/VisualScriptInstance.h>

// clang-format off
EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezVisualScriptClassResource, 1, ezRTTIDefaultAllocator<ezVisualScriptClassResource>)
EZ_END_DYNAMIC_REFLECTED_TYPE;
EZ_RESOURCE_IMPLEMENT_COMMON_CODE(ezVisualScriptClassResource);

EZ_BEGIN_SUBSYSTEM_DECLARATION(TypeScript, Resource)

  BEGIN_SUBSYSTEM_DEPENDENCIES
    "ResourceManager" 
  END_SUBSYSTEM_DEPENDENCIES

  ON_CORESYSTEMS_STARTUP 
  {
    ezResourceManager::RegisterResourceForAssetType("VisualScriptClass", ezGetStaticRTTI<ezVisualScriptClassResource>());
    ezResourceManager::RegisterResourceOverrideType(ezGetStaticRTTI<ezVisualScriptClassResource>(), [](const ezStringBuilder& sResourceID) -> bool  {
        return sResourceID.HasExtension(".ezVisualScriptClassBin");
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

  // skip the absolute file path data that the standard file reader writes into the stream
  {
    ezString sAbsFilePath;
    (*pStream) >> sAbsFilePath;
  }

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
      else if (chunk.GetCurrentChunk().m_sChunkName == "FunctionGraphs")
      {
        ezUInt32 uiNumFunctions;
        chunk >> uiNumFunctions;

        for (ezUInt32 i = 0; i < uiNumFunctions; ++i)
        {
          ezString sFunctionName;
          ezEnum<ezVisualScriptNodeDescription::Type> functionType;
          chunk >> sFunctionName;
          chunk >> functionType;

          ezUniquePtr<ezVisualScriptGraphDescription> pDesc = EZ_DEFAULT_NEW(ezVisualScriptGraphDescription);
          if (pDesc->Deserialize(chunk).Failed())
          {
            ezLog::Error("Invalid visual script desc");
            return ld;
          }

          if (functionType == ezVisualScriptNodeDescription::Type::EntryCall)
          {
            ezUniquePtr<ezVisualScriptFunctionProperty> pFunctionProperty = EZ_DEFAULT_NEW(ezVisualScriptFunctionProperty, sFunctionName, std::move(pDesc));
            functions.PushBack(std::move(pFunctionProperty));
          }
          else if (functionType == ezVisualScriptNodeDescription::Type::MessageHandler)
          {
          }
          else
          {
            ezLog::Error("Invalid event handler type {} for event handler '{}'", functionType, sFunctionName);
            return ld;
          }
        }
      }
      else if (chunk.GetCurrentChunk().m_sChunkName == "ConstantData")
      {
        ezSharedPtr<ezVisualScriptDataDescription> pConstantDataDesc = EZ_DEFAULT_NEW(ezVisualScriptDataDescription);
        if (pConstantDataDesc->Deserialize(chunk).Failed())
        {
          return ld;
        }

        ezSharedPtr<ezVisualScriptDataStorage> pConstantDataStorage = EZ_DEFAULT_NEW(ezVisualScriptDataStorage, pConstantDataDesc);
        if (pConstantDataStorage->Deserialize(chunk).Succeeded())
        {
          m_pConstantDataStorage = pConstantDataStorage;
        }
      }
      else if (chunk.GetCurrentChunk().m_sChunkName == "VariableDataDesc")
      {
        ezSharedPtr<ezVisualScriptDataDescription> pVariableDataDesc = EZ_DEFAULT_NEW(ezVisualScriptDataDescription);
        if (pVariableDataDesc->Deserialize(chunk).Succeeded())
        {
          m_pVariableDataDesc = pVariableDataDesc;
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

ezUniquePtr<ezScriptInstance> ezVisualScriptClassResource::Instantiate(ezReflectedClass& owner, ezWorld* pWorld) const
{
  return EZ_DEFAULT_NEW(ezVisualScriptInstance, owner, pWorld, m_pConstantDataStorage, m_pVariableDataDesc);
}
