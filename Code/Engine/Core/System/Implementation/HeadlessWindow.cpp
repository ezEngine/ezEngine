#include <Core/CorePCH.h>

#include <Core/System/HeadlessWindow.h>

ezHeadlessWindow::~ezHeadlessWindow()
{
  DestroyWindow();
}

ezResult ezHeadlessWindow::InitializeWindow()
{
  if (m_bInitialized)
    DestroyWindow();

  if (m_CreationDescription.m_Resolution.HasNonZeroArea() == false)
    return EZ_FAILURE;

  m_WindowSize = m_CreationDescription.m_Resolution;
  m_bInitialized = true;
  m_bVisible = false;
  m_bHasFocus = false;
  return EZ_SUCCESS;
}

void ezHeadlessWindow::DestroyWindow()
{
  m_WindowSize = {};
  m_bInitialized = false;
}

ezResult ezHeadlessWindow::Resize(const ezSizeU32& newWindowSize)
{
  if (!m_bInitialized || newWindowSize.HasNonZeroArea() == false)
    return EZ_FAILURE;

  m_WindowSize = newWindowSize;
  OnResize(newWindowSize);
  return EZ_SUCCESS;
}

void ezHeadlessWindow::ProcessWindowMessages()
{
}

ezWindowHandle ezHeadlessWindow::GetNativeWindowHandle() const
{
  return m_hWindowHandle;
}
