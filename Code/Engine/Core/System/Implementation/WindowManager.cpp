#include <Core/CorePCH.h>

#include <Core/GameApplication/WindowOutputTargetBase.h>
#include <Core/System/Window.h>
#include <Core/System/WindowManager.h>
#include <Foundation/Configuration/Startup.h>

EZ_IMPLEMENT_SINGLETON(ezWindowManager);

//////////////////////////////////////////////////////////////////////////

static ezUniquePtr<ezWindowManager> s_pWindowManager;

// clang-format off
EZ_BEGIN_SUBSYSTEM_DECLARATION(Core, ezWindowManager)

  BEGIN_SUBSYSTEM_DEPENDENCIES
    "Foundation"
  END_SUBSYSTEM_DEPENDENCIES

  ON_CORESYSTEMS_STARTUP
  {
    s_pWindowManager = EZ_DEFAULT_NEW(ezWindowManager);
  }
  ON_CORESYSTEMS_SHUTDOWN
  {
    s_pWindowManager.Clear();
  }

  ON_HIGHLEVELSYSTEMS_SHUTDOWN
  {
    if (s_pWindowManager)
    {
      s_pWindowManager->CloseAll(nullptr);
    }
  }

EZ_END_SUBSYSTEM_DECLARATION;

// clang-format on

//////////////////////////////////////////////////////////////////////////

ezWindowManager::ezWindowManager()
  : m_SingletonRegistrar(this)
{
}

ezWindowManager::~ezWindowManager()
{
  CloseAll(nullptr);
}

void ezWindowManager::Update()
{
  EZ_LOCK(m_Mutex);

  for (auto it = m_Data.GetIterator(); it.IsValid(); ++it)
  {
    it.Value()->m_pWindow->ProcessWindowMessages();
  }
}

void ezWindowManager::Close(ezRegisteredWndHandle id)
{
  EZ_LOCK(m_Mutex);

  ezUniquePtr<Data>* pDataPtr = nullptr;
  if (!m_Data.TryGetValue(id.GetInternalID(), pDataPtr))
    return;

  Data* pData = pDataPtr->Borrow();
  EZ_ASSERT_DEV(pData != nullptr, "Invalid window data");

  if (pData->m_OnDestroy.IsValid())
  {
    pData->m_OnDestroy(id);
  }

  // The window output target has a dependency to the window, e.g. the swapchain renders to it.
  // Explicitly destroy it first to ensure correct destruction order.

  if (pData->m_pOutputTarget)
  {
    pData->m_pOutputTarget.Clear();
  }

  if (pData->m_pWindow)
  {
    pData->m_pWindow.Clear();
  }

  m_Data.Remove(id.GetInternalID());
}

void ezWindowManager::CloseAll(const void* pCreatedBy)
{
  EZ_LOCK(m_Mutex);

  ezDynamicArray<ezRegisteredWndHandle> toClose;

  for (auto it = m_Data.GetIterator(); it.IsValid(); ++it)
  {
    if (pCreatedBy == nullptr || it.Value()->m_pCreatedBy == pCreatedBy)
    {
      const ezRegisteredWndHandle id = ezRegisteredWndHandle(it.Id());
      toClose.PushBack(id);
    }
  }

  for (const ezRegisteredWndHandle& id : toClose)
  {
    Close(id);
  }
}

bool ezWindowManager::IsValid(ezRegisteredWndHandle id) const
{
  return m_Data.Contains(id.GetInternalID());
}

void ezWindowManager::GetRegistered(ezDynamicArray<ezRegisteredWndHandle>& out_WindowIDs, const void* pCreatedBy /*= nullptr*/)
{
  EZ_LOCK(m_Mutex);

  out_WindowIDs.Clear();

  for (auto it = m_Data.GetIterator(); it.IsValid(); ++it)
  {
    if (pCreatedBy == nullptr || it.Value()->m_pCreatedBy == pCreatedBy)
    {
      const ezRegisteredWndHandle id = ezRegisteredWndHandle(it.Id());
      out_WindowIDs.PushBack(id);
    }
  }
}

ezRegisteredWndHandle ezWindowManager::Register(ezStringView sName, const void* pCreatedBy, ezUniquePtr<ezWindowBase>&& pWindow)
{
  EZ_LOCK(m_Mutex);

  EZ_ASSERT_ALWAYS(pCreatedBy != nullptr, "pCreatedBy is invalid");
  EZ_ASSERT_ALWAYS(pWindow != nullptr, "pWindow is invalid");

  ezUniquePtr<Data> pData = EZ_DEFAULT_NEW(Data);
  pData->m_sName = sName;
  pData->m_pCreatedBy = pCreatedBy;
  pData->m_pWindow = std::move(pWindow);

  return ezRegisteredWndHandle(m_Data.Insert(std::move(pData)));
}

void ezWindowManager::SetOutputTarget(ezRegisteredWndHandle id, ezUniquePtr<ezWindowOutputTargetBase>&& pOutputTarget)
{
  EZ_LOCK(m_Mutex);

  ezUniquePtr<Data>* pDataPtr = nullptr;
  if (!m_Data.TryGetValue(id.GetInternalID(), pDataPtr))
    return;

  (*pDataPtr)->m_pOutputTarget = std::move(pOutputTarget);
}

void ezWindowManager::SetDestroyCallback(ezRegisteredWndHandle id, ezWindowDestroyFunc onDestroyCallback)
{
  EZ_LOCK(m_Mutex);

  ezUniquePtr<Data>* pDataPtr = nullptr;
  if (!m_Data.TryGetValue(id.GetInternalID(), pDataPtr))
    return;

  (*pDataPtr)->m_OnDestroy = onDestroyCallback;
}

ezStringView ezWindowManager::GetName(ezRegisteredWndHandle id) const
{
  if (!m_Data.Contains(id.GetInternalID()))
    return ezStringView();

  return m_Data[id.GetInternalID()]->m_sName;
}

ezWindowBase* ezWindowManager::GetWindow(ezRegisteredWndHandle id) const
{
  if (!m_Data.Contains(id.GetInternalID()))
    return nullptr;

  return m_Data[id.GetInternalID()]->m_pWindow.Borrow();
}

ezWindowOutputTargetBase* ezWindowManager::GetOutputTarget(ezRegisteredWndHandle id) const
{
  if (!m_Data.Contains(id.GetInternalID()))
    return nullptr;

  return m_Data[id.GetInternalID()]->m_pOutputTarget.Borrow();
}


EZ_STATICLINK_FILE(Core, Core_System_Implementation_WindowManager);
