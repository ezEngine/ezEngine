#include <Foundation/Logging/Log.h>

EZ_FORCE_INLINE ezComponent::ezComponent() :
  m_ComponentFlags(ezObjectFlags::Default),
  m_pManager(nullptr),
  m_pOwner(nullptr)
{
  m_uiUniqueID = 0xFFFFFFFF;
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

EZ_ALWAYS_INLINE ezComponentManagerBase* ezComponent::GetManager()
{
  return m_pManager;
}

EZ_ALWAYS_INLINE const ezComponentManagerBase* ezComponent::GetManager() const
{
  return m_pManager;
}

EZ_ALWAYS_INLINE ezGameObject* ezComponent::GetOwner()
{
  return m_pOwner;
}

EZ_ALWAYS_INLINE const ezGameObject* ezComponent::GetOwner() const
{
  return m_pOwner;
}

EZ_ALWAYS_INLINE ezUInt32 ezComponent::GetUniqueID() const
{
  return m_uiUniqueID;
}

EZ_ALWAYS_INLINE void ezComponent::SetUniqueID(ezUInt32 uiUniqueID)
{
  m_uiUniqueID = uiUniqueID;
}

EZ_FORCE_INLINE bool ezComponent::SendMessage(ezMessage& msg)
{
  if (IsActiveAndInitialized() || IsInitializing())
  {
    if (!m_pMessageDispatchType->DispatchMessage(this, msg))
    {
#if EZ_ENABLED(EZ_COMPILE_FOR_DEBUG)
      if (msg.m_bPleaseTellMeInDetailWhenAndWhyThisMessageDoesNotArrive)
        ezLog::Warning("Component type '{0}' does not have a message handler for messages of type {1}", GetDynamicRTTI()->GetTypeName(), msg.GetId());
#endif

      return false;
    }
    else if (m_ComponentFlags.IsSet(ezObjectFlags::UnhandledMessageHandler))
    {
      return OnUnhandledMessage(msg);
    }
  }
  else
  {
#if EZ_ENABLED(EZ_COMPILE_FOR_DEBUG)
    if (msg.m_bPleaseTellMeInDetailWhenAndWhyThisMessageDoesNotArrive)
      ezLog::Warning("Discarded message with ID {0} because component of type '{1}' is not initialized or not active at the moment", msg.GetId(), GetDynamicRTTI()->GetTypeName());
#endif

    return false;
  }

  return true;
}

EZ_FORCE_INLINE bool ezComponent::SendMessage(ezMessage& msg) const
{
  if (IsActiveAndInitialized() || IsInitializing())
  {
    if (!m_pMessageDispatchType->DispatchMessage(this, msg))
    {
#if EZ_ENABLED(EZ_COMPILE_FOR_DEBUG)
      if (msg.m_bPleaseTellMeInDetailWhenAndWhyThisMessageDoesNotArrive)
        ezLog::Warning("(const) Component type '{0}' does not have a CONST message handler for messages of type {1}", GetDynamicRTTI()->GetTypeName(), msg.GetId());
#endif

      return false;
    }
    else if (m_ComponentFlags.IsSet(ezObjectFlags::UnhandledMessageHandler))
    {
      return OnUnhandledMessage(msg);
    }
  }
  else
  {
#if EZ_ENABLED(EZ_COMPILE_FOR_DEBUG)
    if (msg.m_bPleaseTellMeInDetailWhenAndWhyThisMessageDoesNotArrive)
      ezLog::Warning("Discarded message with ID {0} because component of type '{1}' is not initialized or not active at the moment", msg.GetId(), GetDynamicRTTI()->GetTypeName());
#endif

    return false;
  }

  return true;
}

template <typename ManagerType>
EZ_FORCE_INLINE ezComponentHandle ezComponent::GetHandleInternal() const
{
  return ezComponentHandle(ezComponentId(m_InternalId, ManagerType::TypeId(), GetWorldIndex()));
}

EZ_FORCE_INLINE bool ezComponent::IsInitialized() const
{
  return m_ComponentFlags.IsSet(ezObjectFlags::Initialized);
}

EZ_FORCE_INLINE bool ezComponent::IsInitializing() const
{
  return m_ComponentFlags.IsSet(ezObjectFlags::Initializing);
}


