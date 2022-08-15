
ezEditorProcessViewWindow::~ezEditorProcessViewWindow() = default;

ezResult ezEditorProcessViewWindow::UpdateWindow(ezWindowHandle parentWindow, ezUInt16 uiWidth, ezUInt16 uiHeight)
{
  m_hWnd = parentWindow;
  m_uiWidth = uiWidth;
  m_uiHeight = uiHeight;
}