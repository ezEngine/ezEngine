#pragma once

#include <CoreUtils/Basics.h>
#include <Foundation/Containers/HashTable.h>
#include <Foundation/Communication/Event.h>
#include <Foundation/Serialization/AbstractObjectGraph.h>
#include <Foundation/Reflection/ReflectionUtils.h>

template<typename KEY, typename VALUE>
class ezObjectMetaData
{
public:

  struct EventData
  {
    KEY m_ObjectKey;
    const VALUE* m_pValue;
    ezUInt32 m_uiModifiedFlags;
  };

  ezObjectMetaData();

  bool HasMetaData(const KEY ObjectKey) const;

  void ClearMetaData(const KEY ObjectKey);

  const VALUE* BeginReadMetaData(const KEY ObjectKey) const;
  void EndReadMetaData() const;

  VALUE* BeginModifyMetaData(const KEY ObjectKey);
  void EndModifyMetaData(ezUInt32 uiModifiedFlags = 0xFFFFFFFF);

  ezEvent<const EventData&> m_DataModifiedEvent;

  ezMutex& GetMutex() const { return m_Mutex; }

  const VALUE& GetDefaultValue() const { return m_DefaultValue; }

  /// \brief Uses reflection information from VALUE to store all properties that differ from the default value as additional properties for the graph objects.
  void AttachMetaDataToAbstractGraph(ezAbstractObjectGraph& graph);

  /// \brief Uses reflection information from VALUE to restore all meta data properties from the graph.
  void RestoreMetaDataFromAbstractGraph(const ezAbstractObjectGraph& graph);

private:
  VALUE m_DefaultValue;
  mutable enum class AccessMode { Nothing, Read, Write } m_AccessMode;
  mutable KEY m_AcessingKey;
  mutable ezMutex m_Mutex;
  ezHashTable<KEY, VALUE> m_MetaData;
};

#include <CoreUtils/DataStructures/Implementation/ObjectMetaData_inl.h>


