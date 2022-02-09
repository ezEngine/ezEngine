

template <typename Object>
ezRttiMappedObjectFactory<Object>::ezRttiMappedObjectFactory()
{
}

template <typename Object>
ezRttiMappedObjectFactory<Object>::~ezRttiMappedObjectFactory()
{
}

template <typename Object>
void ezRttiMappedObjectFactory<Object>::RegisterCreator(const ezRTTI* pType, CreateObjectFunc creator)
{
  EZ_ASSERT_DEV(!m_Creators.Contains(pType), "Type already registered.");

  m_Creators.Insert(pType, creator);
  Event e;
  e.m_Type = Event::Type::CreatorAdded;
  e.m_pRttiType = pType;
  m_Events.Broadcast(e);
}

template <typename Object>
void ezRttiMappedObjectFactory<Object>::UnregisterCreator(const ezRTTI* pType)
{
  EZ_ASSERT_DEV(m_Creators.Contains(pType), "Type was never registered.");
  m_Creators.Remove(pType);

  Event e;
  e.m_Type = Event::Type::CreatorRemoved;
  e.m_pRttiType = pType;
  m_Events.Broadcast(e);
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
