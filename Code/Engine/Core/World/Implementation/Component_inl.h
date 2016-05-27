
EZ_FORCE_INLINE ezComponent::ezComponent() :
  m_ComponentFlags(ezObjectFlags::Default),
  m_pManager(nullptr),
  m_pOwner(nullptr)
{
  m_uiEditorPickingID = 0xFFFFFFFF;
}

EZ_FORCE_INLINE ezComponent::~ezComponent() 
{
  m_pManager = nullptr;
  m_pOwner = nullptr;
  m_InternalId.Invalidate();
}

EZ_FORCE_INLINE bool ezComponent::IsDynamic() const
{
  return m_ComponentFlags.IsSet(ezObjectFlags::Dynamic);
}

EZ_FORCE_INLINE bool ezComponent::IsActive() const
{
  return m_pOwner && m_ComponentFlags.IsSet(ezObjectFlags::Active);
}

EZ_FORCE_INLINE bool ezComponent::IsActiveAndInitialized() const
{
  return m_pOwner && m_ComponentFlags.AreAllSet(ezObjectFlags::Active | ezObjectFlags::Initialized);
}

EZ_FORCE_INLINE bool ezComponent::IsSimulationStarted() const
{
  return m_ComponentFlags.IsSet(ezObjectFlags::SimulationStarted);
}

EZ_FORCE_INLINE ezComponentManagerBase* ezComponent::GetManager()
{
  return m_pManager;
}

EZ_FORCE_INLINE const ezComponentManagerBase* ezComponent::GetManager() const
{
  return m_pManager;
}

EZ_FORCE_INLINE ezGameObject* ezComponent::GetOwner()
{
  return m_pOwner;
}

EZ_FORCE_INLINE const ezGameObject* ezComponent::GetOwner() const
{
  return m_pOwner;
}

// static 
EZ_FORCE_INLINE ezUInt16 ezComponent::TypeId()
{ 
  return TYPE_ID;
}

EZ_FORCE_INLINE ezUInt32 ezComponent::GetEditorPickingID() const
{
  return m_uiEditorPickingID;
}

EZ_FORCE_INLINE void ezComponent::SetEditorPickingID(ezUInt32 uiEditorPickingID)
{
  m_uiEditorPickingID = uiEditorPickingID;
}

EZ_FORCE_INLINE void ezComponent::SendMessage(ezMessage& msg)
{
  if (IsActiveAndInitialized() || IsInitializing())
    GetDynamicRTTI()->DispatchMessage(this, msg);
}

EZ_FORCE_INLINE void ezComponent::SendMessage(ezMessage& msg) const
{
  if (IsActiveAndInitialized() || IsInitializing())
    GetDynamicRTTI()->DispatchMessage(this, msg);
}

template <typename T>
EZ_FORCE_INLINE ezComponentHandle ezComponent::GetHandleInternal() const
{
  return ezComponentHandle(ezComponentId(m_InternalId, T::TypeId(), GetWorldIndex()));
}

EZ_FORCE_INLINE bool ezComponent::IsInitialized() const
{
  return m_ComponentFlags.IsSet(ezObjectFlags::Initialized);
}

EZ_FORCE_INLINE bool ezComponent::IsInitializing() const
{
  return m_ComponentFlags.IsSet(ezObjectFlags::Initializing);
}


