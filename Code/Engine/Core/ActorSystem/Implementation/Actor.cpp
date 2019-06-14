#include <CorePCH.h>

#include <Core/ActorSystem/Actor.h>

// clang-format off
EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezActor, 1, ezRTTINoAllocator);
EZ_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

struct ezActorImpl
{
  ezString m_sName;
  const void* m_pCreatedBy = nullptr;
  ezHybridArray<ezUniquePtr<ezActorPlugin>, 4> m_AllPlugins;
  ezMap<const ezRTTI*, ezActorPlugin*> m_PluginLookupCache;
};


ezActor::ezActor(const char* szActorName, const void* pCreatedBy)
{
  m_pImpl = EZ_DEFAULT_NEW(ezActorImpl);

  m_pImpl->m_sName = szActorName;
  m_pImpl->m_pCreatedBy = pCreatedBy;

  EZ_ASSERT_DEV(!m_pImpl->m_sName.IsEmpty(), "Actor name must not be empty");
}

ezActor::~ezActor() = default;

const char* ezActor::GetName() const
{
  return m_pImpl->m_sName;
}

const void* ezActor::GetCreatedBy() const
{
  return m_pImpl->m_pCreatedBy;
}

void ezActor::AddPlugin(ezUniquePtr<ezActorPlugin>&& pPlugin)
{
  EZ_ASSERT_DEV(pPlugin != nullptr, "Invalid actor plugin");
  EZ_ASSERT_DEV(pPlugin->m_pOwningActor == nullptr, "Actor plugin already in use");

  pPlugin->m_pOwningActor = this;

  // register this plugin under its type and all its base types
  for (const ezRTTI* pRtti = pPlugin->GetDynamicRTTI(); pRtti != ezGetStaticRTTI<ezActorPlugin>(); pRtti = pRtti->GetParentType())
  {
    m_pImpl->m_PluginLookupCache[pRtti] = pPlugin.Borrow();
  }

  m_pImpl->m_AllPlugins.PushBack(std::move(pPlugin));
}

ezActorPlugin* ezActor::GetPlugin(const ezRTTI* pPluginType) const
{
  EZ_ASSERT_DEV(pPluginType->IsDerivedFrom<ezActorPlugin>(), "The queried type has to derive from ezActorPlugin");

  return m_pImpl->m_PluginLookupCache.GetValueOrDefault(pPluginType, nullptr);
}

void ezActor::DestroyPlugin(ezActorPlugin* pPlugin)
{
  for (ezUInt32 i = 0; i < m_pImpl->m_AllPlugins.GetCount(); ++i)
  {
    if (m_pImpl->m_AllPlugins[i].Borrow() == pPlugin)
    {
      m_pImpl->m_AllPlugins.RemoveAtAndSwap(i);
      break;
    }
  }
}

void ezActor::GetAllPlugins(ezHybridArray<ezActorPlugin*, 8>& out_AllPlugins)
{
  out_AllPlugins.Clear();

  for (auto& pPlugin : m_pImpl->m_AllPlugins)
  {
    out_AllPlugins.PushBack(pPlugin.Borrow());
  }
}

void ezActor::UpdateAllPlugins()
{
  for (auto& pPlugin : m_pImpl->m_AllPlugins)
  {
    pPlugin->Update();
  }
}

void ezActor::Update()
{
  if (m_pWindow)
  {
    m_pWindow->ProcessWindowMessages();
  }

  UpdateAllPlugins();
}
