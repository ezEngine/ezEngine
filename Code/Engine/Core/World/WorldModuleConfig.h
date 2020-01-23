#pragma once

#include <Foundation/Basics.h>
#include <Foundation/Reflection/Reflection.h>
#include <Foundation/Strings/String.h>

class EZ_CORE_DLL ezWorldModuleConfig
{
public:
  ezResult Save();
  void Load();
  void Apply();

  void AddInterfaceImplementation(ezStringView sInterfaceName, ezStringView sImplementationName);
  void RemoveInterfaceImplementation(ezStringView sInterfaceName);

  struct InterfaceImpl
  {
    ezString m_sInterfaceName;
    ezString m_sImplementationName;

    bool operator<(const InterfaceImpl& rhs) const { return m_sInterfaceName < rhs.m_sInterfaceName; }
  };

  ezHybridArray<InterfaceImpl, 8> m_InterfaceImpls;
};
