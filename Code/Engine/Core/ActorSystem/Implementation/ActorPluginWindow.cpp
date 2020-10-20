#include <CorePCH.h>

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

ezWindowBase* ezActorPluginWindowOwner::GetWindow() const
{
  return m_pWindow.Borrow();
}
ezWindowOutputTargetBase* ezActorPluginWindowOwner::GetOutputTarget() const
{
  return m_pWindowOutputTarget.Borrow();
}

//////////////////////////////////////////////////////////////////////////

// clang-format off
EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezActorPluginWindowShared, 1, ezRTTINoAllocator);
EZ_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

ezWindowBase* ezActorPluginWindowShared::GetWindow() const
{
  return m_pWindow;
}

ezWindowOutputTargetBase* ezActorPluginWindowShared::GetOutputTarget() const
{
  return m_pWindowOutputTarget;
}


EZ_STATICLINK_FILE(Core, Core_ActorSystem_Implementation_ActorPluginWindow);
