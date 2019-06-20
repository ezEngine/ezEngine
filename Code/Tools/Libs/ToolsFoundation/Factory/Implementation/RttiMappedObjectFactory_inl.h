

template <typename Object>
ezRttiMappedObjectFactory<Object>::ezRttiMappedObjectFactory()
{
}

template <typename Object>
ezRttiMappedObjectFactory<Object>::~ezRttiMappedObjectFactory()
{
}

template <typename Object>
ezResult ezRttiMappedObjectFactory<Object>::RegisterCreator(const ezRTTI* pType, CreateObjectFunc creator)
{
  if (m_Creators.Contains(pType))
    return EZ_FAILURE;

  m_Creators.Insert(pType, creator);
  Event e;
  e.m_Type = Event::Type::CreatorAdded;
  e.m_pRttiType = pType;
  m_Events.Broadcast(e);
  return EZ_SUCCESS;
}

template <typename Object>
ezResult ezRttiMappedObjectFactory<Object>::UnregisterCreator(const ezRTTI* pType)
{
  if (!m_Creators.Remove(pType))
  {
    return EZ_FAILURE;
  }

  Event e;
  e.m_Type = Event::Type::CreatorRemoved;
  e.m_pRttiType = pType;
  m_Events.Broadcast(e);
  return EZ_SUCCESS;
}

template <typename Object>
Object* ezRttiMappedObjectFactory<Object>::CreateObject(const ezRTTI* pType)
{
  CreateObjectFunc* creator = nullptr;
  while (pType != nullptr)
  {
    if (m_Creators.TryGetValue(pType, creator))
    {
      return (*creator)(pType);
    }
    pType = pType->GetParentType();
  }
  return nullptr;
}
