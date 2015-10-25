#pragma once

template<typename KEY, typename VALUE>
ezObjectMetaData<KEY, VALUE>::ezObjectMetaData()
{
  m_AcessingKey = KEY();
  m_AccessMode = AccessMode::Nothing;
  m_DefaultValue = VALUE();
}

template<typename KEY, typename VALUE>
const VALUE* ezObjectMetaData<KEY, VALUE>::BeginReadMetaData(const KEY ObjectKey) const
{
  m_Mutex.Acquire();
  EZ_ASSERT_DEV(m_AccessMode == AccessMode::Nothing, "Already accessing some data");
  m_AccessMode = AccessMode::Read;
  m_AcessingKey = ObjectKey;

  VALUE* pRes = nullptr;
  if (m_MetaData.TryGetValue(ObjectKey, pRes)) // TryGetValue is not const correct with the second parameter
    return pRes;

  return &m_DefaultValue;
}

template<typename KEY, typename VALUE>
void ezObjectMetaData<KEY, VALUE>::ClearMetaData(const KEY ObjectKey)
{
  EZ_LOCK(m_Mutex);
  EZ_ASSERT_DEV(m_AccessMode == AccessMode::Nothing, "Already accessing some data");

  if (HasMetaData(ObjectKey))
  {
    m_MetaData.Remove(ObjectKey);

    EventData e;
    e.m_ObjectKey = ObjectKey;
    e.m_pValue = &m_DefaultValue;

    m_DataModifiedEvent.Broadcast(e);
  }
}

template<typename KEY, typename VALUE>
bool ezObjectMetaData<KEY, VALUE>::HasMetaData(const KEY ObjectKey) const
{
  EZ_LOCK(m_Mutex);
  VALUE* pValue = nullptr;
  return m_MetaData.TryGetValue(ObjectKey, pValue);
}

template<typename KEY, typename VALUE>
VALUE* ezObjectMetaData<KEY, VALUE>::BeginModifyMetaData(const KEY ObjectKey)
{
  m_Mutex.Acquire();
  EZ_ASSERT_DEV(m_AccessMode == AccessMode::Nothing, "Already accessing some data");
  m_AccessMode = AccessMode::Write;
  m_AcessingKey = ObjectKey;

  return &m_MetaData[ObjectKey];
}

template<typename KEY, typename VALUE>
void ezObjectMetaData<KEY, VALUE>::EndReadMetaData() const
{
  EZ_ASSERT_DEV(m_AccessMode == AccessMode::Read, "Not accessing data at the moment");
  
  m_AccessMode = AccessMode::Nothing;
  m_Mutex.Release();
}


template<typename KEY, typename VALUE>
void ezObjectMetaData<KEY, VALUE>::EndModifyMetaData(ezUInt32 uiModifiedFlags /*= 0xFFFFFFFF*/)
{
  EZ_ASSERT_DEV(m_AccessMode == AccessMode::Write, "Not accessing data at the moment");
  m_AccessMode = AccessMode::Nothing;

  if (uiModifiedFlags != 0)
  {
    EventData e;
    e.m_ObjectKey = m_AcessingKey;
    e.m_pValue = &m_MetaData[m_AcessingKey];
    e.m_uiModifiedFlags = uiModifiedFlags;

    m_DataModifiedEvent.Broadcast(e);
  }
  
  m_Mutex.Release();
}

