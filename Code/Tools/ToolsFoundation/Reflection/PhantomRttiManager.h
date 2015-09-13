#pragma once

#include <ToolsFoundation/Reflection/ReflectedType.h>
#include <Foundation/Communication/Event.h>
#include <Foundation/Containers/IdTable.h>

class ezPhantomRTTI;

/// \brief Manages all ezPhantomRTTI types that have been added to him.
///
/// A ezPhantomRTTI cannot be created directly but must be created via this managers
/// RegisterType function with a given ezReflectedTypeDescriptor.
class EZ_TOOLSFOUNDATION_DLL ezPhantomRttiManager
{
public:
  /// \brief Adds a reflected type to the list of accessible types.
  ///
  /// Types must be added in the correct order, any type must be added before
  /// it can be referenced in other types. Any base class must be added before
  /// any class deriving from it can be added.
  /// Call the function again if a type has changed during the run of the
  /// program. If the type actually differs the last known class layout the
  /// m_TypeChangedEvent event will be called with the old and new ezRTTI.
  ///
  /// \sa ezReflectionUtils::GetReflectedTypeDescriptorFromRtti
  static const ezRTTI* RegisterType(const ezReflectedTypeDescriptor& desc);

  /// \brief Removes a type from the list of accessible types.
  ///
  /// No instance of the given type or storage must still exist when this function is called.
  static bool UnregisterType(const ezRTTI* pRtti);

private:
  EZ_MAKE_SUBSYSTEM_STARTUP_FRIEND(ToolsFoundation, ReflectedTypeManager);

  static void Startup();
  static void Shutdown();

public:
  struct Event
  {
    enum class Type
    {
      TypeAdded,
      TypeRemoved,
      TypeChanged,
    };

    Event() : m_Type(Type::TypeAdded), m_pChangedType(nullptr) {}

    Type m_Type;
    const ezRTTI* m_pChangedType;
  };

  static ezEvent<const Event&> m_Events;

private:
  static ezSet<const ezRTTI*> m_RegisteredConcreteTypes;
  static ezHashTable<const char*, ezPhantomRTTI*> m_NameToPhantom;
};

