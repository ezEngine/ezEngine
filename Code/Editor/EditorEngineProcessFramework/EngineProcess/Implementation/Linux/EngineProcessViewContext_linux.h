
#include <xcb/xcb.h>

ezEditorProcessViewWindow::~ezEditorProcessViewWindow()
{
  if (m_hWnd.type == ezWindowHandle::Type::XCB)
  {
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
  }

  m_uiWidth = uiWidth;
  m_uiHeight = uiHeight;
  EZ_ASSERT_DEV(parentWindow.type == ezWindowHandle::Type::XCB && parentWindow.xcbWindow.m_Window != 0, "Invalid handle passed");
  m_hWnd.xcbWindow.m_Window = parentWindow.xcbWindow.m_Window;

  return EZ_SUCCESS;
}