
EZ_FORCE_INLINE ezComponent::ezComponent() : 
  m_pManager(nullptr),
  m_Flags(ezObjectFlags::Default),
  m_pOwner(nullptr)
{ 
}

EZ_FORCE_INLINE ezComponent::~ezComponent() 
{
  m_pManager = nullptr;
  m_pOwner = nullptr;
  m_InternalId.Invalidate();
}

EZ_FORCE_INLINE bool ezComponent::IsDynamic() const
{
  return m_Flags.IsSet(ezObjectFlags::Dynamic);
}

EZ_FORCE_INLINE bool ezComponent::IsActive() const
{
  return m_Flags.IsSet(ezObjectFlags::Active);
}

EZ_FORCE_INLINE ezComponentManagerBase* ezComponent::GetManager() const
{
  return m_pManager;
}

EZ_FORCE_INLINE ezGameObject* ezComponent::GetOwner() const
{
  return m_pOwner;
}

EZ_FORCE_INLINE ezComponentHandle ezComponent::GetHandle() const
{
  return ezComponentHandle(ezComponentId(m_InternalId, GetTypeId()));
}

//static 
EZ_FORCE_INLINE ezUInt16 ezComponent::GetNextTypeId()
{
  return s_uiNextTypeId++;
}

// static 
EZ_FORCE_INLINE ezUInt16 ezComponent::TypeId()
{ 
  return TYPE_ID;
}

template <typename T>
EZ_FORCE_INLINE ezComponentHandle ezComponent::GetHandle() const
{
  return ezComponentHandle(ezComponentId(m_InternalId, T::TypeId()));
}

EZ_FORCE_INLINE bool ezComponent::IsInitialized() const
{
  return m_Flags.IsSet(ezObjectFlags::Initialized);
}

EZ_FORCE_INLINE void ezComponent::OnMessage(ezMessage& msg) 
{
  GetDynamicRTTI()->DispatchMessage(this, msg);
}

EZ_FORCE_INLINE void ezComponent::OnMessage(ezMessage& msg) const
{
  GetDynamicRTTI()->DispatchMessage(this, msg);
}

