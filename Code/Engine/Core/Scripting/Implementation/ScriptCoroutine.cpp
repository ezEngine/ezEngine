#include <Core/CorePCH.h>

#include <Core/Scripting/ScriptComponent.h>
#include <Core/Scripting/ScriptCoroutine.h>
#include <Core/Scripting/ScriptWorldModule.h>
#include <Foundation/Types/VariantTypeRegistry.h>

// clang-format off
EZ_BEGIN_STATIC_REFLECTED_TYPE(ezScriptCoroutineHandle, ezNoBase, 1, ezRTTIDefaultAllocator<ezScriptCoroutineHandle>)
EZ_END_STATIC_REFLECTED_TYPE;
EZ_DEFINE_CUSTOM_VARIANT_TYPE(ezScriptCoroutineHandle);

EZ_BEGIN_STATIC_REFLECTED_TYPE(ezScriptCoroutine, ezNoBase, 1, ezRTTINoAllocator)
{
  EZ_BEGIN_PROPERTIES
  {
    EZ_ACCESSOR_PROPERTY_READ_ONLY("Name", GetName),
  }
  EZ_END_PROPERTIES;
  EZ_BEGIN_FUNCTIONS
  {
    EZ_FUNCTION_PROPERTY(UpdateAndSchedule),
  }
  EZ_END_FUNCTIONS;
}
EZ_END_STATIC_REFLECTED_TYPE;
// clang-format on

ezScriptCoroutine::ezScriptCoroutine() = default;

ezScriptCoroutine::~ezScriptCoroutine()
{
  EZ_ASSERT_DEV(m_pOwnerModule == nullptr, "Deinitialize was not called");
}

void ezScriptCoroutine::UpdateAndSchedule(ezTime deltaTimeSinceLastUpdate)
{
  auto result = Update(deltaTimeSinceLastUpdate);

  // Has been deleted during update
  if (m_pOwnerModule == nullptr)
    return;

  if (result.m_State == Result::State::Running)
  {
    // We can safely pass false here since we would not end up here if the coroutine is used in a simulation only function
    // but the simulation is not running because then the outer function should not have been called.
    const bool bOnlyWhenSimulating = false;
    m_pOwnerModule->AddUpdateFunctionToSchedule(GetUpdateFunctionProperty(), this, result.m_MaxDelay, bOnlyWhenSimulating);
  }
  else
  {
    m_pOwnerModule->StopAndDeleteCoroutine(GetHandle());
  }
}

void ezScriptCoroutine::Initialize(ezScriptCoroutineId id, ezStringView sName, ezScriptInstance& inout_instance, ezScriptWorldModule& inout_ownerModule)
{
  m_Id = id;
  m_sName.Assign(sName);
  m_pInstance = &inout_instance;
  m_pOwnerModule = &inout_ownerModule;
}

void ezScriptCoroutine::Deinitialize()
{
  m_pOwnerModule->RemoveUpdateFunctionToSchedule(GetUpdateFunctionProperty(), this);
  m_pOwnerModule = nullptr;
}

// static
const ezAbstractFunctionProperty* ezScriptCoroutine::GetUpdateFunctionProperty()
{
  static const ezAbstractFunctionProperty* pUpdateFunctionProperty = []() -> const ezAbstractFunctionProperty*
  {
    const ezRTTI* pType = ezGetStaticRTTI<ezScriptCoroutine>();
    auto functions = pType->GetFunctions();
    for (auto pFunc : functions)
    {
      if (ezStringUtils::IsEqual(pFunc->GetPropertyName(), "UpdateAndSchedule"))
      {
        return pFunc;
      }
    }
    return nullptr;
  }();

  return pUpdateFunctionProperty;
}

//////////////////////////////////////////////////////////////////////////

// clang-format off
EZ_BEGIN_STATIC_REFLECTED_ENUM(ezScriptCoroutineCreationMode, 1)
  EZ_ENUM_CONSTANTS(ezScriptCoroutineCreationMode::StopOther, ezScriptCoroutineCreationMode::DontCreateNew, ezScriptCoroutineCreationMode::AllowOverlap)
EZ_END_STATIC_REFLECTED_ENUM;
// clang-format on

//////////////////////////////////////////////////////////////////////////

ezScriptCoroutineRTTI::ezScriptCoroutineRTTI(ezStringView sName, ezUniquePtr<ezRTTIAllocator>&& pAllocator)
  : ezRTTI(nullptr, ezGetStaticRTTI<ezScriptCoroutine>(), 0, 1, ezVariantType::Invalid, ezTypeFlags::Class, nullptr, ezArrayPtr<const ezAbstractProperty*>(), ezArrayPtr<const ezAbstractFunctionProperty*>(), ezArrayPtr<const ezPropertyAttribute*>(), ezArrayPtr<ezAbstractMessageHandler*>(), ezArrayPtr<ezMessageSenderInfo>(), nullptr)
  , m_sTypeNameStorage(sName)
  , m_pAllocatorStorage(std::move(pAllocator))
{
  m_sTypeName = m_sTypeNameStorage;
  m_pAllocator = m_pAllocatorStorage.Borrow();

  RegisterType();

  SetupParentHierarchy();
}

ezScriptCoroutineRTTI::~ezScriptCoroutineRTTI()
{
  UnregisterType();
  m_sTypeName = nullptr;
}

//////////////////////////////////////////////////////////////////////////

ezScriptCoroutineFunctionProperty::ezScriptCoroutineFunctionProperty(ezStringView sName, const ezSharedPtr<ezScriptCoroutineRTTI>& pType, ezScriptCoroutineCreationMode::Enum creationMode)
  : ezScriptFunctionProperty(sName)
  , m_pType(pType)
  , m_CreationMode(creationMode)
{
}

ezScriptCoroutineFunctionProperty::~ezScriptCoroutineFunctionProperty() = default;

void ezScriptCoroutineFunctionProperty::Execute(void* pInstance, ezArrayPtr<ezVariant> arguments, ezVariant& out_returnValue) const
{
  EZ_IGNORE_UNUSED(out_returnValue);

  EZ_ASSERT_DEBUG(pInstance != nullptr, "Invalid instance");
  auto pScriptInstance = static_cast<ezScriptInstance*>(pInstance);

  ezWorld* pWorld = pScriptInstance->GetWorld();
  if (pWorld == nullptr)
  {
    ezLog::Error("Script coroutines need a script instance with a valid ezWorld");
    return;
  }

  auto pModule = pWorld->GetOrCreateModule<ezScriptWorldModule>();

  ezScriptCoroutine* pCoroutine = nullptr;
  auto hCoroutine = pModule->CreateCoroutine(m_pType.Borrow(), m_szPropertyName, *pScriptInstance, m_CreationMode, pCoroutine);

  if (pCoroutine != nullptr)
  {
    ezHybridArray<ezVariant, 8> finalArgs;
    finalArgs = arguments;
    finalArgs.PushBack(hCoroutine);

    pModule->StartCoroutine(hCoroutine, finalArgs);
  }
}

//////////////////////////////////////////////////////////////////////////

ezScriptCoroutineMessageHandler::ezScriptCoroutineMessageHandler(ezStringView sName, const ezScriptMessageDesc& desc, const ezSharedPtr<ezScriptCoroutineRTTI>& pType, ezScriptCoroutineCreationMode::Enum creationMode)
  : ezScriptMessageHandler(desc)
  , m_pType(pType)
  , m_CreationMode(creationMode)
{
  m_sName.Assign(sName);
  m_DispatchFunc = &Dispatch;
}

ezScriptCoroutineMessageHandler::~ezScriptCoroutineMessageHandler() = default;

// static
void ezScriptCoroutineMessageHandler::Dispatch(ezAbstractMessageHandler* pSelf, void* pInstance, ezMessage& ref_msg)
{
  EZ_ASSERT_DEBUG(pInstance != nullptr, "Invalid instance");
  auto pHandler = static_cast<ezScriptCoroutineMessageHandler*>(pSelf);
  auto pComponent = static_cast<ezScriptComponent*>(pInstance);
  auto pScriptInstance = pComponent->GetScriptInstance();

  ezWorld* pWorld = pScriptInstance->GetWorld();
  if (pWorld == nullptr)
  {
    ezLog::Error("Script coroutines need a script instance with a valid ezWorld");
    return;
  }

  auto pModule = pWorld->GetOrCreateModule<ezScriptWorldModule>();

  ezScriptCoroutine* pCoroutine = nullptr;
  auto hCoroutine = pModule->CreateCoroutine(pHandler->m_pType.Borrow(), pHandler->m_sName, *pScriptInstance, pHandler->m_CreationMode, pCoroutine);

  if (pCoroutine != nullptr)
  {
    ezHybridArray<ezVariant, 8> arguments;
    pHandler->FillMessagePropertyValues(ref_msg, arguments);
    arguments.PushBack(hCoroutine);

    pModule->StartCoroutine(hCoroutine, arguments);
  }
}


EZ_STATICLINK_FILE(Core, Core_Scripting_Implementation_ScriptCoroutine);
