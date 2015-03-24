
template <typename RttiType, typename Object, typename TypeTraverser>
ezHashTable<RttiType, typename ezRttiMappedObjectFactoryBase<RttiType, Object, TypeTraverser>::CreateObjectFunc> ezRttiMappedObjectFactoryBase<RttiType, Object, TypeTraverser>::s_Creators;

template <typename RttiType, typename Object, typename TypeTraverser>
ezEvent<const typename ezRttiMappedObjectFactoryBase<RttiType, Object, TypeTraverser>::Event&> ezRttiMappedObjectFactoryBase<RttiType, Object, TypeTraverser>::s_Events;

template <typename RttiType, typename Object, typename TypeTraverser>
ezResult ezRttiMappedObjectFactoryBase<RttiType, Object, TypeTraverser>::RegisterCreator(RttiType type, CreateObjectFunc creator)
{
  if (s_Creators.Contains(type))
    return EZ_FAILURE;

  s_Creators.Insert(type, creator);
  Event e;
  e.m_Type = Event::Type::CreatorAdded;
  e.m_RttiType = type;
  s_Events.Broadcast(e);
  return EZ_SUCCESS;
}

template <typename RttiType, typename Object, typename TypeTraverser>
ezResult ezRttiMappedObjectFactoryBase<RttiType, Object, TypeTraverser>::UnregisterCreator(RttiType type)
{
  if (!s_Creators.Remove())
  {
    return EZ_FAILURE;
  }

  Event e;
  e.m_Type = Event::Type::CreatorRemoved;
  e.m_RttiType = type;
  s_Events.Broadcast(e);
  return EZ_SUCCESS;
}

template <typename RttiType, typename Object, typename TypeTraverser>
Object* ezRttiMappedObjectFactoryBase<RttiType, Object, TypeTraverser>::CreateObject(RttiType type)
{
  CreateObjectFunc* creator = nullptr;
  while (TypeTraverser::IsValid(type))
  {
    if (s_Creators.TryGetValue(type, creator))
    {
      return (*creator)(type);
    }
    type = TypeTraverser::GetParentType(type);
  }
  return nullptr;
}
