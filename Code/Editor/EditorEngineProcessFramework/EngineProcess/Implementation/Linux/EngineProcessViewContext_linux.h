
#include <xcb/xcb.h>

ezEditorProcessViewWindow::~ezEditorProcessViewWindow()
{
  if (m_hWnd.type == ezWindowHandle::Type::XCB)
  {
    ezGALDevice::GetDefaultDevice()->WaitIdle();

    EZ_ASSERT_DEV(m_iReferenceCount == 0, "The window is still being referenced, probably by a swapchain. Make sure to destroy all swapchains and call ezGALDevice::WaitIdle before destroying a window.");
    xcb_disconnect(m_hWnd.xcbWindow.m_pConnection);
    m_hWnd.xcbWindow.m_pConnection = nullptr;
    m_hWnd.type = ezWindowHandle::Type::Invalid;
  }
}

ezResult ezEditorProcessViewWindow::UpdateWindow(ezWindowHandle parentWindow, ezUInt16 uiWidth, ezUInt16 uiHeight)
{
  if (m_hWnd.type == ezWindowHandle::Type::Invalid)
  {
    // xcb_connect always returns a non-NULL pointer to a xcb_connection_t,
    // even on failure. Callers need to use xcb_connection_has_error() to
    // check for failure. When finished, use xcb_disconnect() to close the
    // connection and free the structure.
    int scr = 0;
    m_hWnd.type = ezWindowHandle::Type::XCB;
    m_hWnd.xcbWindow.m_pConnection = xcb_connect(NULL, &scr);
    if (auto err = xcb_connection_has_error(m_hWnd.xcbWindow.m_pConnection); err != 0)
    {
      ezLog::Error("Could not connect to x11 via xcb. Error-Code '{}'", err);
      xcb_disconnect(m_hWnd.xcbWindow.m_pConnection);
      m_hWnd.xcbWindow.m_pConnection = nullptr;
      m_hWnd.type = ezWindowHandle::Type::Invalid;
      return EZ_FAILURE;
    }

    m_hWnd.xcbWindow.m_Window = parentWindow.xcbWindow.m_Window;
  }

  m_uiWidth = uiWidth;
  m_uiHeight = uiHeight;
  EZ_ASSERT_DEV(parentWindow.type == ezWindowHandle::Type::XCB && parentWindow.xcbWindow.m_Window != 0, "Invalid handle passed");
  EZ_ASSERT_DEV(m_hWnd.xcbWindow.m_Window == parentWindow.xcbWindow.m_Window, "Remote window handle should never change. Window must be destroyed and recreated.");

  return EZ_SUCCESS;
}
