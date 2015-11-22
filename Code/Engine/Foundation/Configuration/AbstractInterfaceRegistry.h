#pragma once

#include <Foundation/Containers/Map.h>
#include <Foundation/Strings/String.h>

/// \todo Document + Test

class EZ_FOUNDATION_DLL ezAbstractInterfaceRegistry
{
public:

  static void RegisterInterfaceImplementation(const char* szName, void* pInterface);

  static void UnregisterInterfaceImplementation(const char* szName, void* pInterface);

  static bool IsInterfaceImplementationRegistered(const char* szName);

  static void* RetrieveImplementationForInterface(const char* szName);

  template<typename Interface>
  static Interface* RetrieveImplementationForInterface(const char* szName)
  {
    return static_cast<Interface*>(RetrieveImplementationForInterface(szName));
  }

private:
  static ezMap<ezString, void*> s_Interfaces;
};