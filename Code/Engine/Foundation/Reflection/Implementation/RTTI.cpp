#include <Foundation/FoundationPCH.h>

#include <Foundation/Reflection/Implementation/AbstractProperty.h>
#include <Foundation/Reflection/Implementation/MessageHandler.h>

#include <Foundation/Communication/Message.h>
#include <Foundation/Configuration/Startup.h>
#include <Foundation/Containers/HashTable.h>

struct ezTypeData
{
  ezMutex m_Mutex;
  ezHashTable<ezUInt64, ezRTTI*, ezHashHelper<ezUInt64>, ezStaticsAllocatorWrapper> m_TypeNameHashToType;
  ezDynamicArray<ezRTTI*> m_AllTypes;

  bool m_bIterating = false;
};

ezTypeData* GetTypeData()
{
  // Prevent static initialization hazard between first ezRTTI instance
  // and type data and also make sure it is sufficiently sized before first use.
  auto CreateData = []() -> ezTypeData*
  {
    ezTypeData* pData = new ezTypeData();
    pData->m_TypeNameHashToType.Reserve(512);
    pData->m_AllTypes.Reserve(512);
    return pData;
  };
  static ezTypeData* pData = CreateData();
  return pData;
}

// clang-format off
EZ_BEGIN_SUBSYSTEM_DECLARATION(Foundation, Reflection)

  //BEGIN_SUBSYSTEM_DEPENDENCIES
  //  "FileSystem"
  //END_SUBSYSTEM_DEPENDENCIES

  ON_CORESYSTEMS_STARTUP
  {
    ezPlugin::Events().AddEventHandler(ezRTTI::PluginEventHandler);
    ezRTTI::AssignPlugin("Static");
  }

  ON_CORESYSTEMS_SHUTDOWN
  {
    ezPlugin::Events().RemoveEventHandler(ezRTTI::PluginEventHandler);
  }

EZ_END_SUBSYSTEM_DECLARATION;
// clang-format on

ezRTTI::ezRTTI(ezStringView sName, const ezRTTI* pParentType, ezUInt32 uiTypeSize, ezUInt32 uiTypeVersion, ezUInt8 uiVariantType,
  ezBitflags<ezTypeFlags> flags, ezRTTIAllocator* pAllocator, ezArrayPtr<const ezAbstractProperty*> properties, ezArrayPtr<const ezAbstractFunctionProperty*> functions,
  ezArrayPtr<const ezPropertyAttribute*> attributes, ezArrayPtr<ezAbstractMessageHandler*> messageHandlers, ezArrayPtr<ezMessageSenderInfo> messageSenders,
  const ezRTTI* (*fnVerifyParent)())
  : m_sTypeName(sName)
  , m_Properties(properties)
  , m_Functions(functions)
  , m_Attributes(attributes)
  , m_pAllocator(pAllocator)
  , m_VerifyParent(fnVerifyParent)
  , m_MessageHandlers(messageHandlers)
  , m_MessageSenders(messageSenders)
{
  UpdateType(pParentType, uiTypeSize, uiTypeVersion, uiVariantType, flags);

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

  if (!m_sTypeName.IsEmpty())
  {
    RegisterType();
  }
}

ezRTTI::~ezRTTI()
{
  if (!m_sTypeName.IsEmpty())
  {
    UnregisterType();
  }
}

void ezRTTI::GatherDynamicMessageHandlers()
{
  // This cannot be done in the constructor, because the parent types are not guaranteed to be initialized at that point

  if (m_uiMsgIdOffset != ezSmallInvalidIndex)
    return;

  m_uiMsgIdOffset = 0;

  ezUInt16 uiMinMsgId = ezSmallInvalidIndex;
  ezUInt16 uiMaxMsgId = 0;

  const ezRTTI* pInstance = this;
  while (pInstance != nullptr)
  {
    for (ezUInt32 i = 0; i < pInstance->m_MessageHandlers.GetCount(); ++i)
    {
      ezUInt16 id = pInstance->m_MessageHandlers[i]->GetMessageId();
      uiMinMsgId = ezMath::Min(uiMinMsgId, id);
      uiMaxMsgId = ezMath::Max(uiMaxMsgId, id);
    }

    pInstance = pInstance->m_pParentType;
  }

  if (uiMinMsgId != ezSmallInvalidIndex)
  {
    m_uiMsgIdOffset = uiMinMsgId;
    ezUInt16 uiNeededCapacity = uiMaxMsgId - uiMinMsgId + 1;

    m_DynamicMessageHandlers.SetCount(uiNeededCapacity);

    pInstance = this;
    while (pInstance != nullptr)
    {
      for (ezUInt32 i = 0; i < pInstance->m_MessageHandlers.GetCount(); ++i)
      {
        ezAbstractMessageHandler* pHandler = pInstance->m_MessageHandlers[i];
        ezUInt16 uiIndex = pHandler->GetMessageId() - m_uiMsgIdOffset;

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

void ezRTTI::SetupParentHierarchy()
{
  m_ParentHierarchy.Clear();

  for (const ezRTTI* rtti = this; rtti != nullptr; rtti = rtti->m_pParentType)
  {
    m_ParentHierarchy.PushBack(rtti);
  }
}

void ezRTTI::VerifyCorrectness() const
{
  if (m_VerifyParent != nullptr)
  {
    EZ_ASSERT_DEV(m_VerifyParent() == m_pParentType, "Type '{0}': The given parent type '{1}' does not match the actual parent type '{2}'",
      m_sTypeName, (m_pParentType != nullptr) ? m_pParentType->GetTypeName() : "null",
      (m_VerifyParent() != nullptr) ? m_VerifyParent()->GetTypeName() : "null");
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

        EZ_IGNORE_UNUSED(bNewProperty);
        EZ_ASSERT_DEV(bNewProperty, "{0}: The property with name '{1}' is already defined in type '{2}'.", m_sTypeName,
          pInstance->m_Properties[i]->GetPropertyName(), pInstance->GetTypeName());
      }

      pInstance = pInstance->m_pParentType;
    }
  }

  {
    for (const ezAbstractProperty* pFunc : m_Functions)
    {
      EZ_IGNORE_UNUSED(pFunc);
      EZ_ASSERT_DEV(pFunc->GetCategory() == ezPropertyCategory::Function, "Invalid function property '{}'", pFunc->GetPropertyName());
    }
  }
}

void ezRTTI::VerifyCorrectnessForAllTypes()
{
  ezRTTI::ForEachType([](const ezRTTI* pRtti)
    { pRtti->VerifyCorrectness(); });
}


void ezRTTI::UpdateType(const ezRTTI* pParentType, ezUInt32 uiTypeSize, ezUInt32 uiTypeVersion, ezUInt8 uiVariantType, ezBitflags<ezTypeFlags> flags)
{
  m_pParentType = pParentType;
  m_uiVariantType = uiVariantType;
  m_uiTypeSize = uiTypeSize;
  m_uiTypeVersion = uiTypeVersion;
  m_TypeFlags = flags;
  m_ParentHierarchy.Clear();
}

void ezRTTI::RegisterType()
{
  m_uiTypeNameHash = ezHashingUtils::StringHash(m_sTypeName);

  auto pData = GetTypeData();
  EZ_LOCK(pData->m_Mutex);
  pData->m_TypeNameHashToType.Insert(m_uiTypeNameHash, this);

  m_uiTypeIndex = pData->m_AllTypes.GetCount();
  pData->m_AllTypes.PushBack(this);
}

void ezRTTI::UnregisterType()
{
  auto pData = GetTypeData();
  EZ_LOCK(pData->m_Mutex);
  pData->m_TypeNameHashToType.Remove(m_uiTypeNameHash);

  EZ_ASSERT_DEV(pData->m_bIterating == false, "Unregistering types while iterating over types might cause unexpected behavior");
  pData->m_AllTypes.RemoveAtAndSwap(m_uiTypeIndex);
  if (m_uiTypeIndex != pData->m_AllTypes.GetCount())
  {
    pData->m_AllTypes[m_uiTypeIndex]->m_uiTypeIndex = m_uiTypeIndex;
  }
}

void ezRTTI::GetAllProperties(ezDynamicArray<const ezAbstractProperty*>& out_properties) const
{
  out_properties.Clear();

  if (m_pParentType)
    m_pParentType->GetAllProperties(out_properties);

  out_properties.PushBackRange(GetProperties());
}

const ezRTTI* ezRTTI::FindTypeByName(ezStringView sName)
{
  ezUInt64 uiNameHash = ezHashingUtils::StringHash(sName);

  auto pData = GetTypeData();
  EZ_LOCK(pData->m_Mutex);

  ezRTTI* pType = nullptr;
  pData->m_TypeNameHashToType.TryGetValue(uiNameHash, pType);
  return pType;
}

const ezRTTI* ezRTTI::FindTypeByNameHash(ezUInt64 uiNameHash)
{
  auto pData = GetTypeData();
  EZ_LOCK(pData->m_Mutex);

  ezRTTI* pType = nullptr;
  pData->m_TypeNameHashToType.TryGetValue(uiNameHash, pType);
  return pType;
}

const ezRTTI* ezRTTI::FindTypeByNameHash32(ezUInt32 uiNameHash)
{
  return FindTypeIf([=](const ezRTTI* pRtti)
    { return (ezHashingUtils::StringHashTo32(pRtti->GetTypeNameHash()) == uiNameHash); });
}

const ezRTTI* ezRTTI::FindTypeIf(PredicateFunc func)
{
  auto pData = GetTypeData();
  EZ_LOCK(pData->m_Mutex);

  for (const ezRTTI* pRtti : pData->m_AllTypes)
  {
    if (func(pRtti))
    {
      return pRtti;
    }
  }

  return nullptr;
}

const ezAbstractProperty* ezRTTI::FindPropertyByName(ezStringView sName, bool bSearchBaseTypes /* = true */) const
{
  const ezRTTI* pInstance = this;

  do
  {
    for (ezUInt32 p = 0; p < pInstance->m_Properties.GetCount(); ++p)
    {
      if (pInstance->m_Properties[p]->GetPropertyName() == sName)
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

bool ezRTTI::DispatchMessage(void* pInstance, ezMessage& ref_msg) const
{
  EZ_ASSERT_DEBUG(m_uiMsgIdOffset != ezSmallInvalidIndex, "Message handler table should have been gathered at this point.\n"
                                                          "If this assert is triggered for a type loaded from a dynamic plugin,\n"
                                                          "you may have forgotten to instantiate an ezPlugin object inside your plugin DLL.");

  const ezUInt32 uiIndex = ref_msg.GetId() - m_uiMsgIdOffset;

  // m_DynamicMessageHandlers contains all message handlers of this type and all base types
  if (uiIndex < m_DynamicMessageHandlers.GetCount())
  {
    ezAbstractMessageHandler* pHandler = m_DynamicMessageHandlers.GetData()[uiIndex];
    if (pHandler != nullptr)
    {
      (*pHandler)(pInstance, ref_msg);
      return true;
    }
  }

  return false;
}

bool ezRTTI::DispatchMessage(const void* pInstance, ezMessage& ref_msg) const
{
  EZ_ASSERT_DEBUG(m_uiMsgIdOffset != ezSmallInvalidIndex, "Message handler table should have been gathered at this point.\n"
                                                          "If this assert is triggered for a type loaded from a dynamic plugin,\n"
                                                          "you may have forgotten to instantiate an ezPlugin object inside your plugin DLL.");

  const ezUInt32 uiIndex = ref_msg.GetId() - m_uiMsgIdOffset;

  // m_DynamicMessageHandlers contains all message handlers of this type and all base types
  if (uiIndex < m_DynamicMessageHandlers.GetCount())
  {
    ezAbstractMessageHandler* pHandler = m_DynamicMessageHandlers.GetData()[uiIndex];
    if (pHandler != nullptr && pHandler->IsConst())
    {
      (*pHandler)(pInstance, ref_msg);
      return true;
    }
  }

  return false;
}

void ezRTTI::ForEachType(VisitorFunc func, ezBitflags<ForEachOptions> options /*= ForEachOptions::Default*/)
{
  auto pData = GetTypeData();
  EZ_LOCK(pData->m_Mutex);

  pData->m_bIterating = true;
  // Can't use ranged based for loop here since we might add new types while iterating and the m_AllTypes array might re-allocate.
  for (ezUInt32 i = 0; i < pData->m_AllTypes.GetCount(); ++i)
  {
    auto pRtti = pData->m_AllTypes.GetData()[i];
    if (options.IsSet(ForEachOptions::ExcludeNonAllocatable) && (pRtti->GetAllocator() == nullptr || pRtti->GetAllocator()->CanAllocate() == false))
      continue;

    if (options.IsSet(ForEachOptions::ExcludeAbstract) && pRtti->GetTypeFlags().IsSet(ezTypeFlags::Abstract))
      continue;

    func(pRtti);
  }
  pData->m_bIterating = false;
}

void ezRTTI::ForEachDerivedType(const ezRTTI* pBaseType, VisitorFunc func, ezBitflags<ForEachOptions> options /*= ForEachOptions::Default*/)
{
  auto pData = GetTypeData();
  EZ_LOCK(pData->m_Mutex);

  pData->m_bIterating = true;
  // Can't use ranged based for loop here since we might add new types while iterating and the m_AllTypes array might re-allocate.
  for (ezUInt32 i = 0; i < pData->m_AllTypes.GetCount(); ++i)
  {
    auto pRtti = pData->m_AllTypes.GetData()[i];
    if (!pRtti->IsDerivedFrom(pBaseType))
      continue;

    if (options.IsSet(ForEachOptions::ExcludeNonAllocatable) && (pRtti->GetAllocator() == nullptr || pRtti->GetAllocator()->CanAllocate() == false))
      continue;

    if (options.IsSet(ForEachOptions::ExcludeAbstract) && pRtti->GetTypeFlags().IsSet(ezTypeFlags::Abstract))
      continue;

    func(pRtti);
  }
  pData->m_bIterating = false;
}

void ezRTTI::AssignPlugin(ezStringView sPluginName)
{
  // assigns the given plugin name to every ezRTTI instance that has no plugin assigned yet

  auto pData = GetTypeData();
  EZ_LOCK(pData->m_Mutex);

  for (ezRTTI* pRtti : pData->m_AllTypes)
  {
    if (pRtti->m_sPluginName.IsEmpty())
    {
      pRtti->m_sPluginName = sPluginName;
      SanityCheckType(pRtti);

      pRtti->SetupParentHierarchy();
      pRtti->GatherDynamicMessageHandlers();
    }
  }
}

#if EZ_ENABLED(EZ_COMPILE_FOR_DEBUG)

static bool IsValidIdentifierName(ezStringView sIdentifier)
{
  // empty strings are not valid
  if (sIdentifier.IsEmpty())
    return false;

  // digits are not allowed as the first character
  ezUInt32 uiChar = sIdentifier.GetCharacter();
  if (uiChar >= '0' && uiChar <= '9')
    return false;

  for (auto it = sIdentifier.GetIteratorFront(); it.IsValid(); ++it)
  {
    const ezUInt32 c = it.GetCharacter();

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

#endif

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
        EZ_IGNORE_UNUSED(pSpecificType);
        EZ_ASSERT_DEV(pSpecificType->GetTypeFlags().IsSet(ezTypeFlags::StandardType), "Only standard type constants are supported!");
      }
      break;
      case ezPropertyCategory::Member:
      {
        EZ_ASSERT_DEV(pProp->GetFlags().IsSet(ezPropertyFlags::StandardType) == pSpecificType->GetTypeFlags().IsSet(ezTypeFlags::StandardType),
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
        EZ_ASSERT_DEV(pProp->GetFlags().IsSet(ezPropertyFlags::StandardType) == pSpecificType->GetTypeFlags().IsSet(ezTypeFlags::StandardType),
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

void ezRTTI::PluginEventHandler(const ezPluginEvent& EventData)
{
  switch (EventData.m_EventType)
  {
    case ezPluginEvent::BeforeLoading:
    {
      // before a new plugin is loaded, make sure all current ezRTTI instances
      // are assigned to the proper plugin
      // all not-yet assigned rtti instances cannot be in any plugin, so assign them to the 'static' plugin
      AssignPlugin("Static");
    }
    break;

    case ezPluginEvent::AfterLoadingBeforeInit:
    {
      // after we loaded a new plugin, but before it is initialized,
      // find all new rtti instances and assign them to that new plugin
      AssignPlugin(EventData.m_sPluginBinary);

#if EZ_ENABLED(EZ_COMPILE_FOR_DEBUG)
      ezRTTI::VerifyCorrectnessForAllTypes();
#endif
    }
    break;

    default:
      break;
  }
}

ezRTTIAllocator::~ezRTTIAllocator() = default;


EZ_STATICLINK_FILE(Foundation, Foundation_Reflection_Implementation_RTTI);
