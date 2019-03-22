#include <FoundationPCH.h>

#include <Foundation/Configuration/Singleton.h>

ezMap<size_t, ezSingletonRegistry::SingletonEntry> ezSingletonRegistry::s_Singletons;

const ezMap<size_t, ezSingletonRegistry::SingletonEntry>& ezSingletonRegistry::GetAllRegisteredSingletons()
{
  return s_Singletons;
}

EZ_STATICLINK_FILE(Foundation, Foundation_Configuration_Implementation_Singleton);

