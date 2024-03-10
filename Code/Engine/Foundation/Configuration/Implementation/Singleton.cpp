#include <Foundation/FoundationPCH.h>

#include <Foundation/Configuration/Singleton.h>

ezMap<size_t, ezSingletonRegistry::SingletonEntry> ezSingletonRegistry::s_Singletons;

const ezMap<size_t, ezSingletonRegistry::SingletonEntry>& ezSingletonRegistry::GetAllRegisteredSingletons()
{
  return s_Singletons;
}
