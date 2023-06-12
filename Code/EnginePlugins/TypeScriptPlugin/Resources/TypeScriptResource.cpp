#include <TypeScriptPlugin/TypeScriptPluginPCH.h>

#include <Core/Assets/AssetFileHeader.h>
#include <Core/Scripting/DuktapeContext.h>
#include <Foundation/Configuration/Startup.h>
#include <TypeScriptPlugin/Components/TypeScriptComponent.h>
#include <TypeScriptPlugin/Resources/TypeScriptResource.h>

namespace
{
  class TypeScriptFunctionProperty : public ezAbstractFunctionProperty
  {
  public:
    TypeScriptFunctionProperty(const char* szPropertyName)
      : ezAbstractFunctionProperty(szPropertyName)
    {
    }

    virtual ezFunctionType::Enum GetFunctionType() const override { return ezFunctionType::Member; }
    virtual const ezRTTI* GetReturnType() const override { return nullptr; }
    virtual ezBitflags<ezPropertyFlags> GetReturnFlags() const override { return ezPropertyFlags::Void; }
    virtual ezUInt32 GetArgumentCount() const override { return 0; }
    virtual const ezRTTI* GetArgumentType(ezUInt32 uiParamIndex) const override { return nullptr; }
    virtual ezBitflags<ezPropertyFlags> GetArgumentFlags(ezUInt32 uiParamIndex) const override { return ezPropertyFlags::Void; }

    virtual void Execute(void* pInstance, ezArrayPtr<ezVariant> arguments, ezVariant& ref_returnValue) const override
    {
      auto pTypeScriptInstance = static_cast<ezTypeScriptInstance*>(pInstance);
      ezTypeScriptBinding& binding = pTypeScriptInstance->GetBinding();

      ezDuktapeHelper duk(binding.GetDukTapeContext());

      // TODO: this needs to be more generic to work with other things besides components
      binding.DukPutComponentObject(&pTypeScriptInstance->GetComponent()); // [ comp ]

      if (duk.PrepareMethodCall(GetPropertyName()).Succeeded()) // [ comp func comp ]
      {
        duk.CallPreparedMethod().IgnoreResult(); // [ comp result ]
        duk.PopStack(2);                         // [ ]

        EZ_DUK_RETURN_VOID_AND_VERIFY_STACK(duk, 0);
      }
      else
      {
        // remove 'this'   [ comp ]
        duk.PopStack(); // [ ]

        EZ_DUK_RETURN_VOID_AND_VERIFY_STACK(duk, 0);
      }
    }
  };
} // namespace

//////////////////////////////////////////////////////////////////////////

ezTypeScriptInstance::ezTypeScriptInstance(ezComponent& owner, ezWorld* pWorld, ezTypeScriptBinding& binding)
  : ezScriptInstance(owner, pWorld)
  , m_Binding(binding)
{
}

void ezTypeScriptInstance::ApplyParameters(const ezArrayMap<ezHashedString, ezVariant>& parameters)
{
  ezDuktapeHelper duk(m_Binding.GetDukTapeContext());

  m_Binding.DukPutComponentObject(&GetComponent()); // [ comp ]

  for (ezUInt32 p = 0; p < parameters.GetCount(); ++p)
  {
    const auto& pair = parameters.GetPair(p);

    ezTypeScriptBinding::SetVariantProperty(duk, pair.key.GetString(), -1, pair.value); // [ comp ]
  }

  duk.PopStack(); // [ ]

  EZ_DUK_RETURN_VOID_AND_VERIFY_STACK(duk, 0);
}

//////////////////////////////////////////////////////////////////////////

// clang-format off
EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezTypeScriptClassResource, 1, ezRTTIDefaultAllocator<ezTypeScriptClassResource>)
EZ_END_DYNAMIC_REFLECTED_TYPE;
EZ_RESOURCE_IMPLEMENT_COMMON_CODE(ezTypeScriptClassResource);

EZ_BEGIN_SUBSYSTEM_DECLARATION(TypeScript, ClassResource)

  BEGIN_SUBSYSTEM_DEPENDENCIES
    "ResourceManager" 
  END_SUBSYSTEM_DEPENDENCIES

  ON_CORESYSTEMS_STARTUP 
  {
    ezResourceManager::RegisterResourceOverrideType(ezGetStaticRTTI<ezTypeScriptClassResource>(), [](const ezStringBuilder& sResourceID) -> bool  {
        return sResourceID.HasExtension(".ezTypeScriptRes");
      });
  }

  ON_CORESYSTEMS_SHUTDOWN
  {
    ezResourceManager::UnregisterResourceOverrideType(ezGetStaticRTTI<ezTypeScriptClassResource>());
  }

EZ_END_SUBSYSTEM_DECLARATION;
// clang-format on

ezTypeScriptClassResource::ezTypeScriptClassResource() = default;
ezTypeScriptClassResource::~ezTypeScriptClassResource() = default;

ezResourceLoadDesc ezTypeScriptClassResource::UnloadData(Unload WhatToUnload)
{
  DeleteScriptType();

  ezResourceLoadDesc ld;
  ld.m_State = ezResourceState::Unloaded;
  ld.m_uiQualityLevelsDiscardable = 0;
  ld.m_uiQualityLevelsLoadable = 0;

  return ld;
}

ezResourceLoadDesc ezTypeScriptClassResource::UpdateContent(ezStreamReader* pStream)
{
  ezResourceLoadDesc ld;
  ld.m_uiQualityLevelsDiscardable = 0;
  ld.m_uiQualityLevelsLoadable = 0;

  if (pStream == nullptr)
  {
    ld.m_State = ezResourceState::LoadedResourceMissing;
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

  ezString sTypeName;
  (*pStream) >> sTypeName;
  (*pStream) >> m_Guid;

  ezScriptRTTI::FunctionList functions;
  ezScriptRTTI::MessageHandlerList messageHandlers;

  // TODO: this list should be generated during asset transform and stored in the resource
  const char* szFunctionNames[] = {"Initialize", "Deinitialize", "OnActivated", "OnDeactivated", "OnSimulationStarted", "Tick"};

  for (ezUInt32 i = 0; i < EZ_ARRAY_SIZE(szFunctionNames); ++i)
  {
    functions.PushBack(EZ_DEFAULT_NEW(TypeScriptFunctionProperty, szFunctionNames[i]));
  }

  const ezRTTI* pParentType = ezGetStaticRTTI<ezComponent>();
  CreateScriptType(sTypeName, pParentType, std::move(functions), std::move(messageHandlers));

  ld.m_State = ezResourceState::Loaded;

  return ld;
}

void ezTypeScriptClassResource::UpdateMemoryUsage(MemoryUsage& out_NewMemoryUsage)
{
  out_NewMemoryUsage.m_uiMemoryCPU = (ezUInt32)sizeof(ezTypeScriptClassResource);
  out_NewMemoryUsage.m_uiMemoryGPU = 0;
}

ezUniquePtr<ezScriptInstance> ezTypeScriptClassResource::Instantiate(ezReflectedClass& owner, ezWorld* pWorld) const
{
  auto pComponent = ezStaticCast<ezComponent*>(&owner);

  // TODO: typescript context needs to be moved to a world module
  auto pTypeScriptComponentManager = static_cast<ezTypeScriptComponentManager*>(pWorld->GetManagerForComponentType(ezGetStaticRTTI<ezTypeScriptComponent>()));
  auto& binding = pTypeScriptComponentManager->GetTsBinding();

  ezTypeScriptBinding::TsComponentTypeInfo componentTypeInfo;
  if (binding.LoadComponent(m_Guid, componentTypeInfo).Failed())
  {
    ezLog::Error("Failed to load TS component type.");
    return nullptr;
  }

  ezUInt32 uiStashIdx = 0;
  if (binding.RegisterComponent(m_pType->GetTypeName(), pComponent->GetHandle(), uiStashIdx, false).Failed())
  {
    ezLog::Error("Failed to register TS component type '{}'. Class may not exist under that name.", m_pType->GetTypeName());
    return nullptr;
  }

  return EZ_DEFAULT_NEW(ezTypeScriptInstance, *pComponent, pWorld, binding);
}
