#include <Foundation/PCH.h>
#include <Foundation/Configuration/AbstractInterfaceRegistry.h>

ezMap<ezString, void*> ezAbstractInterfaceRegistry::s_Interfaces;

void ezAbstractInterfaceRegistry::RegisterInterfaceImplementation(const char* szName, void* pInterface)
{
  EZ_ASSERT_DEV(!IsInterfaceImplementationRegistered(szName), "There is already an implementation registered for interface '%s'", szName);

  s_Interfaces[szName] = pInterface;
}


void ezAbstractInterfaceRegistry::UnregisterInterfaceImplementation(const char* szName, void* pInterface)
{
  EZ_ASSERT_DEV(IsInterfaceImplementationRegistered(szName), "There is no implementation registered for interface '%s'", szName);
  EZ_ASSERT_DEV(s_Interfaces[szName] == pInterface, "Interface to be unregistered has incorrect implementation pointer");

  s_Interfaces.Remove(szName);
}

bool ezAbstractInterfaceRegistry::IsInterfaceImplementationRegistered(const char* szName)
{
  return s_Interfaces.Contains(szName);
}

void* ezAbstractInterfaceRegistry::RetrieveImplementationForInterface(const char* szName)
{
  EZ_ASSERT_DEV(IsInterfaceImplementationRegistered(szName), "There is no implementation registered for interface '%s'", szName);

  return s_Interfaces[szName];
}


