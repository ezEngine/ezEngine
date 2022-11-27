#pragma once

#include <Foundation/Communication/Event.h>
#include <Foundation/Containers/HashTable.h>
#include <Foundation/Reflection/ReflectionUtils.h>
#include <Foundation/Serialization/AbstractObjectGraph.h>
#include <Foundation/Types/RefCounted.h>
#include <Foundation/Types/SharedPtr.h>
#include <ToolsFoundation/ToolsFoundationDLL.h>

/// \brief Stores meta data for document objects that is not part of the object itself. E.g. editor-only states like hidden or prefab information.
/// \tparam KEY The key under which data is stored. Usually ezUuid to reference document objects.
/// \tparam VALUE Meta value type to be stored.
template <typename KEY, typename VALUE>
class ezObjectMetaData
{
public:
  struct EventData
  {
    KEY m_ObjectKey;
    const VALUE* m_pValue;
    ezUInt32 m_uiModifiedFlags;
  };

  ezEvent<const EventData&> m_DataModifiedEvent;

  // \brief Storage for the meta data so it can be swapped when using multiple sub documents.
  class Storage : public ezRefCounted
  {
  public:
    mutable enum class AccessMode { Nothing,
      Read,
      Write } m_AccessMode;
    mutable KEY m_AcessingKey;
    mutable ezMutex m_Mutex;
    ezHashTable<KEY, VALUE> m_MetaData;

    ezEvent<const EventData&> m_DataModifiedEvent;
  };

  ezObjectMetaData();

  bool HasMetaData(const KEY objectKey) const;

  void ClearMetaData(const KEY objectKey);

  /// \brief Will always return a non-null result. May be a default object.
  const VALUE* BeginReadMetaData(const KEY objectKey) const;
  void EndReadMetaData() const;

  VALUE* BeginModifyMetaData(const KEY objectKey);
  void EndModifyMetaData(ezUInt32 uiModifiedFlags = 0xFFFFFFFF);


  ezMutex& GetMutex() const { return m_pMetaStorage->m_Mutex; }

  const VALUE& GetDefaultValue() const { return m_DefaultValue; }

  /// \brief Uses reflection information from VALUE to store all properties that differ from the default value as additional properties for the graph
  /// objects.
  void AttachMetaDataToAbstractGraph(ezAbstractObjectGraph& inout_graph) const;

  /// \brief Uses reflection information from VALUE to restore all meta data properties from the graph.
  void RestoreMetaDataFromAbstractGraph(const ezAbstractObjectGraph& graph);

  ezSharedPtr<ezObjectMetaData<KEY, VALUE>::Storage> SwapStorage(ezSharedPtr<ezObjectMetaData<KEY, VALUE>::Storage> pNewStorage);
  ezSharedPtr<ezObjectMetaData<KEY, VALUE>::Storage> GetStorage() { return m_pMetaStorage; }

private:
  VALUE m_DefaultValue;
  ezSharedPtr<ezObjectMetaData<KEY, VALUE>::Storage> m_pMetaStorage;
  typename ezEvent<const EventData&>::Unsubscriber m_EventsUnsubscriber;
};

#include <ToolsFoundation/Object/Implementation/ObjectMetaData_inl.h>
