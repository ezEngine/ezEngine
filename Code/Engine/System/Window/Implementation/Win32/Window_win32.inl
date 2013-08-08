
#include <System/PCH.h>
#include <Foundation/Basics.h>
#include <System/Basics.h>
#include <System/Window/Window.h>

static LRESULT ezWindowsMessageFuncTrampoline(HWND hWnd, UINT Msg, WPARAM WParam, LPARAM LParam)
{
  ezWindow* pWindow = reinterpret_cast<ezWindow*>(GetWindowLong(hWnd, GWLP_USERDATA));

  if(pWindow != NULL)
  {
    return pWindow->WindowsMessageFunction(hWnd, Msg, WParam, LParam);
  }

  return DefWindowProc(hWnd, Msg, WParam, LParam);
}


ezResult ezWindow::Initialize()
{
  if(m_WindowHandle != 0)
    return EZ_SUCCESS;

  EZ_ASSERT_API(m_CreationDescription.m_ClientAreaSize.HasNonZeroArea(), "The client area size can't be zero sized!");

  DWORD dwWindowStyle = WS_POPUP | WS_CAPTION | WS_BORDER | WS_SYSMENU;

  // TODO: Expose as "point" in creation description
  ezUInt32 x = m_CreationDescription.m_bFullscreenWindow ? 0 : 10;
  ezUInt32 y = x;

  // Create rectangle for window
  RECT Rect = {x, y, x + m_CreationDescription.m_ClientAreaSize.width, y + m_CreationDescription.m_ClientAreaSize.height};
  AdjustWindowRect(&Rect, dwWindowStyle, FALSE);

  // Account for left or top placed task bars
  if(!m_CreationDescription.m_bFullscreenWindow)
  {
    RECT RectWorkArea;
    SystemParametersInfo(SPI_GETWORKAREA, 0, &RectWorkArea, 0);

    x += RectWorkArea.left;
    y += RectWorkArea.top;

    int dx = x - Rect.left;
    int dy = y - Rect.top;

    Rect.left += dx;
    Rect.right += dx;

    Rect.top += dy;
    Rect.bottom += dy;
  }

  WNDCLASS windowClass;

  // Initialize window class
  memset(&windowClass, 0, sizeof(WNDCLASS));
  windowClass.style          = CS_OWNDC;
  windowClass.cbClsExtra     = 0;
  windowClass.cbWndExtra     = 0;
  windowClass.hInstance      = GetModuleHandle(NULL);
  windowClass.hIcon          = LoadIcon(NULL, IDI_APPLICATION); // TODO: Expose this somehow?
  windowClass.hCursor        = LoadCursor(NULL, IDC_ARROW);
  windowClass.hbrBackground  = static_cast<HBRUSH>(GetStockObject(BLACK_BRUSH));
  windowClass.lpszMenuName   = NULL;
  windowClass.lpszClassName  = "ezWindow";
  windowClass.lpfnWndProc    = DefWindowProc; // TODO: How do we want to handle this?

  if(!RegisterClass(&windowClass))
  {
    return EZ_FAILURE;
  }

  // We need to have a wchar version of the UTF8 window title
  ezStringWChar s = m_CreationDescription.m_Title.GetData();
  
  m_WindowHandle = CreateWindowW(L"ezWindow", s.GetData(), dwWindowStyle, Rect.left, Rect.top, Rect.right - Rect.left, Rect.bottom - Rect.top, NULL, NULL, GetModuleHandle(NULL), NULL);
  if(m_WindowHandle == 0)
  {
    return EZ_FAILURE;
  }

  SetWindowLongPtr(m_WindowHandle, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(this));

  ShowWindow(m_WindowHandle, SW_SHOW);

  return EZ_SUCCESS;
}

ezResult ezWindow::Destroy()
{
  DestroyWindow(GetNativeWindowHandle());
  SetWindowLongPtr(GetNativeWindowHandle(), GWLP_USERDATA, reinterpret_cast<LONG_PTR>(NULL));

  return EZ_SUCCESS;
}