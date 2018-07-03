#include <PCH.h>

#include <Foundation/Configuration/Singleton.h>

ezMap<ezString, void*> ezSingletonRegistry::s_Singletons;

void* ezSingletonRegistry::GetSingletonInstance(const char* szSingletonClassType)
{
  return s_Singletons.GetValueOrDefault(szSingletonClassType, nullptr);
}

const ezMap<ezString, void*>& ezSingletonRegistry::GetAllRegisteredSingletons()
{
  return s_Singletons;
}

void ezSingletonRegistry::Register(void* pSingletonInstance, const char* szTypeName)
{
  EZ_ASSERT_DEV(pSingletonInstance != nullptr, "Invalid singleton instance pointer");
  EZ_ASSERT_DEV(s_Singletons[szTypeName] == nullptr, "Singleton for type '{0}' has already been registered", szTypeName);

  s_Singletons[szTypeName] = pSingletonInstance;
}

void ezSingletonRegistry::Unregister(const char* szTypeName)
{
  EZ_ASSERT_DEV(s_Singletons[szTypeName] != nullptr, "Singleton for type '{0}' is currently not registered", szTypeName);

  s_Singletons.Remove(szTypeName);
}



EZ_STATICLINK_FILE(Foundation, Foundation_Configuration_Implementation_Singleton);
