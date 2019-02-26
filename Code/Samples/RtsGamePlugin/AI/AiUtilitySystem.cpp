#include <RtsGamePluginPCH.h>

#include <RtsGamePlugin/AI/AiUtilitySystem.h>

RtsAiUtilitySystem::RtsAiUtilitySystem() = default;
RtsAiUtilitySystem::~RtsAiUtilitySystem() = default;

void RtsAiUtilitySystem::AddUtility(ezUniquePtr<RtsAiUtility>&& pUtility)
{
  m_Utilities.PushBack(std::move(pUtility));
}

void RtsAiUtilitySystem::Reevaluate(ezGameObject* pOwnerObject, ezComponent* pOwnerComponent, ezTime tNow, ezTime frequency)
{
  if (tNow - m_LastUpdate < frequency)
    return;

  m_LastUpdate = tNow;

  double fBestPrio = 0;
  RtsAiUtility* pBestUtility = nullptr;

  for (const auto& pUtility : m_Utilities)
  {
    double prio = pUtility->ComputePriority(pOwnerObject, pOwnerComponent);

    if (prio > fBestPrio)
    {
      fBestPrio = prio;
      pBestUtility = pUtility.Borrow();
    }
  }

  if (pBestUtility == m_pActiveUtility)
    return;

  if (m_pActiveUtility)
    m_pActiveUtility->Deactivate(pOwnerObject, pOwnerComponent);

  m_pActiveUtility = pBestUtility;

  if (m_pActiveUtility)
    m_pActiveUtility->Activate(pOwnerObject, pOwnerComponent);
}

bool RtsAiUtilitySystem::Execute(ezGameObject* pOwnerObject, ezComponent* pOwnerComponent, ezTime tNow)
{
  if (m_pActiveUtility == nullptr)
    return false;

  m_pActiveUtility->Execute(pOwnerObject, pOwnerComponent, tNow);
  return true;
}

//////////////////////////////////////////////////////////////////////////

RtsAiUtility::RtsAiUtility() = default;
RtsAiUtility::~RtsAiUtility() = default;
