
template <typename RttiType, typename Object, typename TypeTraverser>
ezRttiMappedObjectFactoryBase<RttiType, Object, TypeTraverser>::ezRttiMappedObjectFactoryBase()
  : m_TypeTraverser(TypeTraverser())
{
}

template <typename RttiType, typename Object, typename TypeTraverser>
ezRttiMappedObjectFactoryBase<RttiType, Object, TypeTraverser>::~ezRttiMappedObjectFactoryBase()
{
}

template <typename RttiType, typename Object, typename TypeTraverser>
ezResult ezRttiMappedObjectFactoryBase<RttiType, Object, TypeTraverser>::RegisterCreator(RttiType type, Creator& creator)
{
  if (m_Creators.Contains(type))
    return EZ_FAILURE;

  m_Creators.Insert(type, creator);
  Event e;
  e.m_Type = Event::Type::CreatorAdded;
  e.m_RttiType = type;
  m_Events.Broadcast(e);
  return EZ_SUCCESS;
}

template <typename RttiType, typename Object, typename TypeTraverser>
ezResult ezRttiMappedObjectFactoryBase<RttiType, Object, TypeTraverser>::UnregisterCreator(RttiType type)
{
  if (!m_Creators.Remove())
  {
    return EZ_FAILURE;
  }

  Event e;
  e.m_Type = Event::Type::CreatorRemoved;
  e.m_RttiType = type;
  m_Events.Broadcast(e);
  return EZ_SUCCESS;
}

template <typename RttiType, typename Object, typename TypeTraverser>
Object* ezRttiMappedObjectFactoryBase<RttiType, Object, TypeTraverser>::CreateObject(RttiType type)
{
  Creator* creator = nullptr;
  while (m_TypeTraverser.IsValid(type))
  {
    if (m_Creators.TryGetValue(type, creator))
    {
      return creator->m_CreateObject();
    }
    type = m_TypeTraverser.GetParentType(type);
  }
  return nullptr;
}
