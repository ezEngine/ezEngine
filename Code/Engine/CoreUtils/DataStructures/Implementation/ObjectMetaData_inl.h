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


template<typename KEY, typename VALUE>
void ezObjectMetaData<KEY, VALUE>::AttachMetaDataToAbstractGraph(ezAbstractObjectGraph& graph)
{
  auto& AllNodes = graph.GetAllNodes();

  EZ_LOCK(m_Mutex);

  ezHashTable<const char*, ezVariant> DefaultValues;

  // store the default values in an easily accessible hash map, to be able to compare against them
  {
    DefaultValues.Reserve(m_DefaultValue.GetDynamicRTTI()->GetProperties().GetCount());

    for (const auto& pProp : m_DefaultValue.GetDynamicRTTI()->GetProperties())
    {
      if (pProp->GetCategory() != ezPropertyCategory::Member)
        continue;

      const ezAbstractMemberProperty* pDefVal = static_cast<ezAbstractMemberProperty*>(pProp);

      DefaultValues[pProp->GetPropertyName()] = ezReflectionUtils::GetMemberPropertyValue(static_cast<ezAbstractMemberProperty*>(pProp), &m_DefaultValue);
    }
  }

  // now serialize all properties that differ from the default value
  {
    ezVariant value;

    for (auto it = AllNodes.GetIterator(); it.IsValid(); ++it)
    {
      auto* pNode = it.Value();
      const ezUuid& guid = pNode->GetGuid();

      VALUE* pMeta = nullptr;
      if (!m_MetaData.TryGetValue(guid, pMeta)) // TryGetValue is not const correct with the second parameter
        continue; // it is the default object, so all values are default -> skip

      for (const auto& pProp : pMeta->GetDynamicRTTI()->GetProperties())
      {
        if (pProp->GetCategory() != ezPropertyCategory::Member)
          continue;

        value = ezReflectionUtils::GetMemberPropertyValue(static_cast<ezAbstractMemberProperty*>(pProp), pMeta);

        if (value.IsValid() && DefaultValues[pProp->GetPropertyName()] != value)
        {
          pNode->AddProperty(pProp->GetPropertyName(), value);
        }

      }
    }
  }
}


template<typename KEY, typename VALUE>
void ezObjectMetaData<KEY, VALUE>::RestoreMetaDataFromAbstractGraph(const ezAbstractObjectGraph& graph)
{
  EZ_LOCK(m_Mutex);

  ezHybridArray<ezString, 16> PropertyNames;

  // find all properties (names) that we want to read
  {
    for (const auto& pProp : m_DefaultValue.GetDynamicRTTI()->GetProperties())
    {
      if (pProp->GetCategory() != ezPropertyCategory::Member)
        continue;

      PropertyNames.PushBack(pProp->GetPropertyName());
    }
  }

  auto& AllNodes = graph.GetAllNodes();

  for (auto it = AllNodes.GetIterator(); it.IsValid(); ++it)
  {
    auto* pNode = it.Value();
    const ezUuid& guid = pNode->GetGuid();

    for (const auto& name : PropertyNames)
    {
      if (const auto* pProp = pNode->FindProperty(name))
      {
        VALUE* pValue = &m_MetaData[guid];

        ezReflectionUtils::SetMemberPropertyValue(static_cast<ezAbstractMemberProperty*>(pValue->GetDynamicRTTI()->FindPropertyByName(name)), pValue, pProp->m_Value);
      }
    }
  }

}





