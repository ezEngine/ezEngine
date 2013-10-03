#include <System/Window/Window.h>
#include <Foundation/Math/Vec2.h>

class GameWindow : public ezWindow
{
public:
  GameWindow();
  ~GameWindow();

  ezSizeU32 GetResolution() const;
  void SwapBuffers();

private:
  void OnResizeMessage(const ezSizeU32& newWindowSize) EZ_OVERRIDE;

  void CreateContextOGL();
  void DestroyContextOGL();

#if EZ_ENABLED(EZ_PLATFORM_WINDOWS)
  void OnWindowMessage(HWND hWnd, UINT Msg, WPARAM WParam, LPARAM LParam);
#else
  #error "No window input handling!"
#endif

  HDC m_hDC;
  HGLRC m_hRC;
};