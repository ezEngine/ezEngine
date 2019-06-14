#include <CorePCH.h>

#include <Core/ActorSystem/Actor2.h>

// clang-format off
EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezActor2, 1, ezRTTINoAllocator);
EZ_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

struct ezActor2Impl
{
  ezString m_sName;
  const void* m_pCreatedBy = nullptr;
  ezHybridArray<ezUniquePtr<ezActorPlugin>, 4> m_AllDevices;
  ezMap<const ezRTTI*, ezActorPlugin*> m_DeviceLookupCache;
};


ezActor2::ezActor2(const char* szActorName, const void* pCreatedBy)
{
  m_pImpl = EZ_DEFAULT_NEW(ezActor2Impl);

  m_pImpl->m_sName = szActorName;
  m_pImpl->m_pCreatedBy = pCreatedBy;

  EZ_ASSERT_DEV(!m_pImpl->m_sName.IsEmpty(), "Actor name must not be empty");
}

ezActor2::~ezActor2() = default;

const char* ezActor2::GetName() const
{
  return m_pImpl->m_sName;
}

const void* ezActor2::GetCreatedBy() const
{
  return m_pImpl->m_pCreatedBy;
}

void ezActor2::AddPlugin(ezUniquePtr<ezActorPlugin>&& pPlugin)
{
  EZ_ASSERT_DEV(pPlugin != nullptr, "Invalid actor device");
  EZ_ASSERT_DEV(pPlugin->m_pOwningActor == nullptr, "Actor device already in use");

  pPlugin->m_pOwningActor = this;

  // register this device under its type and all its base types
  for (const ezRTTI* pRtti = pPlugin->GetDynamicRTTI(); pRtti != ezGetStaticRTTI<ezActorPlugin>(); pRtti = pRtti->GetParentType())
  {
    m_pImpl->m_DeviceLookupCache[pRtti] = pPlugin.Borrow();
  }

  m_pImpl->m_AllDevices.PushBack(std::move(pPlugin));
}

ezActorPlugin* ezActor2::GetPlugin(const ezRTTI* pDeviceType) const
{
  EZ_ASSERT_DEV(pDeviceType->IsDerivedFrom<ezActorPlugin>(), "The queried type has to derive from ezActorDevice");

  return m_pImpl->m_DeviceLookupCache.GetValueOrDefault(pDeviceType, nullptr);
}

void ezActor2::GetAllPlugins(ezHybridArray<ezActorPlugin*, 8>& out_AllDevices)
{
  out_AllDevices.Clear();

  for (auto& pDevice : m_pImpl->m_AllDevices)
  {
    out_AllDevices.PushBack(pDevice.Borrow());
  }
}

void ezActor2::Update()
{
  if (m_pWindow)
  {
    m_pWindow->ProcessWindowMessages();
  }
}
