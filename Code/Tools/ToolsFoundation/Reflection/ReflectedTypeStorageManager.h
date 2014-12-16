#pragma once

#include <ToolsFoundation/Reflection/ReflectedType.h>
#include <Foundation/Containers/Set.h>

class ezReflectedTypeStorageAccessor;

/// \brief Manages all ezReflectedTypeStorageAccessor instances.
///
/// This class takes care of patching all ezReflectedTypeStorageAccessor instances when their
/// ezReflectedType is modified. It also provides the mapping from ezPropertyPath to the data
/// storage index of the corresponding ezVariant in the ezReflectedTypeStorageAccessor.
class EZ_TOOLSFOUNDATION_DLL ezReflectedTypeStorageManager
{
public:
  ezReflectedTypeStorageManager();

private:
  struct ReflectedTypeStorageMapping
  {
    struct StorageInfo
    {
      StorageInfo() : m_uiIndex(0), m_Type(ezVariant::Type::Invalid) {}
      StorageInfo(ezUInt16 uiIndex, ezVariant::Type::Enum type) : m_uiIndex(uiIndex), m_Type(type) {}

      ezUInt16 m_uiIndex;
      ezEnum<ezVariant::Type> m_Type;
    };

    /// \brief Flattens all POD type properties of the given ezReflectedType into m_PathToStorageInfoTable.
    ///
    /// The functions first adds all parent class properties and then adds its own properties.
    /// POD type properties are added under the current path and non-PODs are recursed into with a new path.
    void AddProperties(const ezReflectedType* pType);
    void AddPropertiesRecursive(const ezReflectedType* pType, const char* szPath);

    void UpdateInstances(ezUInt32 uiIndex, const ezReflectedProperty* pProperty);
    void AddPropertyToInstances(ezUInt32 uiIndex, const ezReflectedProperty* pProperty);


    ezSet<ezReflectedTypeStorageAccessor*> m_Instances;
    ezHashTable<ezString, StorageInfo> m_PathToStorageInfoTable;
  };

  EZ_MAKE_SUBSYSTEM_STARTUP_FRIEND(Core, ReflectedTypeStorageManager);
  friend class ezReflectedTypeStorageAccessor;

  static void Startup();
  static void Shutdown();

  static const ReflectedTypeStorageMapping* AddStorageAccessor(ezReflectedTypeStorageAccessor* pInstance);
  static void RemoveStorageAccessor(ezReflectedTypeStorageAccessor* pInstance);

  static void TypeAddedEvent(ezReflectedTypeChange& data);
  static void TypeChangedEvent(ezReflectedTypeChange& data);
  static void TypeRemovedEvent(ezReflectedTypeChange& data);

private:
  static ezMap<ezReflectedTypeHandle, ReflectedTypeStorageMapping*> m_ReflectedTypeToStorageMapping;
};

