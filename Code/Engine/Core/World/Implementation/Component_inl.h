
EZ_FORCE_INLINE ezComponent::ezComponent() : m_Flags(ezObjectFlags::Default) 
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

EZ_FORCE_INLINE  ezGameObject* ezComponent::GetOwner() const
{
  return m_pOwner;
}

// static 
EZ_FORCE_INLINE ezUInt16 ezComponent::TypeId()
{ 
  return TYPE_ID;
}

