
ezEditorProcessViewWindow::~ezEditorProcessViewWindow()
{
  ezGALDevice::GetDefaultDevice()->WaitIdle();

  EZ_ASSERT_DEV(m_iReferenceCount == 0, "The window is still being referenced, probably by a swapchain. Make sure to destroy all swapchains and call ezGALDevice::WaitIdle before destroying a window.");
}

ezResult ezEditorProcessViewWindow::UpdateWindow(ezWindowHandle parentWindow, ezUInt16 uiWidth, ezUInt16 uiHeight)
{
  m_hWnd = parentWindow;
  m_uiWidth = uiWidth;
  m_uiHeight = uiHeight;

  return EZ_SUCCESS;
}
