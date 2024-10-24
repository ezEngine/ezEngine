#include <Core/Platform/NoImpl/Window_NoImpl.h>

ezWindowNoImpl::~ezWindowNoImpl()
{
}

ezResult ezWindowNoImpl::InitializeWindow()
{
  EZ_ASSERT_NOT_IMPLEMENTED;
  return EZ_FAILURE;
}

void ezWindowNoImpl::DestroyWindow()
{
  EZ_ASSERT_NOT_IMPLEMENTED;
}

ezResult ezWindowNoImpl::Resize(const ezSizeU32& newWindowSize)
{
  EZ_ASSERT_NOT_IMPLEMENTED;
  return EZ_FAILURE;
}

void ezWindowNoImpl::ProcessWindowMessages()
{
  EZ_ASSERT_NOT_IMPLEMENTED;
}

void ezWindowNoImpl::OnResize(const ezSizeU32& newWindowSize)
{
  EZ_ASSERT_NOT_IMPLEMENTED;
}

ezWindowHandle ezWindowNoImpl::GetNativeWindowHandle() const
{
  return m_hWindowHandle;
}
