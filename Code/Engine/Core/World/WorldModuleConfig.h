#pragma once

#include <Foundation/Basics.h>
#include <Foundation/Reflection/Reflection.h>
#include <Foundation/Strings/String.h>

/// \brief Configuration class for world module interface implementations.
///
/// Manages the mapping between world module interfaces and their specific implementations.
/// This is used when multiple implementations exist for the same interface, allowing
/// configuration of which implementation should be used by default.
class EZ_CORE_DLL ezWorldModuleConfig
{
public:
  ezResult Save();
  void Load();

  /// \brief Applies the current configuration to the world module factory.
  void Apply();

  /// \brief Adds a mapping from an interface to a specific implementation.
  void AddInterfaceImplementation(ezStringView sInterfaceName, ezStringView sImplementationName);

  /// \brief Removes the implementation mapping for the given interface.
  void RemoveInterfaceImplementation(ezStringView sInterfaceName);

  /// \brief Represents a mapping between an interface and its implementation.
  struct InterfaceImpl
  {
    ezString m_sInterfaceName;      ///< Name of the world module interface
    ezString m_sImplementationName; ///< Name of the specific implementation to use

    bool operator<(const InterfaceImpl& rhs) const { return m_sInterfaceName < rhs.m_sInterfaceName; }
  };

  ezHybridArray<InterfaceImpl, 8> m_InterfaceImpls; ///< List of interface to implementation mappings
};
