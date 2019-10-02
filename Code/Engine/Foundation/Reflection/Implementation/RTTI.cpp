#include <FoundationPCH.h>

#include <Foundation/Reflection/Implementation/AbstractProperty.h>
#include <Foundation/Reflection/Implementation/MessageHandler.h>

#include <Foundation/Communication/Message.h>
#include <Foundation/Configuration/Startup.h>
#include <Foundation/Containers/HashTable.h>

typedef ezHashTable<const char*, ezRTTI*, ezHashHelper<const char*>, ezStaticAllocatorWrapper> ezTypeHashTable;
ezEvent<const ezRTTI*> ezRTTI::s_TypeUpdatedEvent;

EZ_ENUMERABLE_CLASS_IMPLEMENTATION(ezRTTI);

// clang-format off
EZ_BEGIN_SUBSYSTEM_DECLARATION(Foundation, Reflection)

  //BEGIN_SUBSYSTEM_DEPENDENCIES
  //  "FileSystem"
  //END_SUBSYSTEM_DEPENDENCIES

  ON_CORESYSTEMS_STARTUP
  {
    ezPlugin::s_PluginEvents.AddEventHandler(ezRTTI::PluginEventHandler);
    ezRTTI::AssignPlugin("Static");
  }

  ON_CORESYSTEMS_SHUTDOWN
  {
    ezPlugin::s_PluginEvents.RemoveEventHandler(ezRTTI::PluginEventHandler);
  }

EZ_END_SUBSYSTEM_DECLARATION;
// clang-format on

ezRTTI::ezRTTI(const char* szName, const ezRTTI* pParentType, ezUInt32 uiTypeSize, ezUInt32 uiTypeVersion, ezUInt32 uiVariantType,
  ezBitflags<ezTypeFlags> flags, ezRTTIAllocator* pAllocator, ezArrayPtr<ezAbstractProperty*> properties,
  ezArrayPtr<ezAbstractProperty*> functions, ezArrayPtr<ezPropertyAttribute*> attributes,
  ezArrayPtr<ezAbstractMessageHandler*> messageHandlers, ezArrayPtr<ezMessageSenderInfo> messageSenders, const ezRTTI* (*fnVerifyParent)())
{
  UpdateType(pParentType, uiTypeSize, uiTypeVersion, uiVariantType, flags);

  m_bGatheredDynamicMessageHandlers = false;
  m_szPluginName = nullptr;
  m_szTypeName = szName;
  m_pAllocator = pAllocator;
  m_Properties = properties;
  m_Functions = ezMakeArrayPtr<ezAbstractFunctionProperty*>(reinterpret_cast<ezAbstractFunctionProperty**>(functions.GetPtr()), functions.GetCount());
  ;
  m_Attributes = attributes;
  m_MessageHandlers = messageHandlers;
  m_uiMsgIdOffset = 0;
  m_MessageSenders = messageSenders;

  m_fnVerifyParent = fnVerifyParent;

  // This part is not guaranteed to always work here!
  // pParentType is (apparently) always the correct pointer to the base class BUT it is not guaranteed to have been constructed at this
  // point in time! Therefore the message handler hierarchy is initialized delayed in DispatchMessage
  //
  // However, I don't know where we could do these debug checks where they are guaranteed to be executed.
  // For now they are executed here and one might also do that in e.g. the game application
  {
#if EZ_ENABLED(EZ_COMPILE_FOR_DEVELOPMENT)
    VerifyCorrectness();
#endif
  }

  if (m_szTypeName)
    RegisterType(this);
}

ezRTTI::~ezRTTI()
{
  if (m_szTypeName)
    UnregisterType(this);
}

void ezRTTI::GatherDynamicMessageHandlers()
{
  // This cannot be done in the constructor, because the parent types are not guaranteed to be initialized at that point

  if (m_bGatheredDynamicMessageHandlers)
    return;

  m_bGatheredDynamicMessageHandlers = true;

  ezUInt32 uiMinMsgId = ezInvalidIndex;
  ezUInt32 uiMaxMsgId = 0;

  const ezRTTI* pInstance = this;
  while (pInstance != nullptr)
  {
    for (ezUInt32 i = 0; i < pInstance->m_MessageHandlers.GetCount(); ++i)
    {
      ezUInt32 id = pInstance->m_MessageHandlers[i]->GetMessageId();
      uiMinMsgId = ezMath::Min(uiMinMsgId, id);
      uiMaxMsgId = ezMath::Max(uiMaxMsgId, id);
    }

    pInstance = pInstance->m_pParentType;
  }

  if (uiMinMsgId != ezInvalidIndex)
  {
    m_uiMsgIdOffset = uiMinMsgId;
    ezUInt32 uiNeededCapacity = uiMaxMsgId - uiMinMsgId + 1;

    m_DynamicMessageHandlers.SetCount(uiNeededCapacity);

    pInstance = this;
    while (pInstance != nullptr)
    {
      for (ezUInt32 i = 0; i < pInstance->m_MessageHandlers.GetCount(); ++i)
      {
        ezAbstractMessageHandler* pHandler = pInstance->m_MessageHandlers[i];
        ezUInt32 uiIndex = pHandler->GetMessageId() - m_uiMsgIdOffset;

        // this check ensures that handlers in base classes do not override the derived handlers
        if (m_DynamicMessageHandlers[uiIndex] == nullptr)
        {
          m_DynamicMessageHandlers[uiIndex] = pHandler;
        }
      }

      pInstance = pInstance->m_pParentType;
    }
  }
}

void* ezRTTI::GetTypeHashTable()
{
  // Prevent static initialization hazard between first ezRTTI instance
  // and the hash table and also make sure it is sufficiently sized before first use.
  auto CreateTable = []() -> ezTypeHashTable* {
    ezTypeHashTable* table = new ezTypeHashTable();
    table->Reserve(512);
    return table;
  };
  static ezTypeHashTable* table = CreateTable();
  return table;
}

void ezRTTI::VerifyCorrectness() const
{
  if (m_fnVerifyParent != nullptr)
  {
    EZ_ASSERT_DEV(m_fnVerifyParent() == m_pParentType,
      "Type '{0}': The given parent type '{1}' does not match the actual parent type '{2}'", m_szTypeName,
      (m_pParentType != nullptr) ? m_pParentType->GetTypeName() : "null",
      (m_fnVerifyParent() != nullptr) ? m_fnVerifyParent()->GetTypeName() : "null");
  }

  {
    ezSet<ezStringView> Known;

    const ezRTTI* pInstance = this;

    while (pInstance != nullptr)
    {
      for (ezUInt32 i = 0; i < pInstance->m_Properties.GetCount(); ++i)
      {
        const bool bNewProperty = !Known.Find(pInstance->m_Properties[i]->GetPropertyName()).IsValid();
        Known.Insert(pInstance->m_Properties[i]->GetPropertyName());

        EZ_ASSERT_DEV(bNewProperty, "{0}: The property with name '{1}' is already defined in type '{2}'.", m_szTypeName,
          pInstance->m_Properties[i]->GetPropertyName(), pInstance->GetTypeName());
      }

      pInstance = pInstance->m_pParentType;
    }
  }

  {
    for (ezAbstractProperty* pFunc : m_Functions)
    {
      EZ_ASSERT_DEV(pFunc->GetCategory() == ezPropertyCategory::Function, "Invalid function property '{}'", pFunc->GetPropertyName());
    }
  }
}

void ezRTTI::VerifyCorrectnessForAllTypes()
{
  ezRTTI* pRtti = ezRTTI::GetFirstInstance();

  while (pRtti)
  {
    pRtti->VerifyCorrectness();
    pRtti = pRtti->GetNextInstance();
  }
}


void ezRTTI::UpdateType(
  const ezRTTI* pParentType, ezUInt32 uiTypeSize, ezUInt32 uiTypeVersion, ezUInt32 uiVariantType, ezBitflags<ezTypeFlags> flags)
{
  m_pParentType = pParentType;
  m_uiVariantType = uiVariantType;
  m_uiTypeSize = uiTypeSize;
  m_uiTypeVersion = uiTypeVersion;
  m_TypeFlags = flags;
}

void ezRTTI::RegisterType(ezRTTI* pType)
{
  m_uiTypeNameHash = ezHashingUtils::MurmurHash32String(m_szTypeName);
  static_cast<ezTypeHashTable*>(ezRTTI::GetTypeHashTable())->Insert(pType->m_szTypeName, pType);
}

void ezRTTI::UnregisterType(ezRTTI* pType)
{
  static_cast<ezTypeHashTable*>(ezRTTI::GetTypeHashTable())->Remove(m_szTypeName);
}

bool ezRTTI::IsDerivedFrom(const ezRTTI* pBaseType) const
{
  const ezRTTI* pThis = this;

  while (pThis != nullptr)
  {
    if (pThis == pBaseType)
      return true;

    pThis = pThis->m_pParentType;
  }

  return false;
}

void ezRTTI::GetAllProperties(ezHybridArray<ezAbstractProperty*, 32>& out_Properties) const
{
  out_Properties.Clear();

  if (m_pParentType)
    m_pParentType->GetAllProperties(out_Properties);

  out_Properties.PushBackRange(GetProperties());
}

ezRTTI* ezRTTI::FindTypeByName(const char* szName)
{
  ezRTTI* pInstance = nullptr;
  if (static_cast<ezTypeHashTable*>(ezRTTI::GetTypeHashTable())->TryGetValue(szName, pInstance))
    return pInstance;

#if EZ_ENABLED(EZ_COMPILE_FOR_DEBUG)
  pInstance = ezRTTI::GetFirstInstance();

  while (pInstance)
  {
    if (ezStringUtils::IsEqual(pInstance->GetTypeName(), szName))
    {
      EZ_REPORT_FAILURE("The hash table lookup should have already found the RTTI type '{}'", szName);
      return pInstance;
    }

    pInstance = pInstance->GetNextInstance();
  }
#endif

  return nullptr;
}

ezRTTI* ezRTTI::FindTypeByNameHash(ezUInt32 uiNameHash)
{
  // TODO: actually reuse the hash table for the lookup

  ezRTTI* pInstance = ezRTTI::GetFirstInstance();

  while (pInstance)
  {
    if (pInstance->GetTypeNameHash() == uiNameHash)
      return pInstance;

    pInstance = pInstance->GetNextInstance();
  }

  return nullptr;
}

ezAbstractProperty* ezRTTI::FindPropertyByName(const char* szName, bool bSearchBaseTypes /* = true */) const
{
  const ezRTTI* pInstance = this;

  do
  {
    for (ezUInt32 p = 0; p < pInstance->m_Properties.GetCount(); ++p)
    {
      if (ezStringUtils::IsEqual(pInstance->m_Properties[p]->GetPropertyName(), szName))
      {
        return pInstance->m_Properties[p];
      }
    }

    if (!bSearchBaseTypes)
      return nullptr;

    pInstance = pInstance->m_pParentType;
  } while (pInstance != nullptr);

  return nullptr;
}

bool ezRTTI::DispatchMessage(void* pInstance, ezMessage& msg) const
{
  EZ_ASSERT_DEBUG(m_bGatheredDynamicMessageHandlers, "Message handler table should have been gathered at this point.\n"
                                                     "If this assert is triggered for a type loaded from a dynamic plugin,\n"
                                                     "you may have forgotten to instantiate an ezPlugin object inside your plugin DLL.");

  const ezUInt32 uiIndex = msg.GetId() - m_uiMsgIdOffset;

  // m_DynamicMessageHandlers contains all message handlers of this type and all base types
  if (uiIndex < m_DynamicMessageHandlers.GetCount())
  {
    ezAbstractMessageHandler* pHandler = m_DynamicMessageHandlers[uiIndex];
    if (pHandler != nullptr)
    {
      (*pHandler)(pInstance, msg);
      return true;
    }
  }

  return false;
}

bool ezRTTI::DispatchMessage(const void* pInstance, ezMessage& msg) const
{
  EZ_ASSERT_DEBUG(m_bGatheredDynamicMessageHandlers, "Message handler table should have been gathered at this point.\n"
                                                     "If this assert is triggered for a type loaded from a dynamic plugin,\n"
                                                     "you may have forgotten to instantiate an ezPlugin object inside your plugin DLL.");

  const ezUInt32 uiIndex = msg.GetId() - m_uiMsgIdOffset;

  // m_DynamicMessageHandlers contains all message handlers of this type and all base types
  if (uiIndex < m_DynamicMessageHandlers.GetCount())
  {
    ezAbstractMessageHandler* pHandler = m_DynamicMessageHandlers[uiIndex];
    if (pHandler != nullptr && pHandler->IsConst())
    {
      (*pHandler)(pInstance, msg);
      return true;
    }
  }

  return false;
}

void ezRTTI::AssignPlugin(const char* szPluginName)
{
  // assigns the given plugin name to every ezRTTI instance that has no plugin assigned yet

  ezRTTI* pInstance = ezRTTI::GetFirstInstance();

  while (pInstance)
  {
    if (pInstance->m_szPluginName == nullptr)
    {
      pInstance->m_szPluginName = szPluginName;
      SanityCheckType(pInstance);

      pInstance->GatherDynamicMessageHandlers();
    }
    pInstance = pInstance->GetNextInstance();
  }
}

#define EZ_MSVC_WARNING_NUMBER 4505
#include <Foundation/Basics/Compiler/DisableWarning.h>

static bool IsValidIdentifierName(const char* szIdentifier)
{
  // empty strings are not valid
  if (ezStringUtils::IsNullOrEmpty(szIdentifier))
    return false;

  // digits are not allowed as the first character
  if (szIdentifier[0] >= '0' && szIdentifier[0] <= '9')
    return false;

  for (const char* s = szIdentifier; *s != '\0'; ++s)
  {
    const char c = *s;

    if (c >= 'a' && c <= 'z')
      continue;
    if (c >= 'A' && c <= 'Z')
      continue;
    if (c >= '0' && c <= '9')
      continue;
    if (c >= '_')
      continue;
    if (c >= ':')
      continue;

    return false;
  }

  return true;
}

void ezRTTI::SanityCheckType(ezRTTI* pType)
{
  EZ_ASSERT_DEV(pType->GetTypeFlags().IsSet(ezTypeFlags::StandardType) + pType->GetTypeFlags().IsSet(ezTypeFlags::IsEnum) +
                    pType->GetTypeFlags().IsSet(ezTypeFlags::Bitflags) + pType->GetTypeFlags().IsSet(ezTypeFlags::Class) ==
                  1,
    "Types are mutually exclusive!");

  for (auto pProp : pType->m_Properties)
  {
    const ezRTTI* pSpecificType = pProp->GetSpecificType();

    EZ_ASSERT_DEBUG(IsValidIdentifierName(pProp->GetPropertyName()), "Property name is invalid: '{0}'", pProp->GetPropertyName());

    // if (!IsValidIdentifierName(pProp->GetPropertyName()))
    //{
    //  ezStringBuilder s;
    //  s.Format("RTTI: {0}\n", pProp->GetPropertyName());

    //  OutputDebugStringA(s.GetData());
    //}

    if (pProp->GetCategory() != ezPropertyCategory::Function)
    {
      EZ_ASSERT_DEV(pProp->GetFlags().IsSet(ezPropertyFlags::StandardType) + pProp->GetFlags().IsSet(ezPropertyFlags::IsEnum) +
                        pProp->GetFlags().IsSet(ezPropertyFlags::Bitflags) + pProp->GetFlags().IsSet(ezPropertyFlags::Class) <=
                      1,
        "Types are mutually exclusive!");
    }

    switch (pProp->GetCategory())
    {
      case ezPropertyCategory::Constant:
      {
        EZ_ASSERT_DEV(pSpecificType->GetTypeFlags().IsSet(ezTypeFlags::StandardType), "Only standard type constants are supported!");
      }
      break;
      case ezPropertyCategory::Member:
      {
        EZ_ASSERT_DEV(
          pProp->GetFlags().IsSet(ezPropertyFlags::StandardType) == pSpecificType->GetTypeFlags().IsSet(ezTypeFlags::StandardType),
          "Property-Type missmatch!");
        EZ_ASSERT_DEV(pProp->GetFlags().IsSet(ezPropertyFlags::IsEnum) == pSpecificType->GetTypeFlags().IsSet(ezTypeFlags::IsEnum),
          "Property-Type missmatch! Use EZ_BEGIN_STATIC_REFLECTED_ENUM for type and EZ_ENUM_MEMBER_PROPERTY / "
          "EZ_ENUM_ACCESSOR_PROPERTY for property.");
        EZ_ASSERT_DEV(pProp->GetFlags().IsSet(ezPropertyFlags::Bitflags) == pSpecificType->GetTypeFlags().IsSet(ezTypeFlags::Bitflags),
          "Property-Type missmatch! Use EZ_BEGIN_STATIC_REFLECTED_ENUM for type and EZ_BITFLAGS_MEMBER_PROPERTY / "
          "EZ_BITFLAGS_ACCESSOR_PROPERTY for property.");
        EZ_ASSERT_DEV(pProp->GetFlags().IsSet(ezPropertyFlags::Class) == pSpecificType->GetTypeFlags().IsSet(ezTypeFlags::Class),
          "If ezPropertyFlags::Class is set, the property type must be ezTypeFlags::Class and vise versa.");
      }
      break;
      case ezPropertyCategory::Array:
      case ezPropertyCategory::Set:
      case ezPropertyCategory::Map:
      {
        EZ_ASSERT_DEV(
          pProp->GetFlags().IsSet(ezPropertyFlags::StandardType) == pSpecificType->GetTypeFlags().IsSet(ezTypeFlags::StandardType),
          "Property-Type missmatch!");
        EZ_ASSERT_DEV(pProp->GetFlags().IsSet(ezPropertyFlags::Class) == pSpecificType->GetTypeFlags().IsSet(ezTypeFlags::Class),
          "If ezPropertyFlags::Class is set, the property type must be ezTypeFlags::Class and vise versa.");
      }
      break;
      case ezPropertyCategory::Function:
        EZ_REPORT_FAILURE("Functions need to be put into the EZ_BEGIN_FUNCTIONS / EZ_END_FUNCTIONS; block.");
        break;
    }
  }
}

void ezRTTI::PluginEventHandler(const ezPlugin::PluginEvent& EventData)
{
  switch (EventData.m_EventType)
  {
    case ezPlugin::PluginEvent::BeforeLoading:
    {
      // before a new plugin is loaded, make sure all current ezRTTI instances
      // are assigned to the proper plugin
      // all not-yet assigned rtti instances cannot be in any plugin, so assign them to the 'static' plugin
      AssignPlugin("Static");
    }
    break;

    case ezPlugin::PluginEvent::AfterLoadingBeforeInit:
    {
      // after we loaded a new plugin, but before it is initialized,
      // find all new rtti instances and assign them to that new plugin
      if (EventData.m_pPluginObject)
        AssignPlugin(EventData.m_pPluginObject->GetPluginName());

#if EZ_ENABLED(EZ_COMPILE_FOR_DEBUG)
      ezRTTI::VerifyCorrectnessForAllTypes();
#endif
    }
    break;

    default:
      break;
  }
}



EZ_STATICLINK_FILE(Foundation, Foundation_Reflection_Implementation_RTTI);
