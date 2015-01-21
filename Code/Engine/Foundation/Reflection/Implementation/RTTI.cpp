#include <Foundation/PCH.h>
#include <Foundation/Reflection/Implementation/RTTI.h>
#include <Foundation/Reflection/Implementation/AbstractProperty.h>
#include <Foundation/Reflection/Implementation/DynamicRTTI.h>
#include <Foundation/Reflection/Implementation/MessageHandler.h>
#include <Foundation/Configuration/Startup.h>
#include <Foundation/Containers/Set.h>
#include <Foundation/Strings/String.h>

EZ_ENUMERABLE_CLASS_IMPLEMENTATION(ezRTTI);

EZ_BEGIN_SUBSYSTEM_DECLARATION(Foundation, Reflection)

  //BEGIN_SUBSYSTEM_DEPENDENCIES
  //  "FileSystem"
  //END_SUBSYSTEM_DEPENDENCIES

  ON_CORE_STARTUP
  {
    ezPlugin::s_PluginEvents.AddEventHandler(ezRTTI::PluginEventHandler);
    ezRTTI::AssignPlugin("Static");
  }

  ON_CORE_SHUTDOWN
  {
    ezPlugin::s_PluginEvents.RemoveEventHandler(ezRTTI::PluginEventHandler);
  }

EZ_END_SUBSYSTEM_DECLARATION

ezRTTI::ezRTTI(const char* szName, const ezRTTI* pParentType, ezUInt32 uiTypeSize, ezUInt32 uiTypeVersion, ezUInt32 uiVariantType,
  ezRTTIAllocator* pAllocator, ezArrayPtr<ezAbstractProperty*> properties, ezArrayPtr<ezAbstractMessageHandler*> messageHandlers)
{
  m_szPluginName = nullptr;
  m_szTypeName = szName;
  m_pParentType = pParentType;
  m_uiVariantType = uiVariantType;
  m_uiTypeSize = uiTypeSize;
  m_uiTypeVersion = uiTypeVersion;
  m_pAllocator = pAllocator;
  m_Properties = properties;
  m_MessageHandlers = messageHandlers;  

#if EZ_ENABLED(EZ_COMPILE_FOR_DEVELOPMENT)
  {
    // cannot use ezSet etc. this runs at startup, before allocators are initialized
    ezSet<ezString> Known;

    const ezRTTI* pInstance = this;

    while (pInstance != nullptr)
    {
      for (ezUInt32 i = 0; i < pInstance->m_Properties.GetCount(); ++i)
      {
        const bool bNewProperty = !Known.Find(pInstance->m_Properties[i]->GetPropertyName()).IsValid();
        Known.Insert(pInstance->m_Properties[i]->GetPropertyName());

        EZ_ASSERT_DEV(bNewProperty, "%s: The property with name '%s' is already defined in type '%s'.", szName, pInstance->m_Properties[i]->GetPropertyName(), pInstance->GetTypeName());
      }

      pInstance = pInstance->m_pParentType;
    }
  }
#endif

  {
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
          
          if (m_DynamicMessageHandlers[uiIndex] == nullptr)
          {
            m_DynamicMessageHandlers[uiIndex] = pHandler;
          }
        }

        pInstance = pInstance->m_pParentType;
      }
    }
  }
}

ezRTTI::~ezRTTI()
{
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
  ezRTTI* pInstance = ezRTTI::GetFirstInstance();

  while (pInstance)
  {
    if (ezStringUtils::IsEqual(pInstance->GetTypeName(), szName))
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
  }
  while (pInstance != nullptr);

  return nullptr;
}

bool ezRTTI::DispatchMessage(void* pInstance, ezMessage& msg) const
{
  const ezUInt32 uiIndex = msg.GetId() - m_uiMsgIdOffset;

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
  const ezUInt32 uiIndex = msg.GetId() - m_uiMsgIdOffset;

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
      pInstance->m_szPluginName = szPluginName;

    pInstance = pInstance->GetNextInstance();
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
    }
    break;

  default:
    break;
  }
}



EZ_STATICLINK_FILE(Foundation, Foundation_Reflection_Implementation_RTTI);

