#pragma once

#include <CoreUtils/Basics.h>
#include <Foundation/Containers/HashTable.h>
#include <Foundation/Communication/Event.h>

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

  //class DataReadProxy
  //{
  //public:

  //  const VALUE* operator->() const { return m_pData; }

  //private:
  //  ezObjectMetaData* m_pOwner;
  //  const VALUE* m_pData;
  //};

  ezObjectMetaData();

  bool HasMetaData(const KEY ObjectKey) const;

  void ClearMetaData(const KEY ObjectKey);

  const VALUE* BeginReadMetaData(const KEY ObjectKey) const;
  void EndReadMetaData() const;

  VALUE* BeginModifyMetaData(const KEY ObjectKey);
  void EndModifyMetaData(ezUInt32 uiModifiedFlags = 0xFFFFFFFF);

  ezEvent<const EventData&> m_DataModifiedEvent;

private:
  VALUE m_DefaultValue;
  mutable enum class AccessMode { Nothing, Read, Write } m_AccessMode;
  mutable KEY m_AcessingKey;
  mutable ezNoMutex m_Mutex;
  ezHashTable<KEY, VALUE> m_MetaData;
};

#include <CoreUtils/DataStructures/Implementation/ObjectMetaData_inl.h>


