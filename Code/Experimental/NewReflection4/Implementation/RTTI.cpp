#include "RTTI.h"
#include "AbstractProperty.h"
#include "DynamicRTTI.h"
#include "MessageHandler.h"
#include <Foundation/Configuration/Startup.h>

#include <set>
#include <string>

EZ_ENUMERABLE_CLASS_IMPLEMENTATION(ezRTTI);

EZ_BEGIN_SUBSYSTEM_DECLARATION(Foundation, Reflection)

  //BEGIN_SUBSYSTEM_DEPENDENCIES
  //  "FileSystem"
  //END_SUBSYSTEM_DEPENDENCIES

  ON_BASE_STARTUP
  {
    ezPlugin::s_PluginEvents.AddEventHandler(ezRTTI::PluginEventHandler);
    ezRTTI::AssignPlugin("Static");
  }

  ON_BASE_SHUTDOWN
  {
    ezPlugin::s_PluginEvents.RemoveEventHandler(ezRTTI::PluginEventHandler);
  }

EZ_END_SUBSYSTEM_DECLARATION

ezRTTI::ezRTTI(const char* szName, const ezRTTI* pParentType, ezUInt32 uiTypeSize, ezRTTIAllocator* pAllocator, ezArrayPtr<ezAbstractProperty*> pProperties, ezArrayPtr<ezAbstractMessageHandler*> pMessageHandlers)
{
  m_szPluginName = NULL;
  m_szTypeName = szName;
  m_pParentType = pParentType;
  m_pAllocator = pAllocator;
  m_Properties = pProperties;
  m_uiTypeSize = uiTypeSize;
  m_MessageHandlers = pMessageHandlers;

#if EZ_ENABLED(EZ_COMPILE_FOR_DEVELOPMENT)
  {
    // cannot use ezSet etc. this runs at startup, before allocators are initialized
    std::set<std::string> Known;

    const ezRTTI* pInstance = this;

    while (pInstance != NULL)
    {
      for (ezUInt32 i = 0; i < pInstance->m_Properties.GetCount(); ++i)
      {
        const bool bNewProperty = Known.insert(pInstance->m_Properties[i]->GetPropertyName()).second;
        EZ_ASSERT(bNewProperty, "%s: The property with name '%s' is already defined in type '%s'.", szName, pInstance->m_Properties[i]->GetPropertyName(), pInstance->GetTypeName());
      }

      pInstance = pInstance->m_pParentType;
    }
  }
#endif
}

bool ezRTTI::IsDerivedFrom(const ezRTTI* pBaseType) const
{
  const ezRTTI* pThis = this;

  while (pThis != NULL)
  {
    if (pThis == pBaseType)
      return true;

    pThis = pThis->m_pParentType;
  }

  return false;
}

bool ezRTTI::HandleMessageOfType(void* pInstance, ezMessageId id, ezMessage* pMsg, bool bSearchBaseTypes /* = true */) const
{
  ezAbstractMessageHandler* pHandler = FindMessageHandler(id, bSearchBaseTypes);

  if (pHandler)
  {
    pHandler->HandleMessage(pInstance, pMsg);
    return true;
  }

  return false;
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

  return NULL;
}

ezAbstractProperty* ezRTTI::FindPropertyByName(const char* szName, bool bSearchBaseTypes /* = true */) const
{
  const ezRTTI* pInstance = this;

  do
  {
    for (ezUInt32 p = 0; p < m_Properties.GetCount(); ++p)
    {
      if (ezStringUtils::IsEqual(m_Properties[p]->GetPropertyName(), szName))
      {
        return m_Properties[p];
      }
    }

    if (!bSearchBaseTypes)
      return NULL;

    pInstance = pInstance->m_pParentType;
  }
  while (pInstance != NULL);

  return NULL;
}

ezAbstractMessageHandler* ezRTTI::FindMessageHandler(ezMessageId id, bool bSearchBaseTypes /* = true */) const
{
  const ezRTTI* pInstance = this;

  do
  {
    for (ezUInt32 p = 0; p < m_MessageHandlers.GetCount(); ++p)
    {
      if (m_MessageHandlers[p]->GetMessageTypeID() == id)
      {
        return m_MessageHandlers[p];
      }
    }

    if (!bSearchBaseTypes)
      return NULL;

    pInstance = pInstance->m_pParentType;
  }
  while (pInstance != NULL);

  return NULL;
}

void ezRTTI::AssignPlugin(const char* szPluginName)
{
  // assigns the given plugin name to every ezRTTI instance that has no plugin assigned yet

  ezRTTI* pInstance = ezRTTI::GetFirstInstance();

  while (pInstance)
  {
    if (pInstance->m_szPluginName == NULL)
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

EZ_BEGIN_REFLECTED_TYPE(ezReflectedClass, ezNoBase, ezRTTINoAllocator);
EZ_END_REFLECTED_TYPE(ezReflectedClass);