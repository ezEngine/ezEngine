#include <Core/CorePCH.h>

#include <Core/ActorSystem/ActorPluginWindow.h>

//////////////////////////////////////////////////////////////////////////

// clang-format off
EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezActorPluginWindow, 1, ezRTTINoAllocator)
EZ_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

void ezActorPluginWindow::Update()
{
  if (GetWindow())
  {
    GetWindow()->ProcessWindowMessages();
  }
}

//////////////////////////////////////////////////////////////////////////

// clang-format off
EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezActorPluginWindowOwner, 1, ezRTTINoAllocator)
EZ_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

ezActorPluginWindowOwner::~ezActorPluginWindowOwner()
{
  // The window output target has a dependency to the window, e.g. the swapchain renders to it.
  // Explicitly destroy it first to ensure correct destruction order.
  m_pWindowOutputTarget.Clear();
  m_pWindow.Clear();
}

ezWindowBase* ezActorPluginWindowOwner::GetWindow() const
{
  return m_pWindow.Borrow();
}
ezWindowOutputTargetBase* ezActorPluginWindowOwner::GetOutputTarget() const
{
  return m_pWindowOutputTarget.Borrow();
}

EZ_STATICLINK_FILE(Core, Core_ActorSystem_Implementation_ActorPluginWindow);
