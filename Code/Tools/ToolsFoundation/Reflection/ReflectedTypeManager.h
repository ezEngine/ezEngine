#pragma once

#include <ToolsFoundation/Reflection/ReflectedType.h>
#include <Foundation/Communication/Event.h>
#include <Foundation/Containers/IdTable.h>

typedef ezIdTable<ezReflectedTypeId, ezReflectedType*> ReflectedTypeTable;

/// \brief Manages all reflected types that have been added to him.
///
/// An ezReflectedType cannot be created directly but must be created via this managers
/// RegisterType function with a given ezReflectedTypeDescriptor.
class EZ_TOOLSFOUNDATION_DLL ezReflectedTypeManager
{
public:
  /// \brief Adds a reflected type to the list of accessible types.
  ///
  /// Types must be added in the correct order, any type must be added before
  /// it can be referenced in other types. Any base class must be added before
  /// any class deriving from it can be added.
  /// Call the function again if a type has changed during the run of the
  /// program. If the type actually differs the last known class layout the
  /// m_TypeChangedEvent event will be called with the old and new ezReflectedType.
  ///
  /// \sa ezReflectionUtils::GetReflectedTypeDescriptorFromRtti
  static ezReflectedTypeHandle RegisterType(const ezReflectedTypeDescriptor& desc);

  /// \brief Removes a type from the list of accessible types.
  ///
  /// No instance of the given type or storage must still exist when this function is called.
  static bool UnregisterType(ezReflectedTypeHandle hType);

  /// \brief Returns the number of accessible types handled by this class.
  static ezUInt32 GetTypeCount() { return m_Types.GetCount(); } // [tested]

  /// \brief Returns an iterator to the type list.
  static ReflectedTypeTable::ConstIterator GetTypeIterator() { return m_Types.GetIterator(); }

  /// \brief Returns the type handle at with the given name.
  static ezReflectedTypeHandle GetTypeHandleByName(const char* szName); // [tested]

  /// \brief Resolves the given handle to its corresponding ezReflectedType.
  ///
  /// \param hType
  ///  The reflected type handle to be resolved into the actual instance.
  ///
  /// \return
  ///   nullptr if the handle is invalid, the corresponding ezReflectedType otherwise.
  static const ezReflectedType* GetType(ezReflectedTypeHandle hType); // [tested]

private:
  EZ_MAKE_SUBSYSTEM_STARTUP_FRIEND(Core, ReflectedTypeManager);

  static ezReflectedType* GetTypeInternal(ezReflectedTypeHandle hType);
  static void Startup();
  static void Shutdown();

public:
  static ezEvent<ezReflectedTypeChange&> m_TypeAddedEvent;
  static ezEvent<ezReflectedTypeChange&> m_TypeChangedEvent;
  static ezEvent<ezReflectedTypeChange&> m_TypeRemovedEvent;

private:
  static ReflectedTypeTable m_Types;
  static ezHashTable<const char*, ezReflectedTypeHandle> m_NameToHandle;
};

