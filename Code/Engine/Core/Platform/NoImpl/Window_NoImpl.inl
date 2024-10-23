#include <Core/System/Window.h>

ezResult ezWindow::Initialize()
{
  EZ_ASSERT_NOT_IMPLEMENTED;
  return EZ_FAILURE;
}

ezResult ezWindow::Destroy()
{
  EZ_ASSERT_NOT_IMPLEMENTED;
  return EZ_FAILURE;
}

ezResult ezWindow::Resize(const ezSizeU32& newWindowSize)
{
  EZ_ASSERT_NOT_IMPLEMENTED;
  return EZ_FAILURE;
}

void ezWindow::ProcessWindowMessages()
{
  EZ_ASSERT_NOT_IMPLEMENTED;
}

void ezWindow::OnResize(const ezSizeU32& newWindowSize)
{
  EZ_ASSERT_NOT_IMPLEMENTED;
}

ezWindowHandle ezWindow::GetNativeWindowHandle() const
{
  return m_hWindowHandle;
}
