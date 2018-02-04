#include <Foundation/Logging/Log.h>

EZ_ALWAYS_INLINE ezComponent::ezComponent() :
  m_pMessageDispatchType(nullptr),
  m_ComponentFlags(ezObjectFlags::Active),
  m_pManager(nullptr),
  m_pOwner(nullptr)
{
  m_uiUniqueID = 0xFFFFFFFF;
}

EZ_ALWAYS_INLINE ezComponent::~ezComponent()
{
  m_pMessageDispatchType = nullptr;
  m_pManager = nullptr;
  m_pOwner = nullptr;
  m_InternalId.Invalidate();
}

EZ_ALWAYS_INLINE bool ezComponent::IsDynamic() const
{
  return m_ComponentFlags.IsSet(ezObjectFlags::Dynamic);
}

EZ_ALWAYS_INLINE bool ezComponent::IsActive() const
{
  return m_ComponentFlags.IsSet(ezObjectFlags::Active);
}

EZ_ALWAYS_INLINE bool ezComponent::IsActiveAndInitialized() const
{
  return m_ComponentFlags.AreAllSet(ezObjectFlags::Active | ezObjectFlags::Initialized);
}

EZ_ALWAYS_INLINE ezComponentManagerBase* ezComponent::GetOwningManager()
{
  return m_pManager;
}

EZ_ALWAYS_INLINE const ezComponentManagerBase* ezComponent::GetOwningManager() const
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

EZ_ALWAYS_INLINE bool ezComponent::IsInitialized() const
{
  return m_ComponentFlags.IsSet(ezObjectFlags::Initialized);
}

EZ_ALWAYS_INLINE bool ezComponent::IsInitializing() const
{
  return m_ComponentFlags.IsSet(ezObjectFlags::Initializing);
}

EZ_ALWAYS_INLINE bool ezComponent::IsSimulationStarted() const
{
  return m_ComponentFlags.IsSet(ezObjectFlags::SimulationStarted);
}

EZ_ALWAYS_INLINE bool ezComponent::IsActiveAndSimulating() const
{
  return m_ComponentFlags.AreAllSet(ezObjectFlags::Initialized | ezObjectFlags::Active) && m_ComponentFlags.IsAnySet(ezObjectFlags::SimulationStarting | ezObjectFlags::SimulationStarted);
}



