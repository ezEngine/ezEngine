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
  for (auto it = m_Data.GetIterator(); it.IsValid(); ++it)
  {
    it.Value()->m_pWindow->ProcessWindowMessages();
  }
}

void ezWindowManager::Close(ezRegisteredWndHandle hWindow)
{
  ezUniquePtr<Data>* pDataPtr = nullptr;
  if (!m_Data.TryGetValue(hWindow.GetInternalID(), pDataPtr))
    return;

  Data* pData = pDataPtr->Borrow();
  EZ_ASSERT_DEV(pData != nullptr, "Invalid window data");

  if (pData->m_OnDestroy.IsValid())
  {
    pData->m_OnDestroy(hWindow);
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

  m_Data.Remove(hWindow.GetInternalID());
}

void ezWindowManager::CloseAll(const void* pCreatedBy)
{
  ezDynamicArray<ezRegisteredWndHandle> toClose;

  for (auto it = m_Data.GetIterator(); it.IsValid(); ++it)
  {
    if (pCreatedBy == nullptr || it.Value()->m_pCreatedBy == pCreatedBy)
    {
      toClose.PushBack(ezRegisteredWndHandle(it.Id()));
    }
  }

  for (const ezRegisteredWndHandle& hWindow : toClose)
  {
    Close(hWindow);
  }
}

bool ezWindowManager::IsValid(ezRegisteredWndHandle hWindow) const
{
  return m_Data.Contains(hWindow.GetInternalID());
}

void ezWindowManager::GetRegistered(ezDynamicArray<ezRegisteredWndHandle>& out_windowHandles, const void* pCreatedBy /*= nullptr*/)
{
  out_windowHandles.Clear();

  for (auto it = m_Data.GetIterator(); it.IsValid(); ++it)
  {
    if (pCreatedBy == nullptr || it.Value()->m_pCreatedBy == pCreatedBy)
    {
      out_windowHandles.PushBack(ezRegisteredWndHandle(it.Id()));
    }
  }
}

ezRegisteredWndHandle ezWindowManager::Register(ezStringView sName, const void* pCreatedBy, ezUniquePtr<ezWindowBase>&& pWindow)
{
  EZ_ASSERT_ALWAYS(pCreatedBy != nullptr, "pCreatedBy is invalid");
  EZ_ASSERT_ALWAYS(pWindow != nullptr, "pWindow is invalid");

  ezUniquePtr<Data> pData = EZ_DEFAULT_NEW(Data);
  pData->m_sName = sName;
  pData->m_pCreatedBy = pCreatedBy;
  pData->m_pWindow = std::move(pWindow);

  return ezRegisteredWndHandle(m_Data.Insert(std::move(pData)));
}

void ezWindowManager::SetOutputTarget(ezRegisteredWndHandle hWindow, ezUniquePtr<ezWindowOutputTargetBase>&& pOutputTarget)
{
  ezUniquePtr<Data>* pDataPtr = nullptr;
  if (!m_Data.TryGetValue(hWindow.GetInternalID(), pDataPtr))
    return;

  (*pDataPtr)->m_pOutputTarget = std::move(pOutputTarget);
}

void ezWindowManager::SetDestroyCallback(ezRegisteredWndHandle hWindow, ezWindowDestroyFunc onDestroyCallback)
{
  ezUniquePtr<Data>* pDataPtr = nullptr;
  if (!m_Data.TryGetValue(hWindow.GetInternalID(), pDataPtr))
    return;

  (*pDataPtr)->m_OnDestroy = onDestroyCallback;
}

ezStringView ezWindowManager::GetName(ezRegisteredWndHandle hWindow) const
{
  if (!m_Data.Contains(hWindow.GetInternalID()))
    return ezStringView();

  return m_Data[hWindow.GetInternalID()]->m_sName;
}

ezWindowBase* ezWindowManager::GetWindow(ezRegisteredWndHandle hWindow) const
{
  if (!m_Data.Contains(hWindow.GetInternalID()))
    return nullptr;

  return m_Data[hWindow.GetInternalID()]->m_pWindow.Borrow();
}

ezWindowOutputTargetBase* ezWindowManager::GetOutputTarget(ezRegisteredWndHandle hWindow) const
{
  if (!m_Data.Contains(hWindow.GetInternalID()))
    return nullptr;

  return m_Data[hWindow.GetInternalID()]->m_pOutputTarget.Borrow();
}


EZ_STATICLINK_FILE(Core, Core_System_Implementation_WindowManager);
