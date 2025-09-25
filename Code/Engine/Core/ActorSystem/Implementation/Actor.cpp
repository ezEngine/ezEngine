#include <Core/CorePCH.h>

#include <Core/ActorSystem/Actor.h>

struct ezActorImpl
{
  ezString m_sName;
  const void* m_pCreatedBy = nullptr;
  ezHybridArray<ezUniquePtr<ezActorPlugin>, 1> m_AllPlugins;
};

ezActor::ezActor(ezStringView sActorName, const void* pCreatedBy)
{
  m_pImpl = EZ_DEFAULT_NEW(ezActorImpl);

  m_pImpl->m_sName = sActorName;
  m_pImpl->m_pCreatedBy = pCreatedBy;

  EZ_ASSERT_DEV(!m_pImpl->m_sName.IsEmpty(), "Actor name must not be empty");
}

ezActor::~ezActor() = default;

ezStringView ezActor::GetName() const
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

  EZ_ASSERT_DEV(m_pImpl->m_AllPlugins.IsEmpty(), "Only one plugin allowed");

  m_pImpl->m_AllPlugins.PushBack(std::move(pPlugin));
}

ezActorPlugin* ezActor::GetPlugin(const ezRTTI* pPluginType) const
{
  EZ_ASSERT_DEV(pPluginType->IsDerivedFrom<ezActorPlugin>(), "The queried type has to derive from ezActorPlugin");

  for (auto& pPlugin : m_pImpl->m_AllPlugins)
  {
    if (pPlugin->IsInstanceOf(pPluginType))
    {
      return pPlugin.Borrow();
    }
  }

  return nullptr;
}

void ezActor::Update()
{
  for (auto& pPlugin : m_pImpl->m_AllPlugins)
  {
    pPlugin->Update();
  }
}


