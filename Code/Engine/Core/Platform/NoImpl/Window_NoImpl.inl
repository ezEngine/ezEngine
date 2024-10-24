#include <Core/System/Window.h>

ezResult ezWindowPlatformShared::Initialize()
{
  EZ_ASSERT_NOT_IMPLEMENTED;
  return EZ_FAILURE;
}

ezResult ezWindowPlatformShared::Destroy()
{
  EZ_ASSERT_NOT_IMPLEMENTED;
  return EZ_FAILURE;
}

ezResult ezWindowPlatformShared::Resize(const ezSizeU32& newWindowSize)
{
  EZ_ASSERT_NOT_IMPLEMENTED;
  return EZ_FAILURE;
}

void ezWindowPlatformShared::ProcessWindowMessages()
{
  EZ_ASSERT_NOT_IMPLEMENTED;
}

void ezWindowPlatformShared::OnResize(const ezSizeU32& newWindowSize)
{
  EZ_ASSERT_NOT_IMPLEMENTED;
}

ezWindowHandle ezWindowPlatformShared::GetNativeWindowHandle() const
{
  return m_hWindowHandle;
}
