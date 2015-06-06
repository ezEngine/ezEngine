

template <typename Object>
ezRttiMappedObjectFactory<Object>::ezRttiMappedObjectFactory()
{
}

template <typename Object>
ezRttiMappedObjectFactory<Object>::~ezRttiMappedObjectFactory()
{
}

template <typename Object>
ezHashTable<const ezRTTI*, typename ezRttiMappedObjectFactory<Object>::CreateObjectFunc> ezRttiMappedObjectFactory<Object>::s_Creators;

template <typename Object>
ezEvent<const typename ezRttiMappedObjectFactory<Object>::Event&> ezRttiMappedObjectFactory<Object>::s_Events;

template <typename Object>
ezResult ezRttiMappedObjectFactory<Object>::RegisterCreator(const ezRTTI* pType, CreateObjectFunc creator)
{
  if (s_Creators.Contains(pType))
    return EZ_FAILURE;

  s_Creators.Insert(pType, creator);
  Event e;
  e.m_Type = Event::Type::CreatorAdded;
  e.m_pRttiType = pType;
  s_Events.Broadcast(e);
  return EZ_SUCCESS;
}

template <typename Object>
ezResult ezRttiMappedObjectFactory<Object>::UnregisterCreator(const ezRTTI* pType)
{
  if (!s_Creators.Remove())
  {
    return EZ_FAILURE;
  }

  Event e;
  e.m_Type = Event::Type::CreatorRemoved;
  e.m_pRttiType = pType;
  s_Events.Broadcast(e);
  return EZ_SUCCESS;
}

template <typename Object>
Object* ezRttiMappedObjectFactory<Object>::CreateObject(const ezRTTI* pType)
{
  CreateObjectFunc* creator = nullptr;
  while (pType != nullptr)
  {
    if (s_Creators.TryGetValue(pType, creator))
    {
      return (*creator)(pType);
    }
    pType = pType->GetParentType();
  }
  return nullptr;
}
