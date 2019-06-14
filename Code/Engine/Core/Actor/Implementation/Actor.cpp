#include <CorePCH.h>

#include <Core/Actor/Actor.h>
#include <Core/Actor/ActorDevice.h>

// clang-format off
EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezActor, 1, ezRTTINoAllocator)
EZ_END_DYNAMIC_REFLECTED_TYPE;

// clang-format on

ezEvent<const ezActorEvent&> ezActor::s_Events;

struct ezActorImpl
{
  ezString m_sName;
  const void* m_pCreatedBy = nullptr;
  ezHybridArray<ezUniquePtr<ezActorDevice>, 4> m_AllDevices;
  ezMap<const ezRTTI*, ezActorDevice*> m_DeviceLookupCache;
};


ezActor::ezActor(const char* szActorName, const void* pCreatedBy)
{
  m_pImpl = EZ_DEFAULT_NEW(ezActorImpl);

  m_pImpl->m_sName = szActorName;
  m_pImpl->m_pCreatedBy = pCreatedBy;

  EZ_ASSERT_DEV(!m_pImpl->m_sName.IsEmpty(), "Actor name must not be empty");
}

ezActor::~ezActor() = default;

ezActorManager* ezActor::GetManager() const
{
  return m_pOwningManager;
}

const char* ezActor::GetName() const
{
  return m_pImpl->m_sName;
}

const void* ezActor::GetCreatedBy() const
{
  return m_pImpl->m_pCreatedBy;
}

void ezActor::AddDevice(ezUniquePtr<ezActorDevice>&& pDevice)
{
  EZ_ASSERT_DEV(pDevice != nullptr, "Invalid actor device");
  EZ_ASSERT_DEV(pDevice->m_pOwningActor == nullptr, "Actor device already in use");

  pDevice->m_pOwningActor = this;

  // register this device under its type and all its base types
  for (const ezRTTI* pRtti = pDevice->GetDynamicRTTI(); pRtti != ezGetStaticRTTI<ezActorDevice>(); pRtti = pRtti->GetParentType())
  {
    m_pImpl->m_DeviceLookupCache[pRtti] = pDevice.Borrow();
  }

  m_pImpl->m_AllDevices.PushBack(std::move(pDevice));
}

ezActorDevice* ezActor::GetDevice(const ezRTTI* pDeviceType) const
{
  EZ_ASSERT_DEV(pDeviceType->IsDerivedFrom<ezActorDevice>(), "The queried type has to derive from ezActorDevice");

  return m_pImpl->m_DeviceLookupCache.GetValueOrDefault(pDeviceType, nullptr);
}

void ezActor::GetAllDevices(ezHybridArray<ezActorDevice*, 8>& out_AllDevices)
{
  out_AllDevices.Clear();

  for (auto& pDevice : m_pImpl->m_AllDevices)
  {
    out_AllDevices.PushBack(pDevice.Borrow());
  }
}

void ezActor::Activate() {}
void ezActor::Deactivate() {}
void ezActor::Update() {}
