#pragma once

#include <ToolsFoundation/Reflection/PhantomRttiManager.h>
#include <Foundation/Containers/Set.h>

class ezReflectedTypeStorageAccessor;
class ezDocumentObject;

/// \brief Manages all ezReflectedTypeStorageAccessor instances.
///
/// This class takes care of patching all ezReflectedTypeStorageAccessor instances when their
/// ezRTTI is modified. It also provides the mapping from property name to the data
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
      StorageInfo(ezUInt16 uiIndex, ezVariant::Type::Enum type, const ezVariant& defaultValue)
        : m_uiIndex(uiIndex), m_Type(type), m_DefaultValue(defaultValue) {}

      ezUInt16 m_uiIndex;
      ezEnum<ezVariant::Type> m_Type;
      ezVariant m_DefaultValue;
    };

    /// \brief Flattens all POD type properties of the given ezRTTI into m_PathToStorageInfoTable.
    ///
    /// The functions first adds all parent class properties and then adds its own properties.
    /// POD type properties are added under the current path.
    void AddProperties(const ezRTTI* pType);
    void AddPropertiesRecursive(const ezRTTI* pType, ezSet<const ezDocumentObject*>& requiresPatchingEmbeddedClass);

    void UpdateInstances(ezUInt32 uiIndex, const ezAbstractProperty* pProperty, ezSet<const ezDocumentObject*>& requiresPatchingEmbeddedClass);
    void AddPropertyToInstances(ezUInt32 uiIndex, const ezAbstractProperty* pProperty, ezSet<const ezDocumentObject*>& requiresPatchingEmbeddedClass);

    ezVariantType::Enum GetStorageType(const ezAbstractProperty* pProperty);

    ezSet<ezReflectedTypeStorageAccessor*> m_Instances;
    ezHashTable<ezString, StorageInfo> m_PathToStorageInfoTable;
  };

  EZ_MAKE_SUBSYSTEM_STARTUP_FRIEND(ToolsFoundation, ReflectedTypeStorageManager);
  friend class ezReflectedTypeStorageAccessor;

  static void Startup();
  static void Shutdown();

  static const ReflectedTypeStorageMapping* AddStorageAccessor(ezReflectedTypeStorageAccessor* pInstance);
  static void RemoveStorageAccessor(ezReflectedTypeStorageAccessor* pInstance);

  static ReflectedTypeStorageMapping* GetTypeStorageMapping(const ezRTTI* pType);
  static void TypeEventHandler(const ezPhantomRttiManagerEvent& e);

private:
  static ezMap<const ezRTTI*, ReflectedTypeStorageMapping*> m_ReflectedTypeToStorageMapping;
};

